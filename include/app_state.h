/*
 * Author: Yang
 * Shared state structures and synchronization helpers.
 */
#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "app_config.h"

constexpr size_t WeatherCityMaxLen = 32;
constexpr size_t DeskAiFeatureCount = 5;
constexpr size_t DeskAiClassCount = 4;
constexpr size_t DeskAiTimelineCapacity = 60;

enum class DisplayMode : uint8_t {
  Clock = 0,
  Spectrum = 1,
  Fluid = 2,
  Text = 3,
  Timer = 4,
  Weather = 5,
  Game = 6,
};

enum class ClockTheme : uint8_t {
  Classic = 0,
  Rainbow = 1,
  Breath = 2,
  Night = 3,
  Minimal = 4,
};

enum class TransitionStyle : uint8_t {
  CrossFade = 0,
  Wipe = 1,
  PixelDissolve = 2,
};

enum class SceneReason : uint8_t {
  Manual = 0,
  QuietHours = 1,
  AudioActive = 2,
  MotionActive = 3,
  TimerActive = 4,
  DeskAiFocus = 5,
  DeskAiMeeting = 6,
  DeskAiRest = 7,
  DeskAiAway = 8,
};

enum class DeskState : uint8_t {
  Unknown = 0,
  Focus = 1,
  Meeting = 2,
  Rest = 3,
  Away = 4,
};

enum class TimerMode : uint8_t {
  Pomodoro = 0,
  Countdown = 1,
  Stopwatch = 2,
};

enum class TimerRunState : uint8_t {
  Idle = 0,
  Running = 1,
  Paused = 2,
  Finished = 3,
};

enum class AudioVisualMode : uint8_t {
  Spectrum = 0,
  MirrorSpectrum = 1,
  VuMeter = 2,
  BassPulse = 3,
  FireSpectrum = 4,
  CenterBurst = 5,
};

enum class WeatherDisplayMode : uint8_t {
  IconTemp = 0,
  TempOnly = 1,
  DetailCycle = 2,
};

enum class GameType : uint8_t {
  Snake = 0,
  GravityBall = 1,
  Reaction = 2,
  Pong = 3,
  Breakout = 4,
};

enum class GameRunState : uint8_t {
  Idle = 0,
  Running = 1,
  Paused = 2,
  GameOver = 3,
};

enum class GameDirection : uint8_t {
  Up = 0,
  Down = 1,
  Left = 2,
  Right = 3,
};

struct GamePoint {
  constexpr GamePoint() = default;
  constexpr GamePoint(int8_t px, int8_t py) : x(px), y(py) {}

  int8_t x = 0;
  int8_t y = 0;
};

constexpr int GameBoardW = AppConfig::MatrixWidth;
constexpr int GameBoardH = AppConfig::MatrixHeight;
constexpr int SnakeMaxLen = GameBoardW * GameBoardH;
constexpr int BreakoutBrickWidth = 2;
constexpr int BreakoutBrickRows = 2;
constexpr int BreakoutBrickCols = GameBoardW / BreakoutBrickWidth;
constexpr int BreakoutBrickCount = BreakoutBrickRows * BreakoutBrickCols;
constexpr int BreakoutPaddleWidth = 5;
static_assert(BreakoutBrickCount <= 32, "Breakout brick mask fits in uint32_t");

struct ControlState {
  DisplayMode mode = DisplayMode::Clock;
  ClockTheme clockTheme = ClockTheme::Classic;
  CRGB preferredColor = CRGB(0x3A, 0xD7, 0xFF);
  uint8_t manualBrightness = AppConfig::DefaultBrightness;
  bool autoBrightness = true;
  uint16_t lowLightThreshold = 900;
  uint16_t highLightThreshold = 3300;
  uint8_t fluidParticles = AppConfig::FluidDefaultActiveParticles;
  float fluidFlipRatio = AppConfig::FluidDefaultFlipRatio;
  bool fluidSeparateParticles = true;
  bool fluidCompensateDrift = true;
  char scrollText[AppConfig::ScrollTextMaxLen] = "PIXEL CLOCK";
  uint16_t scrollSpeedMs = AppConfig::DefaultScrollSpeedMs;
  bool scrollRainbow = AppConfig::DefaultScrollRainbow;
  TimerMode timerMode = TimerMode::Pomodoro;
  TimerRunState timerState = TimerRunState::Idle;
  uint32_t timerDurationSec = AppConfig::DefaultPomodoroFocusMin * 60UL;
  uint32_t timerStartMillis = 0;
  uint32_t timerPausedRemainSec = AppConfig::DefaultPomodoroFocusMin * 60UL;
  uint32_t timerElapsedBeforePauseSec = 0;
  uint16_t pomodoroFocusMin = AppConfig::DefaultPomodoroFocusMin;
  uint16_t pomodoroBreakMin = AppConfig::DefaultPomodoroBreakMin;
  bool pomodoroIsBreak = false;
  AudioVisualMode audioVisualMode = AudioVisualMode::Spectrum;
  uint8_t audioSensitivity = AppConfig::DefaultAudioSensitivity;
  uint8_t audioSmoothing = AppConfig::DefaultAudioSmoothing;
  bool audioRainbow = AppConfig::DefaultAudioRainbow;
  bool audioBeatFlash = AppConfig::DefaultAudioBeatFlash;
  bool audioAutoGain = AppConfig::DefaultAudioAutoGain;
  bool smoothTransitions = true;
  TransitionStyle transitionStyle = TransitionStyle::CrossFade;
  uint16_t transitionDurationMs = AppConfig::DefaultTransitionDurationMs;
  bool gammaCorrection = true;
  bool smartScenes = false;
  bool deskAiEnabled = true;
  bool deskAiAutoScene = false;
  float deskAiCentroids[DeskAiClassCount][DeskAiFeatureCount] = {};
  uint16_t deskAiSampleCounts[DeskAiClassCount] = {};
  uint8_t quietStartHour = AppConfig::DefaultQuietStartHour;
  uint8_t quietEndHour = AppConfig::DefaultQuietEndHour;
  uint8_t nightBrightnessCap = AppConfig::DefaultNightBrightnessCap;
  bool weatherEnabled = true;
  WeatherDisplayMode weatherDisplayMode = WeatherDisplayMode::IconTemp;
  char weatherCity[WeatherCityMaxLen] = "Lanzhou";
  float weatherLatitude = 36.0611f;
  float weatherLongitude = 103.8343f;
  uint16_t weatherUpdateIntervalMin = 30;
  GameType gameType = GameType::Snake;
  bool gameUseMpuControl = false;
  uint16_t gameSpeedMs = 160;
};

struct AudioState {
  float rms = 0.0f;
  float peak = 0.0f;
  uint16_t micRaw = 0;
  uint16_t micMin = 4095;
  uint16_t micMax = 0;
  float micBias = 2048.0f;
  float spectrum[AppConfig::SpectrumBins] = {};
  float smoothSpectrum[AppConfig::SpectrumBins] = {};
  float lowEnergy = 0.0f;
  float midEnergy = 0.0f;
  float highEnergy = 0.0f;
  float energy = 0.0f;
  float energyAvg = 0.0f;
  float noiseFloor = 0.0f;
  float autoGain = 1.0f;
  bool signalPresent = false;
  bool beat = false;
  uint32_t lastBeatMs = 0;
  uint32_t frameCounter = 0;
  uint32_t dmaReadCount = 0;
  uint32_t dmaTimeoutCount = 0;
  uint32_t micSampleCount = 0;
  uint32_t vbusSampleCount = 0;
  uint32_t ldrSampleCount = 0;
  bool usingAnalogFallback = false;
};

struct EnvironmentState {
  float temperatureC = NAN;
  float humidityRh = NAN;
  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 1.0f;
  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
  uint16_t rawLdr = 0;
  uint8_t adaptiveBrightness = AppConfig::DefaultBrightness;
  bool mpuOnline = false;
  uint32_t mpuReadCount = 0;
  uint32_t mpuFailCount = 0;
};

struct PowerState {
  float vbus = 0.0f;
  bool highPower = false;
  bool dcdcEnabled = false;
  uint16_t maxMilliamps = 450;
  uint8_t brightnessCap = AppConfig::BrightnessCap5v;
};

struct WeatherState {
  bool online = false;
  bool hasData = false;
  char city[WeatherCityMaxLen] = "Lanzhou";
  float latitude = 36.0611f;
  float longitude = 103.8343f;
  float temperature = 0.0f;
  float apparentTemperature = 0.0f;
  int relativeHumidity = 0;
  int weatherCode = 0;
  float precipitation = 0.0f;
  int cloudCover = 0;
  float windSpeed = 0.0f;
  int todayTempMax = 0;
  int todayTempMin = 0;
  int todayPrecipProb = 0;
  uint32_t lastUpdateMs = 0;
  uint32_t lastSuccessMs = 0;
  uint32_t failCount = 0;
  char lastError[64] = "";
};

struct ContextState {
  DisplayMode effectiveMode = DisplayMode::Clock;
  SceneReason reason = SceneReason::Manual;
  bool quietHours = false;
  bool darkEnvironment = false;
  bool audioActive = false;
  bool motionActive = false;
  uint32_t automaticSwitchCount = 0;
  uint32_t lastEvaluationMs = 0;
};

struct DeskAiState {
  DeskState state = DeskState::Unknown;
  DeskState baselineState = DeskState::Unknown;
  DeskState lastCalibrationLabel = DeskState::Unknown;
  float confidence = 0.0f;
  float baselineConfidence = 0.0f;
  float features[DeskAiFeatureCount] = {};
  float classScores[DeskAiClassCount] = {};
  uint32_t inferenceCount = 0;
  uint32_t lastInferenceMs = 0;
  uint32_t stableSinceMs = 0;
  uint32_t lastCalibrationMs = 0;
  uint32_t lastEvaluationMs = 0;
  uint32_t offlineInferenceCount = 0;
  uint32_t lastOfflineInferenceMs = 0;
  uint16_t inferenceMicros = 0;
  uint16_t evaluationTotal = 0;
  uint16_t personalizedCorrect = 0;
  uint16_t baselineCorrect = 0;
  uint16_t evaluationSamples[DeskAiClassCount] = {};
  uint16_t confusion[DeskAiClassCount][DeskAiClassCount] = {};
  uint8_t profileCoverage = 0;
  uint8_t profileQuality = 0;
  bool profileReady = false;
  float centroidSeparation = 0.0f;
  struct TimelineEntry {
    uint32_t timestampMs = 0;
    DeskState state = DeskState::Unknown;
    uint8_t confidence = 0;
    bool offline = false;
  } timeline[DeskAiTimelineCapacity] = {};
  uint8_t timelineCount = 0;
  uint8_t timelineNext = 0;
  uint32_t lastTimelineEntryMs = 0;
  bool lastInferenceOffline = false;
};

struct NotificationItem {
  char text[AppConfig::NotificationTextMaxLen] = "";
  CRGB color = CRGB(0xFF, 0xB8, 0x6B);
  uint16_t durationMs = AppConfig::DefaultNotificationDurationMs;
  uint16_t speedMs = AppConfig::DefaultNotificationSpeedMs;
};

struct NotificationState {
  NotificationItem queue[AppConfig::NotificationQueueDepth] = {};
  uint8_t count = 0;
  NotificationItem active;
  bool activeVisible = false;
  uint32_t activeStartedMs = 0;
  uint32_t serial = 0;
};

struct GameState {
  GameType type = GameType::Snake;
  GameRunState runState = GameRunState::Idle;
  uint32_t score = 0;
  uint32_t highScore = 0;
  uint32_t lastStepMs = 0;
  uint16_t stepIntervalMs = 160;
  GamePoint snake[SnakeMaxLen] = {};
  uint16_t snakeLen = 3;
  GamePoint food{22, 4};
  GameDirection dir = GameDirection::Right;
  GameDirection nextDir = GameDirection::Right;
  GamePoint ball{16, 4};
  GamePoint target{25, 4};
  int8_t breakoutVelX = 1;
  int8_t breakoutVelY = -1;
  int8_t breakoutPaddleX = 13;
  uint32_t breakoutBricks = 0;
  uint32_t reactionStartMs = 0;
  uint32_t reactionWaitMs = 0;
  bool reactionReady = false;
};

struct RenderState {
  ControlState control;
  AudioState audio;
  EnvironmentState environment;
  PowerState power;
  WeatherState weather;
  GameState game;
  ContextState context;
  DeskAiState deskAi;
  NotificationState notifications;
  time_t unixTime = 0;
};

struct ScreenSnapshot {
  uint8_t rgb[AppConfig::LedCount][3] = {};
  uint32_t frameCounter = 0;
};

struct SharedState {
  SemaphoreHandle_t mutex = nullptr;
  QueueHandle_t renderQueue = nullptr;
  RenderState snapshot;
  ScreenSnapshot screen;
};

extern SharedState gState;

bool initSharedState();
void updateSharedState(const RenderState &patch);
RenderState copySharedState();
bool pushRenderSnapshot(TickType_t timeoutTicks = 0);
void updateControlState(const ControlState &control);
void updateAudioState(const AudioState &audio);
void updateEnvironmentState(const EnvironmentState &environment, time_t unixTime);
void updatePowerState(const PowerState &power);
void updateWeatherState(const WeatherState &weather);
GameState copyGameState();
void updateGameState(const GameState &game);
void updateContextState(const ContextState &context);
void updateDeskAiState(const DeskAiState &deskAi);
NotificationState copyNotificationState();
void updateNotificationState(const NotificationState &notifications);
void updateScreenSnapshot(const CRGB *leds, size_t count);
ScreenSnapshot copyScreenSnapshot();
