/*
 * Author: Yang
 * Persistent user settings stored in ESP32 NVS.
 */
#include "settings_storage.h"

#include <cstring>

#include <Arduino.h>
#include <Preferences.h>

namespace {
constexpr const char *NvsNamespace = "pxclock";
constexpr uint32_t SettingsVersion = 6;
constexpr uint32_t SaveDelayMs = 1500;

portMUX_TYPE saveMux = portMUX_INITIALIZER_UNLOCKED;
bool savePending = false;
uint32_t lastSaveRequestMs = 0;

uint8_t clampU8(int value, int lo, int hi) {
  return static_cast<uint8_t>(constrain(value, lo, hi));
}

uint16_t clampU16(int value, int lo, int hi) {
  return static_cast<uint16_t>(constrain(value, lo, hi));
}

uint32_t clampU32(uint32_t value, uint32_t lo, uint32_t hi) {
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

DisplayMode safeDisplayMode(uint8_t value, DisplayMode fallback) {
  if (value <= static_cast<uint8_t>(DisplayMode::Game)) {
    return static_cast<DisplayMode>(value);
  }
  return fallback;
}

ClockTheme safeClockTheme(uint8_t value, ClockTheme fallback) {
  if (value <= static_cast<uint8_t>(ClockTheme::Minimal)) {
    return static_cast<ClockTheme>(value);
  }
  return fallback;
}

TransitionStyle safeTransitionStyle(uint8_t value, TransitionStyle fallback) {
  if (value <= static_cast<uint8_t>(TransitionStyle::PixelDissolve)) {
    return static_cast<TransitionStyle>(value);
  }
  return fallback;
}

TimerMode safeTimerMode(uint8_t value, TimerMode fallback) {
  if (value <= static_cast<uint8_t>(TimerMode::Stopwatch)) {
    return static_cast<TimerMode>(value);
  }
  return fallback;
}

AudioVisualMode safeAudioVisualMode(uint8_t value, AudioVisualMode fallback) {
  if (value <= static_cast<uint8_t>(AudioVisualMode::CenterBurst)) {
    return static_cast<AudioVisualMode>(value);
  }
  return fallback;
}

WeatherDisplayMode safeWeatherDisplayMode(uint8_t value, WeatherDisplayMode fallback) {
  if (value <= static_cast<uint8_t>(WeatherDisplayMode::DetailCycle)) {
    return static_cast<WeatherDisplayMode>(value);
  }
  return fallback;
}

GameType safeGameType(uint8_t value, GameType fallback) {
  if (value <= static_cast<uint8_t>(GameType::Breakout)) {
    return static_cast<GameType>(value);
  }
  return fallback;
}

void copyStoredText(char *dest, const String &src) {
  size_t out = 0;
  while (out < AppConfig::ScrollTextMaxLen - 1 && out < src.length()) {
    const char c = src[out];
    dest[out] = (c >= 0x20 && c <= 0x7E) ? c : ' ';
    ++out;
  }
  dest[out] = '\0';
}

void copyStoredCity(char *dest, const String &src) {
  size_t out = 0;
  while (out < WeatherCityMaxLen - 1 && out < src.length()) {
    const uint8_t c = static_cast<uint8_t>(src[out]);
    dest[out] = c >= 0x20 ? static_cast<char>(c) : ' ';
    ++out;
  }
  dest[out] = '\0';
}

void resetTimerRuntime(ControlState &control, uint32_t countdownSec) {
  control.timerState = TimerRunState::Idle;
  control.timerStartMillis = 0;
  control.timerElapsedBeforePauseSec = 0;
  control.pomodoroIsBreak = false;

  switch (control.timerMode) {
    case TimerMode::Countdown:
      control.timerDurationSec = countdownSec;
      control.timerPausedRemainSec = countdownSec;
      break;
    case TimerMode::Stopwatch:
      control.timerDurationSec = 0;
      control.timerPausedRemainSec = 0;
      break;
    case TimerMode::Pomodoro:
    default:
      control.timerDurationSec = control.pomodoroFocusMin * 60UL;
      control.timerPausedRemainSec = control.timerDurationSec;
      break;
  }
}

void cancelPendingSave() {
  taskENTER_CRITICAL(&saveMux);
  savePending = false;
  lastSaveRequestMs = 0;
  taskEXIT_CRITICAL(&saveMux);
}
} // namespace

void loadSettingsFromNvs(ControlState &control) {
  Preferences prefs;
  if (!prefs.begin(NvsNamespace, true)) {
    Serial.println("[NVS] open for read failed");
    return;
  }

  const uint32_t version = prefs.getUInt("ver", 0);
  if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5 &&
      version != SettingsVersion) {
    prefs.end();
    Serial.println("[NVS] no valid settings, using defaults");
    return;
  }

  control.mode = safeDisplayMode(prefs.getUChar("mode", static_cast<uint8_t>(control.mode)), control.mode);
  control.clockTheme = safeClockTheme(prefs.getUChar("theme", static_cast<uint8_t>(control.clockTheme)), control.clockTheme);
  control.manualBrightness = clampU8(prefs.getUChar("bright", control.manualBrightness), 1, 255);
  control.autoBrightness = prefs.getBool("autoB", control.autoBrightness);

  control.preferredColor = CRGB(prefs.getUChar("r", control.preferredColor.r),
                                prefs.getUChar("g", control.preferredColor.g),
                                prefs.getUChar("b", control.preferredColor.b));

  control.lowLightThreshold = clampU16(prefs.getUShort("ldrLo", control.lowLightThreshold), 0, 4095);
  control.highLightThreshold = clampU16(prefs.getUShort("ldrHi", control.highLightThreshold), 0, 4095);

  control.fluidParticles = clampU8(prefs.getUChar("flPart", control.fluidParticles), 8, static_cast<int>(AppConfig::FluidParticleCount));
  control.fluidFlipRatio = constrain(prefs.getFloat("flFlip", control.fluidFlipRatio), 0.0f, 1.0f);
  control.fluidSeparateParticles = prefs.getBool("flSep", control.fluidSeparateParticles);
  control.fluidCompensateDrift = prefs.getBool("flDrift", control.fluidCompensateDrift);

  copyStoredText(control.scrollText, prefs.getString("text", String(control.scrollText)));
  control.scrollSpeedMs = clampU16(prefs.getUShort("textSpd", control.scrollSpeedMs), 30, 500);
  control.scrollRainbow = prefs.getBool("textRain", control.scrollRainbow);

  control.timerMode = safeTimerMode(prefs.getUChar("tMode", static_cast<uint8_t>(control.timerMode)), control.timerMode);
  control.pomodoroFocusMin = clampU16(prefs.getUShort("pFocus", control.pomodoroFocusMin), 1, 99);
  control.pomodoroBreakMin = clampU16(prefs.getUShort("pBreak", control.pomodoroBreakMin), 1, 60);
  const uint32_t countdownSec = clampU32(prefs.getUInt("countSec", AppConfig::DefaultCountdownMin * 60UL), 60UL, 99UL * 60UL);
  resetTimerRuntime(control, countdownSec);

  control.audioVisualMode = safeAudioVisualMode(prefs.getUChar("audMode", static_cast<uint8_t>(control.audioVisualMode)), control.audioVisualMode);
  control.audioSensitivity = prefs.getUChar("audSens", control.audioSensitivity);
  control.audioSmoothing = prefs.getUChar("audSmooth", control.audioSmoothing);
  control.audioRainbow = prefs.getBool("audRain", control.audioRainbow);
  control.audioBeatFlash = prefs.getBool("audBeat", control.audioBeatFlash);
  control.audioAutoGain = prefs.getBool("audGain", control.audioAutoGain);

  control.smoothTransitions = prefs.getBool("smoothFx", control.smoothTransitions);
  control.transitionStyle = safeTransitionStyle(
      prefs.getUChar("transFx", static_cast<uint8_t>(control.transitionStyle)),
      control.transitionStyle);
  control.transitionDurationMs = clampU16(
      prefs.getUShort("transMs", control.transitionDurationMs), 120, 1200);
  control.gammaCorrection = prefs.getBool("gamma", control.gammaCorrection);
  control.smartScenes = prefs.getBool("scenes", control.smartScenes);
  control.deskAiEnabled = prefs.getBool("aiOn", control.deskAiEnabled);
  control.deskAiAutoScene = prefs.getBool("aiAuto", control.deskAiAutoScene);
  control.deskAiActiveLearning = prefs.getBool("aiLearn", control.deskAiActiveLearning);
  control.deskAiValidationLocked = false;
  control.deskAiFeedbackThreshold = clampU8(
      prefs.getUChar("aiFeed", control.deskAiFeedbackThreshold), 25, 85);
  control.energyAwareMode = prefs.getBool("energy", control.energyAwareMode);
  control.competitionDemoMode = false;
  const bool loadedDeskAiCentroids =
      prefs.getBytesLength("aiCtr") == sizeof(control.deskAiCentroids);
  if (loadedDeskAiCentroids) {
    prefs.getBytes("aiCtr", control.deskAiCentroids, sizeof(control.deskAiCentroids));
  }
  if (version < 6 && loadedDeskAiCentroids) {
    for (size_t category = 0; category < DeskAiClassCount; ++category) {
      control.deskAiCentroids[category][3] =
          1.0f - constrain(control.deskAiCentroids[category][3], 0.0f, 1.0f);
    }
  }
  if (prefs.getBytesLength("aiCnt") == sizeof(control.deskAiSampleCounts)) {
    prefs.getBytes("aiCnt", control.deskAiSampleCounts, sizeof(control.deskAiSampleCounts));
  }
  control.quietStartHour = clampU8(prefs.getUChar("quietFrom", control.quietStartHour), 0, 23);
  control.quietEndHour = clampU8(prefs.getUChar("quietTo", control.quietEndHour), 0, 23);
  control.nightBrightnessCap = clampU8(
      prefs.getUChar("nightCap", control.nightBrightnessCap), 1, 96);

  control.weatherEnabled = prefs.getBool("wEn", control.weatherEnabled);
  control.weatherDisplayMode = safeWeatherDisplayMode(prefs.getUChar("wMode", static_cast<uint8_t>(control.weatherDisplayMode)),
                                                      control.weatherDisplayMode);
  copyStoredCity(control.weatherCity, prefs.getString("wCity", String(control.weatherCity)));
  control.weatherLatitude = constrain(prefs.getFloat("wLat", control.weatherLatitude), -90.0f, 90.0f);
  control.weatherLongitude = constrain(prefs.getFloat("wLon", control.weatherLongitude), -180.0f, 180.0f);
  control.weatherAutoLocate = prefs.getBool("wGeo", control.weatherAutoLocate);
  control.weatherUpdateIntervalMin = clampU16(prefs.getUShort("wInt", control.weatherUpdateIntervalMin), 5, 180);

  control.gameType = safeGameType(prefs.getUChar("gameType", static_cast<uint8_t>(control.gameType)), control.gameType);
  control.gameSpeedMs = clampU16(prefs.getUShort("gameSpeed", control.gameSpeedMs), 60, 600);
  control.gameUseMpuControl = prefs.getBool("gameMpu", control.gameUseMpuControl);

  prefs.end();
  Serial.println("[NVS] settings loaded");
  if (version < SettingsVersion) {
    saveSettingsToNvs(control);
    Serial.println("[NVS] migrated LDR/Desk AI settings to v6");
  }
}

void saveSettingsToNvs(const ControlState &control) {
  Preferences prefs;
  if (!prefs.begin(NvsNamespace, false)) {
    Serial.println("[NVS] open for write failed");
    return;
  }

  prefs.putUInt("ver", SettingsVersion);
  prefs.putUChar("mode", static_cast<uint8_t>(control.mode));
  prefs.putUChar("theme", static_cast<uint8_t>(control.clockTheme));
  prefs.putUChar("bright", control.manualBrightness);
  prefs.putBool("autoB", control.autoBrightness);
  prefs.putUChar("r", control.preferredColor.r);
  prefs.putUChar("g", control.preferredColor.g);
  prefs.putUChar("b", control.preferredColor.b);

  prefs.putUShort("ldrLo", control.lowLightThreshold);
  prefs.putUShort("ldrHi", control.highLightThreshold);

  prefs.putUChar("flPart", control.fluidParticles);
  prefs.putFloat("flFlip", control.fluidFlipRatio);
  prefs.putBool("flSep", control.fluidSeparateParticles);
  prefs.putBool("flDrift", control.fluidCompensateDrift);

  prefs.putString("text", control.scrollText);
  prefs.putUShort("textSpd", control.scrollSpeedMs);
  prefs.putBool("textRain", control.scrollRainbow);

  prefs.putUChar("tMode", static_cast<uint8_t>(control.timerMode));
  if (control.timerMode == TimerMode::Countdown && control.timerDurationSec > 0) {
    prefs.putUInt("countSec", clampU32(control.timerDurationSec, 60UL, 99UL * 60UL));
  }
  prefs.putUShort("pFocus", control.pomodoroFocusMin);
  prefs.putUShort("pBreak", control.pomodoroBreakMin);

  prefs.putUChar("audMode", static_cast<uint8_t>(control.audioVisualMode));
  prefs.putUChar("audSens", control.audioSensitivity);
  prefs.putUChar("audSmooth", control.audioSmoothing);
  prefs.putBool("audRain", control.audioRainbow);
  prefs.putBool("audBeat", control.audioBeatFlash);
  prefs.putBool("audGain", control.audioAutoGain);

  prefs.putBool("smoothFx", control.smoothTransitions);
  prefs.putUChar("transFx", static_cast<uint8_t>(control.transitionStyle));
  prefs.putUShort("transMs", clampU16(control.transitionDurationMs, 120, 1200));
  prefs.putBool("gamma", control.gammaCorrection);
  prefs.putBool("scenes", control.smartScenes);
  prefs.putBool("aiOn", control.deskAiEnabled);
  prefs.putBool("aiAuto", control.deskAiAutoScene);
  prefs.putBool("aiLearn", control.deskAiActiveLearning);
  prefs.putUChar("aiFeed", clampU8(control.deskAiFeedbackThreshold, 25, 85));
  prefs.putBool("energy", control.energyAwareMode);
  prefs.putBytes("aiCtr", control.deskAiCentroids, sizeof(control.deskAiCentroids));
  prefs.putBytes("aiCnt", control.deskAiSampleCounts, sizeof(control.deskAiSampleCounts));
  prefs.putUChar("quietFrom", clampU8(control.quietStartHour, 0, 23));
  prefs.putUChar("quietTo", clampU8(control.quietEndHour, 0, 23));
  prefs.putUChar("nightCap", clampU8(control.nightBrightnessCap, 1, 96));

  prefs.putBool("wEn", control.weatherEnabled);
  prefs.putUChar("wMode", static_cast<uint8_t>(control.weatherDisplayMode));
  prefs.putString("wCity", control.weatherCity);
  prefs.putFloat("wLat", control.weatherLatitude);
  prefs.putFloat("wLon", control.weatherLongitude);
  prefs.putBool("wGeo", control.weatherAutoLocate);
  prefs.putUShort("wInt", clampU16(control.weatherUpdateIntervalMin, 5, 180));

  prefs.putUChar("gameType", static_cast<uint8_t>(control.gameType));
  prefs.putUShort("gameSpeed", clampU16(control.gameSpeedMs, 60, 600));
  prefs.putBool("gameMpu", control.gameUseMpuControl);

  prefs.end();
  Serial.println("[NVS] settings saved");
}

void clearSettingsNvs() {
  cancelPendingSave();

  Preferences prefs;
  if (!prefs.begin(NvsNamespace, false)) {
    Serial.println("[NVS] open for clear failed");
    return;
  }

  prefs.clear();
  prefs.end();
  Serial.println("[NVS] settings cleared");
}

void requestSettingsSave() {
  const uint32_t now = millis();
  taskENTER_CRITICAL(&saveMux);
  savePending = true;
  lastSaveRequestMs = now;
  taskEXIT_CRITICAL(&saveMux);
}

void serviceSettingsSave() {
  const uint32_t now = millis();
  bool shouldSave = false;

  taskENTER_CRITICAL(&saveMux);
  if (savePending && now - lastSaveRequestMs >= SaveDelayMs) {
    savePending = false;
    shouldSave = true;
  }
  taskEXIT_CRITICAL(&saveMux);

  if (!shouldSave) {
    return;
  }

  const RenderState state = copySharedState();
  saveSettingsToNvs(state.control);
}
