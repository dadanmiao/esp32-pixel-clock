/*
 * Author: Yang
 * Shared state synchronization implementation.
 */
#include "app_state.h"

#include <cstring>

SharedState gState;

bool initSharedState() {
  gState.mutex = xSemaphoreCreateMutex();
  gState.renderQueue = xQueueCreate(1, sizeof(RenderState));
  return gState.mutex != nullptr && gState.renderQueue != nullptr;
}

void updateSharedState(const RenderState &patch) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    gState.snapshot = patch;
    xSemaphoreGive(gState.mutex);
  }
}

RenderState copySharedState() {
  RenderState copy;
  if (!gState.mutex) {
    return copy;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    copy = gState.snapshot;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

ControlState copyControlState() {
  ControlState copy;
  if (gState.mutex && xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.control;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

AudioState copyAudioState() {
  AudioState copy;
  if (gState.mutex && xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.audio;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

EnvironmentState copyEnvironmentState() {
  EnvironmentState copy;
  if (gState.mutex && xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.environment;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

PowerState copyPowerState() {
  PowerState copy;
  if (gState.mutex && xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.power;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

void updateControlState(const ControlState &control) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.control = control;
    xSemaphoreGive(gState.mutex);
  }
}

void updateAudioState(const AudioState &audio) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.audio = audio;
    xSemaphoreGive(gState.mutex);
  }
}

void updateEnvironmentState(const EnvironmentState &environment, time_t unixTime) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.environment = environment;
    gState.snapshot.unixTime = unixTime;
    xSemaphoreGive(gState.mutex);
  }
}

void updatePowerState(const PowerState &power) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.power = power;
    xSemaphoreGive(gState.mutex);
  }
}

void updateWeatherState(const WeatherState &weather) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.weather = weather;
    xSemaphoreGive(gState.mutex);
  }
}

GameState copyGameState() {
  GameState copy;
  if (!gState.mutex) {
    return copy;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.game;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

void updateGameState(const GameState &game) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.game = game;
    xSemaphoreGive(gState.mutex);
  }
}

void updateContextState(const ContextState &context) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.context = context;
    xSemaphoreGive(gState.mutex);
  }
}

void updateDeskAiState(const DeskAiState &deskAi) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.deskAi = deskAi;
    xSemaphoreGive(gState.mutex);
  }
}

void updateDeskAiInferenceState(const DeskAiState &deskAi) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    const DeskAiState &latest = gState.snapshot.deskAi;
    DeskAiState merged = deskAi;
    merged.lastCalibrationLabel = latest.lastCalibrationLabel;
    merged.lastCalibrationMs = latest.lastCalibrationMs;
    merged.evaluationTotal = latest.evaluationTotal;
    merged.personalizedCorrect = latest.personalizedCorrect;
    merged.baselineCorrect = latest.baselineCorrect;
    merged.quantizedCorrect = latest.quantizedCorrect;
    merged.rejectedPredictions = latest.rejectedPredictions;
    memcpy(merged.evaluationSamples, latest.evaluationSamples, sizeof(merged.evaluationSamples));
    memcpy(merged.confusion, latest.confusion, sizeof(merged.confusion));
    merged.lastBlindActual = latest.lastBlindActual;
    merged.lastBlindPersonalized = latest.lastBlindPersonalized;
    merged.lastBlindBaseline = latest.lastBlindBaseline;
    merged.lastBlindQuantized = latest.lastBlindQuantized;
    merged.lastBlindConfidence = latest.lastBlindConfidence;
    merged.lastBlindResultMs = latest.lastBlindResultMs;
    merged.lastEvaluationMs = latest.lastEvaluationMs;
    if (latest.feedbackResolvedCount > merged.feedbackResolvedCount) {
      merged.feedbackRequested = latest.feedbackRequested;
      merged.feedbackSuggestedState = latest.feedbackSuggestedState;
      merged.feedbackRequestedMs = latest.feedbackRequestedMs;
      merged.feedbackRequestCount = latest.feedbackRequestCount;
      merged.feedbackResolvedCount = latest.feedbackResolvedCount;
      merged.lowConfidenceSinceMs = latest.lowConfidenceSinceMs;
    }
    gState.snapshot.deskAi = merged;
    xSemaphoreGive(gState.mutex);
  }
}

void updateCompetitionState(const CompetitionState &competition) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.competition = competition;
    xSemaphoreGive(gState.mutex);
  }
}

NotificationState copyNotificationState() {
  NotificationState copy;
  if (!gState.mutex) {
    return copy;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    copy = gState.snapshot.notifications;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}

void updateNotificationState(const NotificationState &notifications) {
  if (!gState.mutex) {
    return;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    gState.snapshot.notifications = notifications;
    xSemaphoreGive(gState.mutex);
  }
}

bool pushRenderSnapshot(TickType_t timeoutTicks) {
  if (!gState.renderQueue) {
    return false;
  }
  (void)timeoutTicks;
  if (!gState.mutex) {
    return false;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  const BaseType_t result = xQueueOverwrite(gState.renderQueue, &gState.snapshot);
  xSemaphoreGive(gState.mutex);
  return result == pdPASS;
}

void updateScreenSnapshot(const CRGB *leds, size_t count) {
  if (!gState.mutex || !leds) {
    return;
  }

  const size_t copyCount = count < AppConfig::LedCount ? count : AppConfig::LedCount;
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    for (size_t i = 0; i < copyCount; ++i) {
      gState.screen.rgb[i][0] = leds[i].r;
      gState.screen.rgb[i][1] = leds[i].g;
      gState.screen.rgb[i][2] = leds[i].b;
    }
    for (size_t i = copyCount; i < AppConfig::LedCount; ++i) {
      gState.screen.rgb[i][0] = 0;
      gState.screen.rgb[i][1] = 0;
      gState.screen.rgb[i][2] = 0;
    }
    gState.screen.frameCounter++;
    xSemaphoreGive(gState.mutex);
  }
}

ScreenSnapshot copyScreenSnapshot() {
  ScreenSnapshot copy;
  if (!gState.mutex) {
    return copy;
  }
  if (xSemaphoreTake(gState.mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    copy = gState.screen;
    xSemaphoreGive(gState.mutex);
  }
  return copy;
}
