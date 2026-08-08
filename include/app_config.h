/*
 * Author: Yang
 * Application-wide build-time configuration.
 */
#pragma once

#include <Arduino.h>

namespace AppConfig {
constexpr const char *FirmwareVersion = "2.7.3";
constexpr uint16_t MatrixWidth = 32;
constexpr uint16_t MatrixHeight = 8;
constexpr uint16_t LedCount = MatrixWidth * MatrixHeight;

constexpr uint32_t RenderFps = 60;
constexpr uint32_t SensorPollMs = 50;
constexpr uint32_t PowerPollMs = 500;
constexpr uint32_t DeskAiInferenceIntervalMs = 1000;
constexpr uint32_t DeskAiAwayTimeoutMs = 90UL * 1000UL;
constexpr uint32_t DeskAiTimelineIntervalMs = 30UL * 1000UL;
constexpr uint8_t DeskAiMinCalibrationSamplesPerClass = 4;
constexpr uint8_t DeskAiRecommendedCalibrationSamplesPerClass = 8;
constexpr float DeskAiMinCentroidSeparation = 0.12f;
constexpr uint32_t DeskAiFeedbackHoldMs = 5000;
constexpr uint32_t DeskAiFeedbackCooldownMs = 30000;
constexpr float DeskAiUnknownConfidenceThreshold = 0.30f;
constexpr float DeskAiUnknownDistanceThreshold = 0.58f;
constexpr uint8_t DeskAiStateConfirmFrames = 3;

constexpr uint32_t AudioSampleRate = 16000;
constexpr size_t AudioFftSize = 256;
constexpr size_t SpectrumBins = MatrixWidth;
constexpr bool AudioUseAdcDma = false; // ESP32-S3 Arduino 2.x ADC DMA can be board-package sensitive. Use analogRead by default.

constexpr size_t FluidParticleCount = 64;
constexpr uint8_t FluidDefaultActiveParticles = 48;
constexpr float FluidDefaultFlipRatio = 0.78f;
constexpr uint8_t FluidCoreBrightness = 185;
constexpr uint8_t FluidEdgeBrightness = 44;

constexpr size_t ScrollTextMaxLen = 64;
constexpr uint16_t DefaultScrollSpeedMs = 90;
constexpr bool DefaultScrollRainbow = true;

constexpr uint16_t DefaultPomodoroFocusMin = 25;
constexpr uint16_t DefaultPomodoroBreakMin = 5;
constexpr uint16_t DefaultCountdownMin = 5;

constexpr uint8_t DefaultAudioSensitivity = 128;
constexpr uint8_t DefaultAudioSmoothing = 160;
constexpr bool DefaultAudioRainbow = true;
constexpr bool DefaultAudioBeatFlash = true;
constexpr bool DefaultAudioAutoGain = true;

constexpr uint16_t DefaultTransitionDurationMs = 320;
constexpr uint8_t DefaultNightBrightnessCap = 22;
constexpr uint8_t DefaultQuietStartHour = 23;
constexpr uint8_t DefaultQuietEndHour = 7;
constexpr size_t NotificationTextMaxLen = 48;
constexpr size_t NotificationQueueDepth = 4;
constexpr uint16_t DefaultNotificationDurationMs = 5000;
constexpr uint16_t DefaultNotificationSpeedMs = 72;

constexpr uint8_t DefaultBrightness = 64;
constexpr uint8_t MinAutoBrightness = 8;
constexpr uint8_t MaxAutoBrightness = 180;
constexpr uint8_t BrightnessCap12v = 255;
constexpr uint8_t BrightnessCap5v = 48;
constexpr uint8_t BrightnessCapUnknown = 24;

constexpr float VbusDividerRatio = 6.1f; // 51k upper and 10k lower divider: (51k + 10k) / 10k.
constexpr float Vbus12vThreshold = 8.0f;
constexpr float Vbus5vThreshold = 3.5f;
constexpr uint32_t DcdcEnableDelayMs = 1200;

constexpr const char *Hostname = "pixel-fluid-clock";
constexpr const char *NtpServer1 = "ntp.aliyun.com";
constexpr const char *NtpServer2 = "time1.cloud.tencent.com";
constexpr long GmtOffsetSec = 8 * 3600;
constexpr int DaylightOffsetSec = 0;
} // namespace AppConfig
