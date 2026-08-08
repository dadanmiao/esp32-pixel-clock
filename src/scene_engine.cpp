/*
 * Author: Yang
 * Context-aware display scene selection.
 */
#include "scene_engine.h"

#include <cmath>
#include <ctime>

#include <Arduino.h>

#include "app_state.h"

namespace {
constexpr uint32_t EvaluationIntervalMs = 250;
constexpr uint32_t AudioHoldMs = 8000;
constexpr uint32_t MotionHoldMs = 3500;
constexpr uint32_t AudioConfirmMs = 250;
constexpr time_t ValidClockEpoch = 1577836800; // 2020-01-01 UTC
constexpr float MotionActivityThreshold = 0.18f;

uint32_t lastEvaluationMs = 0;
uint32_t lastAudioActiveMs = 0;
uint32_t lastMotionActiveMs = 0;
uint32_t audioSignalStartedMs = 0;
float previousAccelX = 0.0f;
float previousAccelY = 0.0f;
float previousAccelZ = 0.0f;
bool motionSampleReady = false;

bool hourIsWithin(uint8_t hour, uint8_t startHour, uint8_t endHour) {
  if (startHour == endHour) {
    return false;
  }
  if (startHour < endHour) {
    return hour >= startHour && hour < endHour;
  }
  return hour >= startHour || hour < endHour;
}

bool sameContext(const ContextState &a, const ContextState &b) {
  return a.effectiveMode == b.effectiveMode &&
         a.reason == b.reason &&
         a.quietHours == b.quietHours &&
         a.darkEnvironment == b.darkEnvironment &&
         a.audioActive == b.audioActive &&
         a.motionActive == b.motionActive;
}
} // namespace

void serviceSceneEngine() {
  const uint32_t now = millis();
  if (now - lastEvaluationMs < EvaluationIntervalMs) {
    return;
  }
  lastEvaluationMs = now;

  const RenderState state = copySharedState();
  ContextState next = state.context;

  struct tm local = {};
  if (state.unixTime >= ValidClockEpoch) {
    localtime_r(&state.unixTime, &local);
    next.quietHours = hourIsWithin(
        static_cast<uint8_t>(local.tm_hour),
        state.control.quietStartHour,
        state.control.quietEndHour);
  } else {
    // Do not enter a scheduled scene using the 1970 epoch before NTP sync.
    next.quietHours = false;
  }

  // The LDR ADC rises as illumination falls. Enter darkness at the upper
  // calibration threshold and leave it after crossing a small hysteresis band.
  const uint16_t darkEnter =
      max(state.control.lowLightThreshold, state.control.highLightThreshold);
  const uint16_t darkExit = static_cast<uint16_t>(
      constrain(static_cast<int>(darkEnter) - 220, 0, 4095));
  if (next.darkEnvironment) {
    next.darkEnvironment = state.environment.rawLdr > darkExit;
  } else {
    next.darkEnvironment = state.environment.rawLdr > darkEnter;
  }

  if (state.audio.signalPresent) {
    if (audioSignalStartedMs == 0) {
      audioSignalStartedMs = now;
    }
    if (now - audioSignalStartedMs >= AudioConfirmMs) {
      lastAudioActiveMs = now;
    }
  } else {
    audioSignalStartedMs = 0;
  }
  next.audioActive = lastAudioActiveMs != 0 && now - lastAudioActiveMs < AudioHoldMs;

  if (state.environment.mpuOnline) {
    if (motionSampleReady) {
      const float accelDelta = fabsf(state.environment.accelX - previousAccelX) +
                               fabsf(state.environment.accelY - previousAccelY) +
                               fabsf(state.environment.accelZ - previousAccelZ);
      const float gyroActivity = (fabsf(state.environment.gyroX) +
                                  fabsf(state.environment.gyroY) +
                                  fabsf(state.environment.gyroZ)) * 0.012f;
      if (accelDelta + gyroActivity > MotionActivityThreshold) {
        lastMotionActiveMs = now;
      }
    }
    previousAccelX = state.environment.accelX;
    previousAccelY = state.environment.accelY;
    previousAccelZ = state.environment.accelZ;
    motionSampleReady = true;
  } else {
    motionSampleReady = false;
  }
  next.motionActive = lastMotionActiveMs != 0 && now - lastMotionActiveMs < MotionHoldMs;

  next.effectiveMode = state.control.mode;
  next.reason = SceneReason::Manual;

  const bool ambientMode = state.control.mode == DisplayMode::Clock ||
                           state.control.mode == DisplayMode::Spectrum ||
                           state.control.mode == DisplayMode::Fluid;
  const bool timerActive = state.control.timerState == TimerRunState::Running ||
                           state.control.timerState == TimerRunState::Finished;
  const bool ruleNight = state.control.smartScenes && ambientMode &&
                         (next.quietHours || next.darkEnvironment);
  const bool interactiveMotion =
      (state.control.smartScenes || state.control.deskAiAutoScene) &&
      ambientMode && next.motionActive && !next.quietHours && !next.darkEnvironment;
  const bool ruleAudio = state.control.smartScenes && ambientMode && next.audioActive;

  // Keep the priorities explicit so AI cannot overwrite safety or direct
  // interaction scenes: timer > night > motion > audio > AI > manual.
  if (state.control.smartScenes && timerActive) {
    next.effectiveMode = DisplayMode::Timer;
    next.reason = SceneReason::TimerActive;
  } else if (ruleNight) {
    next.effectiveMode = DisplayMode::Clock;
    next.reason = SceneReason::QuietHours;
  } else if (!timerActive && interactiveMotion) {
    next.effectiveMode = DisplayMode::Fluid;
    next.reason = SceneReason::MotionActive;
  } else if (!timerActive && ruleAudio) {
    next.effectiveMode = DisplayMode::Spectrum;
    next.reason = SceneReason::AudioActive;
  } else if (!timerActive && state.control.deskAiEnabled &&
             state.control.deskAiAutoScene && ambientMode) {
    switch (state.deskAi.state) {
      case DeskState::Focus:
        next.effectiveMode = DisplayMode::Clock;
        next.reason = SceneReason::DeskAiFocus;
        break;
      case DeskState::Meeting:
        next.effectiveMode = DisplayMode::Clock;
        next.reason = SceneReason::DeskAiMeeting;
        break;
      case DeskState::Rest:
        next.effectiveMode = DisplayMode::Fluid;
        next.reason = SceneReason::DeskAiRest;
        break;
      case DeskState::Away:
        next.effectiveMode = DisplayMode::Clock;
        next.reason = SceneReason::DeskAiAway;
        break;
      case DeskState::Unknown:
      default:
        break;
    }
  }

  if ((state.control.smartScenes || state.control.deskAiAutoScene) &&
      next.effectiveMode != state.context.effectiveMode) {
    next.automaticSwitchCount = state.context.automaticSwitchCount + 1;
  }
  next.lastEvaluationMs = now;

  if (!sameContext(next, state.context) ||
      now - state.context.lastEvaluationMs >= 1000) {
    updateContextState(next);
    pushRenderSnapshot(0);
  }
}
