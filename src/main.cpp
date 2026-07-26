/*
 * Author: Yang
 * ESP32-S3 Desktop Pixel Clock and Fluid Rhythm Terminal entry point.
 */
#include <Arduino.h>
#include <cstring>

#include "app_state.h"
#include "audio_task.h"
#include "competition_metrics.h"
#include "display_task.h"
#include "desk_ai.h"
#include "game_logic.h"
#include "notification_manager.h"
#include "pinmap.h"
#include "power_manager.h"
#include "scene_engine.h"
#include "sensor_task.h"
#include "settings_storage.h"
#include "timer_logic.h"
#include "web_server.h"
#include "weather_task.h"
#include "wifi_manager_app.h"

namespace {
void initInitialState() {
  RenderState state;
  state.control.mode = DisplayMode::Clock;
  state.control.clockTheme = ClockTheme::Classic;
  state.control.preferredColor = CRGB(0x3A, 0xD7, 0xFF);
  state.control.manualBrightness = AppConfig::DefaultBrightness;
  state.control.autoBrightness = true;
  state.control.lowLightThreshold = 900;
  state.control.highLightThreshold = 3300;
  state.control.fluidParticles = AppConfig::FluidDefaultActiveParticles;
  state.control.fluidFlipRatio = AppConfig::FluidDefaultFlipRatio;
  state.control.fluidSeparateParticles = true;
  state.control.fluidCompensateDrift = true;
  state.control.timerMode = TimerMode::Pomodoro;
  state.control.timerState = TimerRunState::Idle;
  state.control.pomodoroFocusMin = AppConfig::DefaultPomodoroFocusMin;
  state.control.pomodoroBreakMin = AppConfig::DefaultPomodoroBreakMin;
  state.control.pomodoroIsBreak = false;
  state.control.timerDurationSec = AppConfig::DefaultPomodoroFocusMin * 60UL;
  state.control.timerPausedRemainSec = state.control.timerDurationSec;
  state.control.timerStartMillis = 0;
  state.control.timerElapsedBeforePauseSec = 0;
  state.control.audioVisualMode = AudioVisualMode::Spectrum;
  state.control.audioSensitivity = AppConfig::DefaultAudioSensitivity;
  state.control.audioSmoothing = AppConfig::DefaultAudioSmoothing;
  state.control.audioRainbow = AppConfig::DefaultAudioRainbow;
  state.control.audioBeatFlash = AppConfig::DefaultAudioBeatFlash;
  state.control.weatherEnabled = true;
  state.control.weatherDisplayMode = WeatherDisplayMode::IconTemp;
  strncpy(state.control.weatherCity, "Lanzhou", WeatherCityMaxLen - 1);
  state.control.weatherCity[WeatherCityMaxLen - 1] = '\0';
  state.control.weatherLatitude = 36.0611f;
  state.control.weatherLongitude = 103.8343f;
  state.control.weatherUpdateIntervalMin = 30;
  state.control.gameType = GameType::Snake;
  state.control.gameUseMpuControl = false;
  state.control.gameSpeedMs = 160;
  initializeDeskAiProfile(state.control);
  strncpy(state.weather.city, state.control.weatherCity, WeatherCityMaxLen - 1);
  state.weather.city[WeatherCityMaxLen - 1] = '\0';
  state.weather.latitude = state.control.weatherLatitude;
  state.weather.longitude = state.control.weatherLongitude;
  state.environment.adaptiveBrightness = AppConfig::DefaultBrightness;
  state.power.maxMilliamps = 450;
  state.power.brightnessCap = AppConfig::BrightnessCap5v;
  loadSettingsFromNvs(state.control);
  refreshDeskAiProfileMetrics(state.control, state.deskAi);
  state.context.effectiveMode = state.control.mode;
  state.context.reason = SceneReason::Manual;
  gameReset(state.game, state.control);
  strncpy(state.weather.city, state.control.weatherCity, WeatherCityMaxLen - 1);
  state.weather.city[WeatherCityMaxLen - 1] = '\0';
  state.weather.latitude = state.control.weatherLatitude;
  state.weather.longitude = state.control.weatherLongitude;
  time(&state.unixTime);
  updateSharedState(state);
}
} // namespace

size_t getArduinoLoopTaskStackSize() {
  // RenderState contains the game, notification, and Desk AI evidence snapshots.
  // The Arduino default of 8 KB is no longer sufficient when these snapshots are copied.
  return 16 * 1024;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n[boot] ESP32-S3 Pixel Fluid Clock v%s\n", AppConfig::FirmwareVersion);

  pinMode(static_cast<uint8_t>(Pinmap::USER_BOOT), INPUT_PULLUP);

  if (!initSharedState()) {
    Serial.println("[fatal] shared state init failed");
    while (true) {
      delay(1000);
    }
  }

  initInitialState();
  initPowerManager();

  const bool wifiOk = startWiFiManager();
  if (!wifiOk) {
    Serial.println("[wifi] station is not connected yet, console remains available");
  }
  startWebServer();

  startAudioTask();   // Core 0: ADC DMA and FFT.
  startSensorTask();  // Core 0: I2C, LDR, time snapshot.
  startPowerTask();   // Core 0: VBUS policy and DC-DC enable.
  startWeatherTask(); // Core 0: Open-Meteo fetcher.
  startDisplayTask(); // Core 1: FastLED animation engine.

  markSystemFullyStarted();
}

void loop() {
  static uint32_t lastBootCheck = 0;
  static uint32_t lastTimerServiceMs = 0;
  if (millis() - lastBootCheck > 50) {
    lastBootCheck = millis();
    if (digitalRead(static_cast<uint8_t>(Pinmap::USER_BOOT)) == LOW) {
      RenderState state = copySharedState();
      const uint32_t pressStart = millis();
      while (digitalRead(static_cast<uint8_t>(Pinmap::USER_BOOT)) == LOW && millis() - pressStart < 1200) {
        delay(20);
      }
      const bool longPress = millis() - pressStart >= 800;
      if (state.control.mode == DisplayMode::Game) {
        if (longPress) {
          gameReset(state.game, state.control);
        } else if (state.game.runState == GameRunState::Idle) {
          gameStart(state.game);
        } else if (state.game.runState == GameRunState::Running) {
          gamePause(state.game);
        } else if (state.game.runState == GameRunState::Paused) {
          gameResume(state.game);
        } else {
          gameReset(state.game, state.control);
          gameStart(state.game);
        }
      } else if (state.control.mode == DisplayMode::Timer) {
        if (longPress) {
          resetTimer(state.control);
        } else {
          applyTimerAction(state.control, "toggle", millis());
        }
      } else {
        const uint8_t nextMode = (static_cast<uint8_t>(state.control.mode) + 1) %
                                 (static_cast<uint8_t>(DisplayMode::Game) + 1);
        state.control.mode = static_cast<DisplayMode>(nextMode);
        requestSettingsSave();
      }
      updateControlState(state.control);
      updateGameState(state.game);
      pushRenderSnapshot(0);
      delay(350);
    }
  }
  if (millis() - lastTimerServiceMs >= 100) {
    lastTimerServiceMs = millis();
    if (serviceTimerState(lastTimerServiceMs)) {
      enqueueNotification("TIME UP", CRGB(0xFF, 0x4C, 0x35), 7000, 70);
    }
  }
  serviceSceneEngine();
  serviceDeskAi();
  serviceCompetitionMetrics();
  serviceNotifications();
  serviceWiFiManager();
  serviceWebServer();
  serviceSettingsSave();
  vTaskDelay(pdMS_TO_TICKS(10));
}
