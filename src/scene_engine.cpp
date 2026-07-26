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

uint32_t lastEvaluationMs = 0;
uint32_t lastAudioActiveMs = 0;
uint32_t lastMotionActiveMs = 0;

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
  localtime_r(&state.unixTime, &local);
  next.quietHours = hourIsWithin(
      static_cast<uint8_t>(local.tm_hour),
      state.control.quietStartHour,
      state.control.quietEndHour);

  const uint16_t darkEnter = state.control.lowLightThreshold;
  const uint16_t darkExit = static_cast<uint16_t>(
      constrain(static_cast<int>(darkEnter) + 220, 0, 4095));
  if (next.darkEnvironment) {
    next.darkEnvironment = state.environment.rawLdr < darkExit;
  } else {
    next.darkEnvironment = state.environment.rawLdr < darkEnter;
  }

  if (state.audio.signalPresent) {
    lastAudioActiveMs = now;
  }
  next.audioActive = lastAudioActiveMs != 0 && now - lastAudioActiveMs < AudioHoldMs;

  const float motion = fabsf(state.environment.accelX) +
                       fabsf(state.environment.accelY) +
                       fabsf(state.environment.gyroX) * 0.012f +
                       fabsf(state.environment.gyroY) * 0.012f;
  if (state.environment.mpuOnline && motion > 0.48f) {
    lastMotionActiveMs = now;
  }
  next.motionActive = lastMotionActiveMs != 0 && now - lastMotionActiveMs < MotionHoldMs;

  next.effectiveMode = state.control.mode;
  next.reason = SceneReason::Manual;

  const bool ambientMode = state.control.mode == DisplayMode::Clock ||
                           state.control.mode == DisplayMode::Spectrum ||
                           state.control.mode == DisplayMode::Fluid;
  if (state.control.smartScenes) {
    if (state.control.timerState == TimerRunState::Running ||
        state.control.timerState == TimerRunState::Finished) {
      next.effectiveMode = DisplayMode::Timer;
      next.reason = SceneReason::TimerActive;
    } else if (ambientMode && (next.quietHours || next.darkEnvironment)) {
      next.effectiveMode = DisplayMode::Clock;
      next.reason = SceneReason::QuietHours;
    } else if (ambientMode && next.audioActive) {
      next.effectiveMode = DisplayMode::Spectrum;
      next.reason = SceneReason::AudioActive;
    } else if (ambientMode && next.motionActive) {
      next.effectiveMode = DisplayMode::Fluid;
      next.reason = SceneReason::MotionActive;
    }
  }

  if (state.control.deskAiEnabled && state.control.deskAiAutoScene && ambientMode &&
      state.control.timerState != TimerRunState::Running &&
      state.control.timerState != TimerRunState::Finished) {
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
