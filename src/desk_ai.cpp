/*
 * Author: Yang
 * A tiny, explainable nearest-prototype classifier. It runs entirely on the ESP32-S3.
 */
#include "desk_ai.h"

#include <cmath>
#include <cstring>

#include <Arduino.h>
#include <WiFi.h>

namespace {
constexpr float FeatureWeights[DeskAiFeatureCount] = {1.20f, 0.72f, 1.28f, 0.54f, 1.08f};
constexpr float DefaultCentroids[DeskAiClassCount][DeskAiFeatureCount] = {
    {0.10f, 0.09f, 0.05f, 0.38f, 0.13f}, // Focus
    {0.62f, 0.38f, 0.14f, 0.45f, 0.74f}, // Meeting
    {0.05f, 0.05f, 0.03f, 0.22f, 0.04f}, // Rest
    {0.01f, 0.01f, 0.01f, 0.07f, 0.00f}, // Away
};

uint32_t lastServiceMs = 0;
uint32_t lastEngagementMs = 0;
float previousAccelX = 0.0f;
float previousAccelY = 0.0f;
float previousAccelZ = 1.0f;
uint8_t candidateIndex = 0xFF;
uint8_t candidateFrames = 0;
float smoothedFeatures[DeskAiFeatureCount] = {};
bool smoothedFeaturesReady = false;

struct Classification {
  uint8_t best = 0;
  float bestDistance = 1000.0f;
  float secondDistance = 1000.0f;
  float confidence = 0.0f;
  float scores[DeskAiClassCount] = {};
};

struct QuantizedClassification {
  uint8_t best = 0;
  uint32_t bestDistance = UINT32_MAX;
  uint32_t secondDistance = UINT32_MAX;
  float confidence = 0.0f;
};

float clampUnit(float value) {
  return constrain(value, 0.0f, 1.0f);
}

uint32_t modelFingerprint(const ControlState &control) {
  uint32_t hash = 2166136261UL;
  const auto append = [&hash](const uint8_t *bytes, size_t length) {
    for (size_t index = 0; index < length; ++index) {
      hash ^= bytes[index];
      hash *= 16777619UL;
    }
  };
  append(reinterpret_cast<const uint8_t *>(control.deskAiCentroids),
         sizeof(control.deskAiCentroids));
  append(reinterpret_cast<const uint8_t *>(control.deskAiSampleCounts),
         sizeof(control.deskAiSampleCounts));
  return hash;
}

uint8_t classIndex(DeskState state) {
  if (state >= DeskState::Focus && state <= DeskState::Away) {
    return static_cast<uint8_t>(state) - 1;
  }
  return 0xFF;
}

DeskState stateForClass(uint8_t index) {
  return index < DeskAiClassCount ? static_cast<DeskState>(index + 1) : DeskState::Unknown;
}

Classification classifyFeatures(
    const float features[DeskAiFeatureCount],
    const float centroids[DeskAiClassCount][DeskAiFeatureCount]) {
  Classification result;
  for (uint8_t category = 0; category < DeskAiClassCount; ++category) {
    float weightedSum = 0.0f;
    float totalWeight = 0.0f;
    for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
      const float difference = features[feature] - centroids[category][feature];
      weightedSum += difference * difference * FeatureWeights[feature];
      totalWeight += FeatureWeights[feature];
    }
    const float distance = sqrtf(weightedSum / totalWeight);
    result.scores[category] = clampUnit(1.0f - distance);
    if (distance < result.bestDistance) {
      result.secondDistance = result.bestDistance;
      result.best = category;
      result.bestDistance = distance;
    } else if (distance < result.secondDistance) {
      result.secondDistance = distance;
    }
  }
  const float separation = clampUnit((result.secondDistance - result.bestDistance) / 0.55f);
  result.confidence = clampUnit((1.0f - result.bestDistance) * 0.62f + separation * 0.38f);
  return result;
}

QuantizedClassification classifyFeaturesInt8(
    const float features[DeskAiFeatureCount],
    const float centroids[DeskAiClassCount][DeskAiFeatureCount]) {
  static constexpr uint8_t QuantizedWeights[DeskAiFeatureCount] = {12, 7, 13, 5, 11};
  QuantizedClassification result;
  for (uint8_t category = 0; category < DeskAiClassCount; ++category) {
    uint32_t distance = 0;
    for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
      const int16_t input = static_cast<int16_t>(clampUnit(features[feature]) * 127.0f + 0.5f);
      const int16_t center = static_cast<int16_t>(clampUnit(centroids[category][feature]) * 127.0f + 0.5f);
      const int16_t difference = input - center;
      distance += static_cast<uint32_t>(difference * difference) * QuantizedWeights[feature];
    }
    if (distance < result.bestDistance) {
      result.secondDistance = result.bestDistance;
      result.bestDistance = distance;
      result.best = category;
    } else if (distance < result.secondDistance) {
      result.secondDistance = distance;
    }
  }
  const float normalized = sqrtf(static_cast<float>(result.bestDistance) / (48.0f * 127.0f * 127.0f));
  const float separation = result.secondDistance == UINT32_MAX
                               ? 0.0f
                               : clampUnit(static_cast<float>(result.secondDistance - result.bestDistance) /
                                           static_cast<float>(48UL * 127UL * 127UL));
  result.confidence = clampUnit((1.0f - normalized) * 0.68f + separation * 0.32f);
  return result;
}

float centroidDistance(
    const float first[DeskAiFeatureCount],
    const float second[DeskAiFeatureCount]) {
  float weightedSum = 0.0f;
  float totalWeight = 0.0f;
  for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
    const float difference = first[feature] - second[feature];
    weightedSum += difference * difference * FeatureWeights[feature];
    totalWeight += FeatureWeights[feature];
  }
  return sqrtf(weightedSum / totalWeight);
}

void appendTimeline(DeskAiState &deskAi, uint32_t now, bool stateChanged, bool offline) {
  const bool intervalReached = now - deskAi.lastTimelineEntryMs >= AppConfig::DeskAiTimelineIntervalMs;
  if (deskAi.timelineCount != 0 && !stateChanged && !intervalReached) {
    return;
  }
  DeskAiState::TimelineEntry &entry = deskAi.timeline[deskAi.timelineNext];
  entry.timestampMs = now;
  entry.state = deskAi.state;
  entry.confidence = static_cast<uint8_t>(clampUnit(deskAi.confidence) * 100.0f);
  entry.offline = offline;
  deskAi.timelineNext = (deskAi.timelineNext + 1) % DeskAiTimelineCapacity;
  if (deskAi.timelineCount < DeskAiTimelineCapacity) {
    ++deskAi.timelineCount;
  }
  deskAi.lastTimelineEntryMs = now;
}

void extractFeatures(const RenderState &state, float features[DeskAiFeatureCount]) {
  if (state.control.competitionDemoMode) {
    const uint8_t demoClass = static_cast<uint8_t>((millis() / 8000UL) % DeskAiClassCount);
    const float phase = static_cast<float>((millis() / 250UL) % 9) * 0.0025f;
    for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
      const float direction = ((feature + demoClass) & 1U) == 0 ? 1.0f : -1.0f;
      features[feature] = clampUnit(DefaultCentroids[demoClass][feature] + phase * direction);
    }
    return;
  }

  const float gain = state.control.audioAutoGain ? state.audio.autoGain : 1.0f;
  const float audioAboveFloor = fmaxf(0.0f, state.audio.rms - state.audio.noiseFloor * 1.08f);
  const float audio = clampUnit(audioAboveFloor * gain * 5.2f + state.audio.energy * gain * 1.35f);
  const float bass = clampUnit(state.audio.lowEnergy * gain * 2.7f);

  const float accelDelta = fabsf(state.environment.accelX - previousAccelX) +
                           fabsf(state.environment.accelY - previousAccelY) +
                           fabsf(state.environment.accelZ - previousAccelZ);
  const float gyro = (fabsf(state.environment.gyroX) + fabsf(state.environment.gyroY) +
                      fabsf(state.environment.gyroZ)) * 0.008f;
  const float motion = state.environment.mpuOnline ? clampUnit(accelDelta * 0.85f + gyro) : 0.0f;
  const float light = clampUnit(static_cast<float>(state.environment.rawLdr) / 4095.0f);
  const float engagement = clampUnit(fmaxf(audio * 0.82f, motion * 0.95f));

  features[0] = audio;
  features[1] = bass;
  features[2] = motion;
  features[3] = light;
  features[4] = engagement;

  previousAccelX = state.environment.accelX;
  previousAccelY = state.environment.accelY;
  previousAccelZ = state.environment.accelZ;
}

void resetRuntimeClassifier() {
  candidateIndex = 0xFF;
  candidateFrames = 0;
  lastEngagementMs = millis();
  smoothedFeaturesReady = false;
  memset(smoothedFeatures, 0, sizeof(smoothedFeatures));
}
} // namespace

const char *deskStateToString(DeskState state) {
  switch (state) {
    case DeskState::Focus: return "Focus";
    case DeskState::Meeting: return "Meeting";
    case DeskState::Rest: return "Rest";
    case DeskState::Away: return "Away";
    case DeskState::Unknown:
    default: return "Unknown";
  }
}

void initializeDeskAiProfile(ControlState &control) {
  memcpy(control.deskAiCentroids, DefaultCentroids, sizeof(DefaultCentroids));
  memset(control.deskAiSampleCounts, 0, sizeof(control.deskAiSampleCounts));
  resetRuntimeClassifier();
}

void resetDeskAiProfile(ControlState &control) {
  initializeDeskAiProfile(control);
}

bool calibrateDeskAiProfile(ControlState &control, DeskAiState &deskAi, DeskState label) {
  if (control.deskAiValidationLocked) {
    return false;
  }
  const uint8_t index = classIndex(label);
  if (index == 0xFF) {
    return false;
  }

  const uint16_t oldCount = control.deskAiSampleCounts[index];
  const uint16_t cappedCount = oldCount < 12 ? oldCount + 1 : 12;
  const float alpha = oldCount == 0 ? 0.45f : 1.0f / static_cast<float>(cappedCount);
  for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
    control.deskAiCentroids[index][feature] =
        control.deskAiCentroids[index][feature] * (1.0f - alpha) + deskAi.features[feature] * alpha;
  }
  control.deskAiSampleCounts[index] = oldCount < 999 ? oldCount + 1 : 999;
  deskAi.lastCalibrationLabel = label;
  deskAi.lastCalibrationMs = millis();
  return true;
}

bool recordDeskAiEvaluation(DeskAiState &deskAi, DeskState actualLabel) {
  const uint8_t actual = classIndex(actualLabel);
  const uint8_t predicted = classIndex(deskAi.state);
  const uint8_t baseline = classIndex(deskAi.baselineState);
  const uint8_t quantized = classIndex(deskAi.quantizedState);
  if (actual == 0xFF || baseline == 0xFF || quantized == 0xFF) {
    return false;
  }
  deskAi.lastBlindActual = actualLabel;
  deskAi.lastBlindPersonalized = deskAi.state;
  deskAi.lastBlindBaseline = deskAi.baselineState;
  deskAi.lastBlindQuantized = deskAi.quantizedState;
  deskAi.lastBlindConfidence = deskAi.confidence;
  deskAi.lastBlindResultMs = millis();
  ++deskAi.evaluationTotal;
  ++deskAi.evaluationSamples[actual];
  if (predicted == 0xFF) {
    ++deskAi.rejectedPredictions;
  } else {
    ++deskAi.confusion[actual][predicted];
    if (predicted == actual) {
      ++deskAi.personalizedCorrect;
    }
  }
  if (baseline == actual) {
    ++deskAi.baselineCorrect;
  }
  if (quantized == actual) {
    ++deskAi.quantizedCorrect;
  }
  deskAi.lastEvaluationMs = millis();
  return true;
}

bool resolveDeskAiFeedback(ControlState &control, DeskAiState &deskAi, DeskState actualLabel) {
  if (control.deskAiValidationLocked ||
      !recordDeskAiEvaluation(deskAi, actualLabel) ||
      !calibrateDeskAiProfile(control, deskAi, actualLabel)) {
    return false;
  }
  deskAi.feedbackRequested = false;
  deskAi.feedbackSuggestedState = DeskState::Unknown;
  deskAi.lowConfidenceSinceMs = 0;
  ++deskAi.feedbackResolvedCount;
  refreshDeskAiProfileMetrics(control, deskAi);
  return true;
}

void resetDeskAiEvaluation(DeskAiState &deskAi) {
  deskAi.evaluationTotal = 0;
  deskAi.personalizedCorrect = 0;
  deskAi.baselineCorrect = 0;
  deskAi.quantizedCorrect = 0;
  deskAi.rejectedPredictions = 0;
  memset(deskAi.evaluationSamples, 0, sizeof(deskAi.evaluationSamples));
  memset(deskAi.confusion, 0, sizeof(deskAi.confusion));
  deskAi.lastBlindActual = DeskState::Unknown;
  deskAi.lastBlindPersonalized = DeskState::Unknown;
  deskAi.lastBlindBaseline = DeskState::Unknown;
  deskAi.lastBlindQuantized = DeskState::Unknown;
  deskAi.lastBlindConfidence = 0.0f;
  deskAi.lastBlindResultMs = 0;
  deskAi.lastEvaluationMs = millis();
}

void refreshDeskAiProfileMetrics(const ControlState &control, DeskAiState &deskAi) {
  uint8_t coveredClasses = 0;
  uint16_t cappedSamples = 0;
  for (size_t category = 0; category < DeskAiClassCount; ++category) {
    const uint16_t samples = control.deskAiSampleCounts[category];
    if (samples >= AppConfig::DeskAiMinCalibrationSamplesPerClass) {
      ++coveredClasses;
    }
    cappedSamples += samples > AppConfig::DeskAiRecommendedCalibrationSamplesPerClass
                         ? AppConfig::DeskAiRecommendedCalibrationSamplesPerClass
                         : samples;
  }

  float separationSum = 0.0f;
  uint8_t separationPairs = 0;
  for (size_t first = 0; first < DeskAiClassCount; ++first) {
    if (control.deskAiSampleCounts[first] == 0) {
      continue;
    }
    for (size_t second = first + 1; second < DeskAiClassCount; ++second) {
      if (control.deskAiSampleCounts[second] == 0) {
        continue;
      }
      separationSum += centroidDistance(control.deskAiCentroids[first], control.deskAiCentroids[second]);
      ++separationPairs;
    }
  }

  const float averageSeparation = separationPairs == 0 ? 0.0f : separationSum / separationPairs;
  const float coverageScore = static_cast<float>(cappedSamples) /
                              static_cast<float>(DeskAiClassCount * AppConfig::DeskAiRecommendedCalibrationSamplesPerClass);
  const float separationScore = clampUnit(averageSeparation / 0.42f);

  deskAi.profileCoverage = coveredClasses;
  deskAi.centroidSeparation = averageSeparation;
  deskAi.profileQuality = static_cast<uint8_t>(
      clampUnit(coverageScore * 0.70f + separationScore * 0.30f) * 100.0f + 0.5f);
  deskAi.profileReady = coveredClasses == DeskAiClassCount &&
                        averageSeparation >= AppConfig::DeskAiMinCentroidSeparation;
  deskAi.modelFingerprint = modelFingerprint(control);
}

void serviceDeskAi() {
  const uint32_t now = millis();
  if (now - lastServiceMs < AppConfig::DeskAiInferenceIntervalMs) {
    return;
  }
  lastServiceMs = now;

  const uint32_t startedUs = micros();
  const RenderState state = copySharedState();
  DeskAiState next = state.deskAi;
  float rawFeatures[DeskAiFeatureCount] = {};
  extractFeatures(state, rawFeatures);
  if (!smoothedFeaturesReady) {
    memcpy(smoothedFeatures, rawFeatures, sizeof(smoothedFeatures));
    smoothedFeaturesReady = true;
  } else {
    for (size_t feature = 0; feature < DeskAiFeatureCount; ++feature) {
      const float alpha = feature == 2 ? 0.72f : 0.38f;
      smoothedFeatures[feature] =
          smoothedFeatures[feature] * (1.0f - alpha) + rawFeatures[feature] * alpha;
    }
  }
  memcpy(next.features, smoothedFeatures, sizeof(smoothedFeatures));

  if (smoothedFeatures[4] > 0.12f || state.context.audioActive || state.context.motionActive) {
    lastEngagementMs = now;
  }

  Classification personalized = classifyFeatures(smoothedFeatures, state.control.deskAiCentroids);
  Classification baseline = classifyFeatures(smoothedFeatures, DefaultCentroids);
  const uint32_t quantizedStartedUs = micros();
  const QuantizedClassification quantized =
      classifyFeaturesInt8(smoothedFeatures, state.control.deskAiCentroids);
  const uint32_t quantizedElapsedUs = micros() - quantizedStartedUs;
  uint8_t best = personalized.best;
  bool rejected = personalized.confidence < AppConfig::DeskAiUnknownConfidenceThreshold ||
                  personalized.bestDistance > AppConfig::DeskAiUnknownDistanceThreshold;
  memcpy(next.classScores, personalized.scores, sizeof(next.classScores));
  next.baselineState = stateForClass(baseline.best);
  next.baselineConfidence = baseline.confidence;
  next.quantizedState = stateForClass(quantized.best);
  next.quantizedConfidence = quantized.confidence;
  next.quantizedInferenceMicros =
      static_cast<uint16_t>(quantizedElapsedUs < 65535 ? quantizedElapsedUs : 65535);
  next.demoActive = state.control.competitionDemoMode;
  refreshDeskAiProfileMetrics(state.control, next);

  if (now - lastEngagementMs >= AppConfig::DeskAiAwayTimeoutMs) {
    best = classIndex(DeskState::Away);
    rejected = false;
  }

  next.confidence = personalized.confidence;
  const DeskState bestState = rejected ? DeskState::Unknown : stateForClass(best);
  const uint8_t nextCandidate = rejected ? 0xFE : best;

  if (!state.control.deskAiEnabled) {
    next.state = DeskState::Unknown;
    candidateIndex = 0xFF;
    candidateFrames = 0;
  } else if (nextCandidate == candidateIndex) {
    candidateFrames = candidateFrames < 8 ? candidateFrames + 1 : 8;
  } else {
    candidateIndex = nextCandidate;
    candidateFrames = 1;
  }

  if (state.control.deskAiEnabled &&
      (next.state == bestState ||
       candidateFrames >= AppConfig::DeskAiStateConfirmFrames ||
       next.state == DeskState::Unknown) &&
      (bestState == DeskState::Unknown || next.confidence >= 0.20f)) {
    const bool stateChanged = next.state != bestState;
    if (stateChanged) {
      next.stableSinceMs = now;
    }
    next.state = bestState;
  }

  const float feedbackThreshold =
      static_cast<float>(state.control.deskAiFeedbackThreshold) / 100.0f;
  if (!state.control.deskAiEnabled || !state.control.deskAiActiveLearning ||
      state.control.deskAiValidationLocked || state.control.competitionDemoMode ||
      next.confidence >= feedbackThreshold) {
    next.lowConfidenceSinceMs = 0;
    if (!state.control.deskAiActiveLearning || state.control.competitionDemoMode) {
      next.feedbackRequested = false;
    }
  } else {
    if (next.lowConfidenceSinceMs == 0) {
      next.lowConfidenceSinceMs = now;
    }
    const bool heldLongEnough = now - next.lowConfidenceSinceMs >= AppConfig::DeskAiFeedbackHoldMs;
    const bool cooldownElapsed = next.feedbackRequestedMs == 0 ||
                                 now - next.feedbackRequestedMs >= AppConfig::DeskAiFeedbackCooldownMs;
    if (!next.feedbackRequested && heldLongEnough && cooldownElapsed) {
      next.feedbackRequested = true;
      next.feedbackSuggestedState = bestState;
      next.feedbackRequestedMs = now;
      ++next.feedbackRequestCount;
    }
  }

  next.inferenceCount++;
  next.lastInferenceMs = now;
  next.lastInferenceOffline = WiFi.status() != WL_CONNECTED;
  if (next.lastInferenceOffline) {
    ++next.offlineInferenceCount;
    next.lastOfflineInferenceMs = now;
  }
  const uint32_t elapsedUs = micros() - startedUs;
  next.inferenceMicros = static_cast<uint16_t>(elapsedUs < 65535 ? elapsedUs : 65535);
  appendTimeline(next, now, next.state != state.deskAi.state, next.lastInferenceOffline);
  updateDeskAiState(next);
  pushRenderSnapshot(0);
}
