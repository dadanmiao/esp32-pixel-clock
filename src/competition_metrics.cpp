/*
 * Single-device competition evidence: productivity, health, privacy, and
 * display energy estimates. All calculations stay on the ESP32-S3.
 */
#include "competition_metrics.h"

#include <cmath>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

#include "app_state.h"
#include "audio_task.h"
#include "display_task.h"
#include "power_manager.h"
#include "sensor_task.h"
#include "weather_task.h"

namespace {
uint32_t lastServiceMs = 0;
uint32_t lastScreenFrame = 0;
uint32_t lastFpsSampleMs = 0;
DeskState previousState = DeskState::Unknown;
bool wifiWasConnected = false;
portMUX_TYPE metricsMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t apiRequestCounter = 0;
uint32_t externalRequestCounter = 0;
uint32_t networkBytesReceivedCounter = 0;

uint8_t deskStateIndex(DeskState state) {
  if (state >= DeskState::Focus && state <= DeskState::Away) {
    return static_cast<uint8_t>(state) - 1;
  }
  return 0xFF;
}

uint8_t effectiveBrightness(const RenderState &state, bool includeEnergyAware) {
  uint8_t brightness = state.control.autoBrightness
                           ? state.environment.adaptiveBrightness
                           : state.control.manualBrightness;
  brightness = min(brightness, state.power.brightnessCap);
  if (state.control.smartScenes &&
      (state.context.quietHours || state.context.darkEnvironment)) {
    brightness = min(brightness, state.control.nightBrightnessCap);
  }
  if (includeEnergyAware && state.control.energyAwareMode &&
      state.deskAi.state == DeskState::Away) {
    brightness = min(brightness, static_cast<uint8_t>(4));
  }
  return brightness;
}

float estimateLedCurrentMa(
    const RenderState &state,
    const ScreenSnapshot &screen,
    bool includeEnergyAware) {
  uint32_t rgbSum = 0;
  for (size_t index = 0; index < AppConfig::LedCount; ++index) {
    rgbSum += screen.rgb[index][0];
    rgbSum += screen.rgb[index][1];
    rgbSum += screen.rgb[index][2];
  }
  const float channelLoad =
      static_cast<float>(rgbSum) / static_cast<float>(AppConfig::LedCount * 3UL * 255UL);
  const float brightnessScale =
      static_cast<float>(effectiveBrightness(state, includeEnergyAware)) / 255.0f;
  const float ledCurrent = AppConfig::LedCount * (0.8f + 59.2f * channelLoad * brightnessScale);
  return fminf(ledCurrent, static_cast<float>(state.power.maxMilliamps));
}
} // namespace

void recordCompetitionApiRequest(size_t responseBytes) {
  portENTER_CRITICAL(&metricsMux);
  ++apiRequestCounter;
  networkBytesReceivedCounter += static_cast<uint32_t>(responseBytes);
  portEXIT_CRITICAL(&metricsMux);
}

void recordCompetitionExternalRequest(size_t responseBytes) {
  portENTER_CRITICAL(&metricsMux);
  ++externalRequestCounter;
  networkBytesReceivedCounter += static_cast<uint32_t>(responseBytes);
  portEXIT_CRITICAL(&metricsMux);
}

void serviceCompetitionMetrics() {
  const uint32_t now = millis();
  if (lastServiceMs != 0 && now - lastServiceMs < 1000) {
    return;
  }

  const uint32_t elapsedMs = lastServiceMs == 0 ? 0 : now - lastServiceMs;
  lastServiceMs = now;

  const RenderState state = copySharedState();
  const ScreenSnapshot screen = copyScreenSnapshot();
  CompetitionState next = state.competition;
  next.lastUpdateMs = now;
  next.localOnly = true;
  next.rawUploadCount = 0;
  next.cloudInferenceCount = 0;
  portENTER_CRITICAL(&metricsMux);
  next.apiRequestCount = apiRequestCounter;
  next.externalRequestCount = externalRequestCounter;
  next.networkBytesReceived = networkBytesReceivedCounter;
  portEXIT_CRITICAL(&metricsMux);

  const uint8_t currentIndex = deskStateIndex(state.deskAi.state);
  if (elapsedMs > 0 && currentIndex != 0xFF && state.control.deskAiEnabled) {
    next.stateDurationMs[currentIndex] += elapsedMs;
  }

  if (state.deskAi.state != previousState && state.deskAi.state != DeskState::Unknown) {
    if (previousState != DeskState::Unknown) {
      ++next.stateChangeCount;
    }
    if (state.deskAi.state == DeskState::Focus) {
      ++next.focusSessionCount;
      next.currentFocusMs = 0;
    } else if (previousState == DeskState::Focus) {
      ++next.focusInterruptionCount;
      next.currentFocusMs = 0;
    }
    previousState = state.deskAi.state;
  }

  if (state.deskAi.state == DeskState::Focus && elapsedMs > 0) {
    next.currentFocusMs += elapsedMs;
    next.longestFocusMs = max(next.longestFocusMs, next.currentFocusMs);
  } else if (state.deskAi.state != DeskState::Unknown) {
    next.currentFocusMs = 0;
  }

  const uint64_t productiveWindow =
      static_cast<uint64_t>(next.stateDurationMs[0]) +
      static_cast<uint64_t>(next.stateDurationMs[1]) +
      static_cast<uint64_t>(next.stateDurationMs[2]);
  const float focusRatio = productiveWindow == 0
                               ? 0.0f
                               : static_cast<float>(next.stateDurationMs[0]) /
                                     static_cast<float>(productiveWindow);
  const float sustainedFocus =
      fminf(1.0f, static_cast<float>(next.longestFocusMs) / (25.0f * 60.0f * 1000.0f));
  next.focusScore = static_cast<uint8_t>(
      constrain((focusRatio * 0.70f + sustainedFocus * 0.30f) * 100.0f, 0.0f, 100.0f));

  next.audioHealthy = state.audio.frameCounter > 0 && state.audio.micSampleCount > 0;
  next.motionHealthy = state.environment.mpuOnline && state.environment.mpuReadCount > 0;
  next.environmentHealthy = state.audio.ldrSampleCount > 0 &&
                            (isfinite(state.environment.temperatureC) ||
                             isfinite(state.environment.humidityRh));
  const uint32_t frameDelta = screen.frameCounter - lastScreenFrame;
  next.displayHealthy = screen.frameCounter > lastScreenFrame;
  next.powerHealthy = state.power.vbus >= 3.0f && state.power.brightnessCap > 0;
  next.wifiHealthy = WiFi.status() == WL_CONNECTED ||
                     WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA;
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  if (wifiWasConnected && !wifiConnected) {
    ++next.wifiDisconnectCount;
  }
  wifiWasConnected = wifiConnected;

  const uint8_t healthyCount =
      static_cast<uint8_t>(next.audioHealthy) +
      static_cast<uint8_t>(next.motionHealthy) +
      static_cast<uint8_t>(next.environmentHealthy) +
      static_cast<uint8_t>(next.displayHealthy) +
      static_cast<uint8_t>(next.powerHealthy) +
      static_cast<uint8_t>(next.wifiHealthy);
  next.healthScore = static_cast<uint8_t>((healthyCount * 100U) / 6U);

  next.estimatedCurrentMa = estimateLedCurrentMa(state, screen, true);
  next.estimatedPowerW = next.estimatedCurrentMa * 5.0f / 1000.0f;
  const float baselineCurrentMa = estimateLedCurrentMa(state, screen, false);
  next.estimatedBaselinePowerW = baselineCurrentMa * 5.0f / 1000.0f;
  next.estimatedSavedPowerW =
      fmaxf(0.0f, next.estimatedBaselinePowerW - next.estimatedPowerW);
  if (elapsedMs > 0) {
    next.estimatedEnergyWh +=
        next.estimatedPowerW * static_cast<float>(elapsedMs) / 3600000.0f;
    next.estimatedEnergySavedWh +=
        next.estimatedSavedPowerW * static_cast<float>(elapsedMs) / 3600000.0f;
  }

  next.minFreeHeap = ESP.getMinFreeHeap();
  next.resetReason = static_cast<uint32_t>(esp_reset_reason());
  if (lastFpsSampleMs != 0 && now > lastFpsSampleMs) {
    const uint32_t fps = frameDelta * 1000UL / (now - lastFpsSampleMs);
    next.displayFps = static_cast<uint16_t>(fps > 999 ? 999 : fps);
  }
  lastScreenFrame = screen.frameCounter;
  lastFpsSampleMs = now;
  next.taskStackWatermark[0] = getAudioTaskStackWatermark();
  next.taskStackWatermark[1] = getSensorTaskStackWatermark();
  next.taskStackWatermark[2] = getPowerTaskStackWatermark();
  next.taskStackWatermark[3] = getWeatherTaskStackWatermark();
  next.taskStackWatermark[4] = getDisplayTaskStackWatermark();

  updateCompetitionState(next);
}
