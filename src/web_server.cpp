/*
 * Author: Yang
 * Wi-Fi, NTP, and AsyncWebServer dashboard.
 */
#include "web_server.h"

#include <cstring>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <time.h>

#include "app_config.h"
#include "app_state.h"
#include "competition_metrics.h"
#include "desk_ai.h"
#include "game_logic.h"
#include "notification_manager.h"
#include "settings_storage.h"
#include "timer_logic.h"
#include "web_assets.h"
#include "weather_task.h"
#include "wifi_manager_app.h"

namespace {
AsyncWebServer server(80);
AsyncWebSocket realtimeSocket("/ws");
uint32_t lastRealtimePushMs = 0;

const char indexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Pixel Fluid Clock</title>
  <style>
    :root { color-scheme: dark; --bg:#101316; --panel:#191f24; --line:#2e3942; --text:#edf5f7; --muted:#91a0a8; --accent:#3ad7ff; --warm:#ffb86b; }
    * { box-sizing: border-box; }
    body { margin:0; min-height:100vh; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; background: radial-gradient(circle at top left, #19313a, var(--bg) 44%); color:var(--text); }
    main { width:min(980px, 100%); margin:0 auto; padding:28px 18px; }
    header { display:flex; justify-content:space-between; gap:16px; align-items:end; margin-bottom:20px; }
    h1 { margin:0; font-size:clamp(28px, 5vw, 54px); letter-spacing:0; }
    .sub { color:var(--muted); margin-top:6px; }
    .grid { display:grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap:14px; }
    .card { background: color-mix(in srgb, var(--panel), transparent 8%); border:1px solid var(--line); border-radius:8px; padding:16px; box-shadow: 0 16px 42px rgba(0,0,0,.22); }
    .seg { display:grid; grid-template-columns: repeat(auto-fit, minmax(72px, 1fr)); gap:8px; }
    button { min-height:42px; border:1px solid var(--line); border-radius:6px; background:#12181d; color:var(--text); font-weight:700; cursor:pointer; }
    button.active { border-color:var(--accent); background:#12313a; color:#e9fbff; }
    label { display:block; color:var(--muted); font-size:13px; margin:14px 0 8px; }
    input[type="range"] { width:100%; accent-color:var(--accent); }
    input[type="color"], input[type="text"], input[type="number"], select { border:1px solid var(--line); border-radius:6px; background:#12181d; color:var(--text); }
    input[type="color"] { width:56px; height:38px; padding:3px; }
    input[type="text"] { width:100%; min-height:38px; padding:0 10px; font-weight:700; }
    input[type="number"] { width:96px; min-height:38px; padding:0 10px; font-weight:700; }
    select { min-height:38px; padding:0 10px; font-weight:700; }
    .row { display:flex; align-items:center; justify-content:space-between; gap:12px; }
    .value { color:var(--warm); font-variant-numeric: tabular-nums; }
    .metrics { display:grid; grid-template-columns: repeat(2, 1fr); gap:10px; }
    .metric { border:1px solid var(--line); border-radius:6px; padding:12px; background:#11171b; }
    .metric b { display:block; font-size:22px; margin-top:4px; }
    .toggle { display:flex; align-items:center; gap:10px; justify-content:flex-start; margin-top:14px; color:var(--muted); }
    .bars { display:grid; grid-template-columns: repeat(32, 1fr); gap:2px; height:84px; align-items:end; }
    .bar { min-height:3px; background:linear-gradient(var(--accent), #aaff7c); border-radius:3px 3px 0 0; }
  </style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>Pixel Fluid Clock</h1>
      <div class="sub">ESP32-S3 desktop terminal</div>
    </div>
    <div class="value" id="clock">--:--:--</div>
  </header>

  <section class="grid">
    <div class="card">
      <div class="seg">
        <button data-mode="0">Clock</button>
        <button data-mode="1">Spectrum</button>
        <button data-mode="2">Fluid</button>
        <button data-mode="3">Text</button>
        <button data-mode="4">Timer</button>
      </div>
      <label>Color</label>
      <div class="row"><input id="color" type="color" value="#3ad7ff"><span id="modeName" class="value">Clock</span></div>
      <div class="seg">
        <button id="saveSettings" type="button">Save</button>
        <button id="resetSettings" type="button">Defaults</button>
      </div>
      <label>Clock theme</label>
      <select id="clockTheme">
        <option value="0">Classic</option>
        <option value="1">Rainbow</option>
        <option value="2">Breath</option>
        <option value="3">Night</option>
        <option value="4">Minimal</option>
      </select>
      <label>Audio visual</label>
      <div class="row">
        <select id="audioVisualMode">
          <option value="0">Spectrum</option>
          <option value="1">Mirror Spectrum</option>
          <option value="2">VU Meter</option>
          <option value="3">Bass Pulse</option>
          <option value="4">Fire Spectrum</option>
          <option value="5">Center Burst</option>
        </select>
        <button id="showSpectrumMode" type="button">Show</button>
      </div>
      <label>Audio sensitivity <span id="audioSensitivityValue" class="value">128</span></label>
      <input id="audioSensitivity" type="range" min="0" max="255" value="128">
      <label>Audio smoothing <span id="audioSmoothingValue" class="value">160</span></label>
      <input id="audioSmoothing" type="range" min="0" max="255" value="160">
      <label class="toggle"><input id="audioRainbow" type="checkbox" checked> Rainbow Audio</label>
      <label class="toggle"><input id="audioBeatFlash" type="checkbox" checked> Beat Flash</label>
      <label>Scrolling text</label>
      <div class="row"><input id="scrollText" type="text" maxlength="63" value="PIXEL CLOCK" placeholder="PIXEL CLOCK"><button id="applyScrollText" type="button">Apply</button></div>
      <label>Text speed <span id="scrollSpeedValue" class="value">90 ms</span></label>
      <input id="scrollSpeed" type="range" min="30" max="500" value="90">
      <label class="toggle"><input id="scrollRainbow" type="checkbox" checked> Rainbow Text</label>
      <label>Timer</label>
      <div class="row">
        <select id="timerMode">
          <option value="0">Pomodoro</option>
          <option value="1">Countdown</option>
          <option value="2">Stopwatch</option>
        </select>
        <span id="timerStatus" class="value">--:--</span>
      </div>
      <label>Countdown minutes</label>
      <div class="row"><input id="timerMinutes" type="number" min="1" max="99" value="5"><button id="setCountdown" type="button">Set</button></div>
      <label>Pomodoro</label>
      <div class="row"><span>Focus</span><input id="pomodoroFocusMin" type="number" min="1" max="99" value="25"><span>Break</span><input id="pomodoroBreakMin" type="number" min="1" max="60" value="5"></div>
      <div class="seg">
        <button id="timerStart" type="button">Start</button>
        <button id="timerPause" type="button">Pause</button>
        <button id="timerResume" type="button">Resume</button>
        <button id="timerReset" type="button">Reset</button>
      </div>
      <label>Manual brightness <span id="brightnessValue" class="value">64</span></label>
      <input id="brightness" type="range" min="1" max="255" value="64">
      <label class="toggle"><input id="autoBrightness" type="checkbox" checked> Auto brightness</label>
      <label>Low light threshold <span id="lowValue" class="value">900</span></label>
      <input id="low" type="range" min="0" max="4095" value="900">
      <label>High light threshold <span id="highValue" class="value">3300</span></label>
      <input id="high" type="range" min="0" max="4095" value="3300">
      <label>Particles <span id="particlesValue" class="value">48</span></label>
      <input id="particles" type="range" min="8" max="64" value="48">
      <label>PIC / FLIP <span id="flipValue" class="value">78%</span></label>
      <input id="flip" type="range" min="0" max="100" value="78">
      <label class="toggle"><input id="separate" type="checkbox" checked> Separate Particles</label>
      <label class="toggle"><input id="compensate" type="checkbox" checked> Compensate Drift</label>
    </div>

    <div class="card">
      <div class="metrics">
        <div class="metric">VBUS<b id="vbus">-- V</b></div>
        <div class="metric">Power<b id="power">-- mA</b></div>
        <div class="metric">Temp<b id="temp">-- C</b></div>
        <div class="metric">Humidity<b id="hum">-- %</b></div>
        <div class="metric">Light<b id="ldr">--</b></div>
        <div class="metric">Bright Cap<b id="cap">--</b></div>
        <div class="metric">Audio<b id="audio">--</b></div>
        <div class="metric">Audio Band<b id="audioBand">--</b></div>
        <div class="metric">Beat<b id="audioBeat">--</b></div>
        <div class="metric">MIC ADC<b id="mic">--</b></div>
        <div class="metric">Audio Src<b id="audioSrc">--</b></div>
        <div class="metric">ADC Counts<b id="adcCounts">--</b></div>
        <div class="metric">Accel<b id="accel">--</b></div>
        <div class="metric">MPU<b id="mpu">--</b></div>
        <div class="metric">Frames<b id="frames">--</b></div>
      </div>
    </div>

    <div class="card">
      <div class="bars" id="bars"></div>
    </div>
  </section>
</main>
<script>
const modes = ["Clock", "Spectrum", "Fluid", "Text", "Timer"];
const clockThemes = ["Classic", "Rainbow", "Breath", "Night", "Minimal"];
const audioVisualModes = ["Spectrum", "Mirror Spectrum", "VU Meter", "Bass Pulse", "Fire Spectrum", "Center Burst"];
const timerModes = ["Pomodoro", "Countdown", "Stopwatch"];
const timerStates = ["Idle", "Running", "Paused", "Finished"];
const bars = document.getElementById("bars");
for (let i = 0; i < 32; i++) {
  const el = document.createElement("div");
  el.className = "bar";
  bars.appendChild(el);
}
const q = id => document.getElementById(id);
let state = {};
let scrollTextTimer = 0;
let pendingScrollText = null;
let queuedScrollText = null;
let scrollTextInFlight = false;
function colorToRgb(hex) {
  const n = parseInt(hex.slice(1), 16);
  return { r:(n >> 16) & 255, g:(n >> 8) & 255, b:n & 255 };
}
function rgbToHex(c) {
  return "#" + [c.r, c.g, c.b].map(v => Math.max(0, Math.min(255, v)).toString(16).padStart(2, "0")).join("");
}
function formatSeconds(total) {
  total = Math.max(0, Math.floor(total || 0));
  const m = Math.floor(total / 60);
  const s = total % 60;
  return String(m).padStart(2, "0") + ":" + String(s).padStart(2, "0");
}
async function sendPatch(patch, refreshAfter = true) {
  await fetch("/api/control", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify(patch) });
  if (refreshAfter) await refresh();
}
function sendScrollTextNow(value) {
  clearTimeout(scrollTextTimer);
  scrollTextTimer = 0;
  queuedScrollText = value;
  return flushScrollText();
}
async function flushScrollText() {
  if (scrollTextInFlight) return;
  scrollTextInFlight = true;
  try {
    while (queuedScrollText !== null) {
      const value = queuedScrollText;
      queuedScrollText = null;
      pendingScrollText = value;
      await sendPatch({ scrollText:value }, false);
    }
  } finally {
    scrollTextInFlight = false;
    pendingScrollText = null;
  }
}
function queueScrollText(value) {
  pendingScrollText = value;
  clearTimeout(scrollTextTimer);
  scrollTextTimer = setTimeout(() => sendScrollTextNow(value), 250);
}
document.querySelectorAll("button[data-mode]").forEach(btn => btn.onclick = () => sendPatch({ mode:Number(btn.dataset.mode) }));
q("color").oninput = e => sendPatch({ color:colorToRgb(e.target.value) });
q("clockTheme").onchange = e => sendPatch({ clockTheme:Number(e.target.value) });
q("audioVisualMode").onchange = e => sendPatch({ audioVisualMode:Number(e.target.value), mode:1 });
q("showSpectrumMode").onclick = () => sendPatch({ mode:1 });
q("audioSensitivity").oninput = e => q("audioSensitivityValue").textContent = e.target.value;
q("audioSensitivity").onchange = e => sendPatch({ audioSensitivity:Number(e.target.value) });
q("audioSmoothing").oninput = e => q("audioSmoothingValue").textContent = e.target.value;
q("audioSmoothing").onchange = e => sendPatch({ audioSmoothing:Number(e.target.value) });
q("audioRainbow").onchange = e => sendPatch({ audioRainbow:e.target.checked });
q("audioBeatFlash").onchange = e => sendPatch({ audioBeatFlash:e.target.checked });
q("scrollText").oninput = e => queueScrollText(e.target.value);
q("scrollText").onchange = e => sendScrollTextNow(e.target.value);
q("scrollText").onblur = e => sendScrollTextNow(e.target.value);
q("scrollText").oncompositionend = e => queueScrollText(e.target.value);
q("scrollText").onkeydown = e => { if (e.key === "Enter") sendScrollTextNow(e.target.value); };
q("applyScrollText").onclick = () => sendScrollTextNow(q("scrollText").value);
q("scrollSpeed").oninput = e => q("scrollSpeedValue").textContent = e.target.value + " ms";
q("scrollSpeed").onchange = e => sendPatch({ scrollSpeedMs:Number(e.target.value) });
q("scrollRainbow").onchange = e => sendPatch({ scrollRainbow:e.target.checked });
q("timerMode").onchange = e => sendPatch({ timerMode:Number(e.target.value) });
q("setCountdown").onclick = () => sendPatch({ timerMinutes:Number(q("timerMinutes").value), mode:4 });
q("pomodoroFocusMin").onchange = e => sendPatch({ pomodoroFocusMin:Number(e.target.value) });
q("pomodoroBreakMin").onchange = e => sendPatch({ pomodoroBreakMin:Number(e.target.value) });
q("timerStart").onclick = () => sendPatch({ timerAction:"start", mode:4 });
q("timerPause").onclick = () => sendPatch({ timerAction:"pause", mode:4 });
q("timerResume").onclick = () => sendPatch({ timerAction:"resume", mode:4 });
q("timerReset").onclick = () => sendPatch({ timerAction:"reset", mode:4 });
q("brightness").oninput = e => { q("brightnessValue").textContent = e.target.value; };
q("brightness").onchange = e => sendPatch({ manualBrightness:Number(e.target.value) });
q("autoBrightness").onchange = e => sendPatch({ autoBrightness:e.target.checked });
q("saveSettings").onclick = async () => {
  await fetch("/api/save-settings", { method:"POST" });
  await refresh();
};
q("resetSettings").onclick = async () => {
  if (!confirm("Restore default settings?")) return;
  await fetch("/api/reset-settings", { method:"POST" });
  await refresh();
};
q("low").oninput = e => q("lowValue").textContent = e.target.value;
q("low").onchange = e => sendPatch({ lowLightThreshold:Number(e.target.value) });
q("high").oninput = e => q("highValue").textContent = e.target.value;
q("high").onchange = e => sendPatch({ highLightThreshold:Number(e.target.value) });
q("particles").oninput = e => q("particlesValue").textContent = e.target.value;
q("particles").onchange = e => sendPatch({ fluidParticles:Number(e.target.value) });
q("flip").oninput = e => q("flipValue").textContent = e.target.value + "%";
q("flip").onchange = e => sendPatch({ fluidFlipRatio:Number(e.target.value) / 100 });
q("separate").onchange = e => sendPatch({ fluidSeparateParticles:e.target.checked });
q("compensate").onchange = e => sendPatch({ fluidCompensateDrift:e.target.checked });
async function refresh() {
  const res = await fetch("/api/state");
  state = await res.json();
  document.querySelectorAll("button[data-mode]").forEach(btn => btn.classList.toggle("active", Number(btn.dataset.mode) === state.mode));
  q("modeName").textContent = modes[state.mode] || "Clock";
  q("clock").textContent = state.time || "--:--:--";
  q("color").value = rgbToHex(state.color);
  q("clockTheme").value = state.clockTheme ?? 0;
  q("clockTheme").title = clockThemes[state.clockTheme] || "Classic";
  q("audioVisualMode").value = state.audioVisualMode ?? 0;
  q("audioVisualMode").title = audioVisualModes[state.audioVisualMode] || "Spectrum";
  q("audioSensitivity").value = state.audioSensitivity;
  q("audioSensitivityValue").textContent = state.audioSensitivity;
  q("audioSmoothing").value = state.audioSmoothing;
  q("audioSmoothingValue").textContent = state.audioSmoothing;
  q("audioRainbow").checked = state.audioRainbow;
  q("audioBeatFlash").checked = state.audioBeatFlash;
  if (document.activeElement !== q("scrollText") && pendingScrollText === null && queuedScrollText === null && !scrollTextInFlight) q("scrollText").value = state.scrollText || "";
  q("scrollSpeed").value = state.scrollSpeedMs;
  q("scrollSpeedValue").textContent = state.scrollSpeedMs + " ms";
  q("scrollRainbow").checked = state.scrollRainbow;
  q("timerMode").value = state.timerMode ?? 0;
  if (document.activeElement !== q("timerMinutes")) q("timerMinutes").value = state.timerDurationSec ? Math.max(1, Math.round(state.timerDurationSec / 60)) : 5;
  if (document.activeElement !== q("pomodoroFocusMin")) q("pomodoroFocusMin").value = state.pomodoroFocusMin;
  if (document.activeElement !== q("pomodoroBreakMin")) q("pomodoroBreakMin").value = state.pomodoroBreakMin;
  const timerValue = state.timerMode === 2 ? state.stopwatchElapsedSec : state.timerRemainingSec;
  q("timerStatus").textContent = formatSeconds(timerValue) + " " + (timerStates[state.timerState] || "Idle");
  q("timerStatus").title = (timerModes[state.timerMode] || "Pomodoro") + (state.pomodoroIsBreak ? " Break" : " Focus");
  q("brightness").value = state.manualBrightness;
  q("brightnessValue").textContent = state.manualBrightness;
  q("autoBrightness").checked = state.autoBrightness;
  q("low").value = state.lowLightThreshold;
  q("lowValue").textContent = state.lowLightThreshold;
  q("high").value = state.highLightThreshold;
  q("highValue").textContent = state.highLightThreshold;
  q("particles").value = state.fluidParticles;
  q("particlesValue").textContent = state.fluidParticles;
  q("flip").value = Math.round(state.fluidFlipRatio * 100);
  q("flipValue").textContent = Math.round(state.fluidFlipRatio * 100) + "%";
  q("separate").checked = state.fluidSeparateParticles;
  q("compensate").checked = state.fluidCompensateDrift;
  q("vbus").textContent = state.vbus.toFixed(2) + " V";
  q("power").textContent = state.maxMilliamps + " mA";
  q("cap").textContent = state.brightnessCap + " / 255";
  q("temp").textContent = Number.isFinite(state.temperatureC) ? state.temperatureC.toFixed(1) + " C" : "--";
  q("hum").textContent = Number.isFinite(state.humidityRh) ? state.humidityRh.toFixed(0) + " %" : "--";
  q("ldr").textContent = state.rawLdr + " / " + state.adaptiveBrightness;
  q("audio").textContent = state.rms.toFixed(4) + " / " + state.peak.toFixed(4);
  q("audioBand").textContent = state.audioLowEnergy.toFixed(3) + " / " + state.audioMidEnergy.toFixed(3) + " / " + state.audioHighEnergy.toFixed(3);
  q("audioBeat").textContent = state.audioBeat ? "yes" : "no";
  q("mic").textContent = state.micRaw + " [" + state.micMin + "-" + state.micMax + "] / " + state.micBias.toFixed(0);
  q("audioSrc").textContent = state.usingAnalogFallback ? "analogRead" : "ADC DMA";
  q("adcCounts").textContent = state.dmaReadCount + " / " + state.micSampleCount + " / " + state.vbusSampleCount + " / " + state.ldrSampleCount;
  q("accel").textContent = state.accelX.toFixed(2) + ", " + state.accelY.toFixed(2) + ", " + state.accelZ.toFixed(2);
  q("mpu").textContent = (state.mpuOnline ? "online" : "offline") + " " + state.mpuReadCount + "/" + state.mpuFailCount;
  q("frames").textContent = state.audioFrames;
  [...bars.children].forEach((bar, i) => bar.style.height = Math.max(3, ((state.smoothSpectrum?.[i] ?? state.spectrum?.[i]) || 0) * 84) + "px");
}
setInterval(refresh, 1000);
refresh();
</script>
</body>
</html>
)HTML";

String modeToString(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::Spectrum:
      return "spectrum";
    case DisplayMode::Fluid:
      return "fluid";
    case DisplayMode::Text:
      return "text";
    case DisplayMode::Timer:
      return "timer";
    case DisplayMode::Weather:
      return "weather";
    case DisplayMode::Game:
      return "game";
    case DisplayMode::Clock:
    default:
      return "clock";
  }
}

void copyScrollText(char *dest, const char *src) {
  if (!src) {
    src = "";
  }

  size_t out = 0;
  while (src[0] != '\0' && out < AppConfig::ScrollTextMaxLen - 1) {
    const char c = *src++;
    dest[out++] = (c >= 0x20 && c <= 0x7E) ? c : ' ';
  }
  dest[out] = '\0';
}

void copyWeatherCity(char *dest, const char *src) {
  if (!src) {
    src = "";
  }

  size_t out = 0;
  while (src[0] != '\0' && out < WeatherCityMaxLen - 1) {
    const uint8_t c = static_cast<uint8_t>(*src++);
    dest[out++] = c >= 0x20 ? static_cast<char>(c) : ' ';
  }
  dest[out] = '\0';
}

uint16_t matrixIndex(uint8_t x, uint8_t y) {
  const uint16_t columnStart = x * AppConfig::MatrixHeight;
  if (x & 0x01) {
    return columnStart + (AppConfig::MatrixHeight - 1 - y);
  }
  return columnStart + y;
}

uint16_t frontViewMatrixIndex(uint8_t x, uint8_t y) {
  const uint8_t physicalX = AppConfig::MatrixWidth - 1 - x;
  const uint8_t physicalY = AppConfig::MatrixHeight - 1 - y;
  return matrixIndex(physicalX, physicalY);
}

void addControlConfig(JsonDocument &doc, const ControlState &control) {
  doc["mode"] = static_cast<uint8_t>(control.mode);
  doc["clockTheme"] = static_cast<uint8_t>(control.clockTheme);
  doc["manualBrightness"] = control.manualBrightness;
  doc["brightness"] = control.manualBrightness;
  doc["autoBrightness"] = control.autoBrightness;
  doc["lowLightThreshold"] = control.lowLightThreshold;
  doc["highLightThreshold"] = control.highLightThreshold;
  doc["fluidParticles"] = control.fluidParticles;
  doc["fluidFlipRatio"] = control.fluidFlipRatio;
  doc["fluidSeparateParticles"] = control.fluidSeparateParticles;
  doc["fluidCompensateDrift"] = control.fluidCompensateDrift;
  doc["scrollText"] = control.scrollText;
  doc["scrollSpeedMs"] = control.scrollSpeedMs;
  doc["scrollRainbow"] = control.scrollRainbow;
  doc["timerMode"] = static_cast<uint8_t>(control.timerMode);
  doc["timerDurationSec"] = control.timerDurationSec;
  doc["pomodoroFocusMin"] = control.pomodoroFocusMin;
  doc["pomodoroBreakMin"] = control.pomodoroBreakMin;
  doc["audioVisualMode"] = static_cast<uint8_t>(control.audioVisualMode);
  doc["audioSensitivity"] = control.audioSensitivity;
  doc["audioSmoothing"] = control.audioSmoothing;
  doc["audioRainbow"] = control.audioRainbow;
  doc["audioBeatFlash"] = control.audioBeatFlash;
  doc["audioAutoGain"] = control.audioAutoGain;
  doc["smoothTransitions"] = control.smoothTransitions;
  doc["transitionStyle"] = static_cast<uint8_t>(control.transitionStyle);
  doc["transitionDurationMs"] = control.transitionDurationMs;
  doc["gammaCorrection"] = control.gammaCorrection;
  doc["smartScenes"] = control.smartScenes;
  doc["quietStartHour"] = control.quietStartHour;
  doc["quietEndHour"] = control.quietEndHour;
  doc["nightBrightnessCap"] = control.nightBrightnessCap;
  doc["weatherEnabled"] = control.weatherEnabled;
  doc["weatherDisplayMode"] = static_cast<uint8_t>(control.weatherDisplayMode);
  doc["weatherCity"] = control.weatherCity;
  doc["weatherLatitude"] = control.weatherLatitude;
  doc["weatherLongitude"] = control.weatherLongitude;
  doc["weatherAutoLocate"] = control.weatherAutoLocate;
  doc["weatherUpdateIntervalMin"] = control.weatherUpdateIntervalMin;
  doc["gameType"] = static_cast<uint8_t>(control.gameType);
  doc["gameUseMpuControl"] = control.gameUseMpuControl;
  doc["gameSpeedMs"] = control.gameSpeedMs;
  doc["color"]["r"] = control.preferredColor.r;
  doc["color"]["g"] = control.preferredColor.g;
  doc["color"]["b"] = control.preferredColor.b;
}

void sendConfigJson(AsyncWebServerRequest *request) {
  RenderState state = copySharedState();
  JsonDocument doc;
  addControlConfig(doc, state.control);

  String body;
  serializeJsonPretty(doc, body);
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", body);
  response->addHeader("Content-Disposition", "attachment; filename=\"pixel-clock-config.json\"");
  request->send(response);
}

void sendScreenJson(AsyncWebServerRequest *request) {
  const ScreenSnapshot screen = copyScreenSnapshot();
  JsonDocument doc;
  doc["width"] = AppConfig::MatrixWidth;
  doc["height"] = AppConfig::MatrixHeight;
  doc["frameCounter"] = screen.frameCounter;
  doc["orientation"] = "front";

  JsonArray pixels = doc["pixels"].to<JsonArray>();
  for (uint8_t y = 0; y < AppConfig::MatrixHeight; ++y) {
    for (uint8_t x = 0; x < AppConfig::MatrixWidth; ++x) {
      const uint16_t idx = frontViewMatrixIndex(x, y);
      JsonArray pixel = pixels.add<JsonArray>();
      pixel.add(screen.rgb[idx][0]);
      pixel.add(screen.rgb[idx][1]);
      pixel.add(screen.rgb[idx][2]);
    }
  }

  String body;
  serializeJson(doc, body);
  request->send(200, "application/json", body);
}

void rebootTask(void *) {
  vTaskDelay(pdMS_TO_TICKS(350));
  ESP.restart();
}

void wifiResetTask(void *) {
  vTaskDelay(pdMS_TO_TICKS(350));
  resetWiFiSettingsAndRestart();
}

void handleWiFiConnectBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }

  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  const char *ssid = doc["ssid"] | "";
  const char *password = doc["password"] | "";
  if (!saveWiFiCredentialsAndConnect(ssid, password)) {
    request->send(400, "application/json", "{\"error\":\"ssid required\"}");
    return;
  }

  request->send(200, "application/json", getWiFiStatusJson());
}

String buildStateJson() {
  RenderState state = copySharedState();
  JsonDocument doc;

  doc["mode"] = static_cast<uint8_t>(state.control.mode);
  doc["firmwareVersion"] = AppConfig::FirmwareVersion;
  doc["modeName"] = modeToString(state.control.mode);
  doc["clockTheme"] = static_cast<uint8_t>(state.control.clockTheme);
  doc["scrollText"] = state.control.scrollText;
  doc["scrollSpeedMs"] = state.control.scrollSpeedMs;
  doc["scrollRainbow"] = state.control.scrollRainbow;
  doc["audioVisualMode"] = static_cast<uint8_t>(state.control.audioVisualMode);
  doc["audioSensitivity"] = state.control.audioSensitivity;
  doc["audioSmoothing"] = state.control.audioSmoothing;
  doc["audioRainbow"] = state.control.audioRainbow;
  doc["audioBeatFlash"] = state.control.audioBeatFlash;
  doc["audioAutoGain"] = state.control.audioAutoGain;
  doc["smoothTransitions"] = state.control.smoothTransitions;
  doc["transitionStyle"] = static_cast<uint8_t>(state.control.transitionStyle);
  doc["transitionDurationMs"] = state.control.transitionDurationMs;
  doc["gammaCorrection"] = state.control.gammaCorrection;
  doc["smartScenes"] = state.control.smartScenes;
  doc["deskAiEnabled"] = state.control.deskAiEnabled;
  doc["deskAiAutoScene"] = state.control.deskAiAutoScene;
  doc["deskAiActiveLearning"] = state.control.deskAiActiveLearning;
  doc["deskAiValidationLocked"] = state.control.deskAiValidationLocked;
  doc["deskAiFeedbackThreshold"] = state.control.deskAiFeedbackThreshold;
  doc["competitionDemoMode"] = state.control.competitionDemoMode;
  doc["energyAwareMode"] = state.control.energyAwareMode;
  doc["quietStartHour"] = state.control.quietStartHour;
  doc["quietEndHour"] = state.control.quietEndHour;
  doc["nightBrightnessCap"] = state.control.nightBrightnessCap;
  doc["effectiveMode"] = static_cast<uint8_t>(state.context.effectiveMode);
  doc["sceneReason"] = static_cast<uint8_t>(state.context.reason);
  doc["quietHours"] = state.context.quietHours;
  doc["darkEnvironment"] = state.context.darkEnvironment;
  doc["audioActive"] = state.context.audioActive;
  doc["motionActive"] = state.context.motionActive;
  doc["automaticSwitchCount"] = state.context.automaticSwitchCount;
  doc["notificationActive"] = state.notifications.activeVisible;
  doc["notificationQueueCount"] = state.notifications.count;
  doc["notificationText"] = state.notifications.active.text;
  doc["weatherEnabled"] = state.control.weatherEnabled;
  doc["weatherDisplayMode"] = static_cast<uint8_t>(state.control.weatherDisplayMode);
  doc["weatherCity"] = state.control.weatherCity;
  doc["weatherLatitude"] = state.control.weatherLatitude;
  doc["weatherLongitude"] = state.control.weatherLongitude;
  doc["weatherUpdateIntervalMin"] = state.control.weatherUpdateIntervalMin;
  doc["gameType"] = static_cast<uint8_t>(state.control.gameType);
  doc["gameUseMpuControl"] = state.control.gameUseMpuControl;
  doc["gameSpeedMs"] = state.control.gameSpeedMs;
  const uint32_t nowMs = millis();
  doc["timerMode"] = static_cast<uint8_t>(state.control.timerMode);
  doc["timerState"] = static_cast<uint8_t>(state.control.timerState);
  doc["timerDurationSec"] = state.control.timerDurationSec;
  doc["timerRemainingSec"] = timerRemainingSec(state.control, nowMs);
  doc["stopwatchElapsedSec"] = stopwatchElapsedSec(state.control, nowMs);
  doc["pomodoroFocusMin"] = state.control.pomodoroFocusMin;
  doc["pomodoroBreakMin"] = state.control.pomodoroBreakMin;
  doc["pomodoroIsBreak"] = state.control.pomodoroIsBreak;
  doc["manualBrightness"] = state.control.manualBrightness;
  doc["brightness"] = state.control.manualBrightness;
  doc["autoBrightness"] = state.control.autoBrightness;
  uint8_t effectiveBrightness = state.control.autoBrightness
                                    ? state.environment.adaptiveBrightness
                                    : state.control.manualBrightness;
  effectiveBrightness = min(effectiveBrightness, state.power.brightnessCap);
  if (state.control.smartScenes &&
      (state.context.quietHours || state.context.darkEnvironment)) {
    effectiveBrightness = min(effectiveBrightness, state.control.nightBrightnessCap);
  }
  if (state.control.energyAwareMode && state.deskAi.state == DeskState::Away) {
    effectiveBrightness = min(effectiveBrightness, static_cast<uint8_t>(4));
  }
  doc["effectiveBrightness"] = effectiveBrightness;
  doc["lowLightThreshold"] = state.control.lowLightThreshold;
  doc["highLightThreshold"] = state.control.highLightThreshold;
  doc["fluidParticles"] = state.control.fluidParticles;
  doc["fluidFlipRatio"] = state.control.fluidFlipRatio;
  doc["fluidSeparateParticles"] = state.control.fluidSeparateParticles;
  doc["fluidCompensateDrift"] = state.control.fluidCompensateDrift;
  doc["color"]["r"] = state.control.preferredColor.r;
  doc["color"]["g"] = state.control.preferredColor.g;
  doc["color"]["b"] = state.control.preferredColor.b;

  struct tm tmNow = {};
  localtime_r(&state.unixTime, &tmNow);
  char timeBuf[16] = "--:--:--";
  strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmNow);
  doc["time"] = timeBuf;

  doc["temperatureC"] = state.environment.temperatureC;
  doc["humidityRh"] = state.environment.humidityRh;
  doc["rawLdr"] = state.environment.rawLdr;
  doc["adaptiveBrightness"] = state.environment.adaptiveBrightness;
  doc["accelX"] = state.environment.accelX;
  doc["accelY"] = state.environment.accelY;
  doc["accelZ"] = state.environment.accelZ;
  doc["mpuOnline"] = state.environment.mpuOnline;
  doc["mpuReadCount"] = state.environment.mpuReadCount;
  doc["mpuFailCount"] = state.environment.mpuFailCount;
  doc["vbus"] = state.power.vbus;
  doc["highPower"] = state.power.highPower;
  doc["dcdcEnabled"] = state.power.dcdcEnabled;
  doc["maxMilliamps"] = state.power.maxMilliamps;
  doc["brightnessCap"] = state.power.brightnessCap;
  doc["rms"] = state.audio.rms;
  doc["peak"] = state.audio.peak;
  doc["audioLowEnergy"] = state.audio.lowEnergy;
  doc["audioMidEnergy"] = state.audio.midEnergy;
  doc["audioHighEnergy"] = state.audio.highEnergy;
  doc["audioEnergy"] = state.audio.energy;
  doc["audioEnergyAvg"] = state.audio.energyAvg;
  doc["audioNoiseFloor"] = state.audio.noiseFloor;
  doc["audioAutoGainValue"] = state.audio.autoGain;
  doc["audioSignalPresent"] = state.audio.signalPresent;
  doc["audioBeat"] = state.audio.beat;
  doc["micRaw"] = state.audio.micRaw;
  doc["micMin"] = state.audio.micMin;
  doc["micMax"] = state.audio.micMax;
  doc["micBias"] = state.audio.micBias;
  doc["audioFrames"] = state.audio.frameCounter;
  doc["dmaReadCount"] = state.audio.dmaReadCount;
  doc["dmaTimeoutCount"] = state.audio.dmaTimeoutCount;
  doc["micSampleCount"] = state.audio.micSampleCount;
  doc["vbusSampleCount"] = state.audio.vbusSampleCount;
  doc["ldrSampleCount"] = state.audio.ldrSampleCount;
  doc["usingAnalogFallback"] = state.audio.usingAnalogFallback;
  const uint32_t uptimeMs = millis();
  const uint32_t freeHeap = ESP.getFreeHeap();
  const wifi_mode_t mode = WiFi.getMode();
  const bool wifiApMode = mode == WIFI_AP || mode == WIFI_AP_STA;
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  const String wifiMode = wifiConnected ? (wifiApMode ? "AP+STA" : "STA") : (wifiApMode ? "AP" : "offline");
  const int32_t wifiRssi = wifiConnected ? WiFi.RSSI() : 0;
  const String ipAddress = wifiConnected ? WiFi.localIP().toString() : (wifiApMode ? WiFi.softAPIP().toString() : "0.0.0.0");
  const String wifiSsid = wifiConnected ? WiFi.SSID() : "";

  doc["uptimeMs"] = uptimeMs;
  doc["freeHeap"] = freeHeap;
  doc["wifiMode"] = wifiMode;
  doc["wifiRssi"] = wifiRssi;
  doc["wifiSsid"] = wifiSsid;
  doc["wifiConnected"] = wifiConnected;
  doc["ip"] = ipAddress;

  JsonObject power = doc["power"].to<JsonObject>();
  power["vbus"] = state.power.vbus;
  power["highPower"] = state.power.highPower;
  power["dcdcEnabled"] = state.power.dcdcEnabled;
  power["maxMilliamps"] = state.power.maxMilliamps;
  power["brightnessCap"] = state.power.brightnessCap;

  JsonObject audio = doc["audio"].to<JsonObject>();
  audio["rms"] = state.audio.rms;
  audio["peak"] = state.audio.peak;
  audio["lowEnergy"] = state.audio.lowEnergy;
  audio["midEnergy"] = state.audio.midEnergy;
  audio["highEnergy"] = state.audio.highEnergy;
  audio["noiseFloor"] = state.audio.noiseFloor;
  audio["autoGain"] = state.audio.autoGain;
  audio["signalPresent"] = state.audio.signalPresent;
  audio["beat"] = state.audio.beat;
  audio["micRaw"] = state.audio.micRaw;
  audio["micMin"] = state.audio.micMin;
  audio["micMax"] = state.audio.micMax;
  audio["micBias"] = state.audio.micBias;
  audio["frames"] = state.audio.frameCounter;
  audio["source"] = state.audio.usingAnalogFallback ? "analogRead" : "ADC DMA";

  JsonObject environment = doc["environment"].to<JsonObject>();
  environment["temperatureC"] = state.environment.temperatureC;
  environment["temperature"] = state.environment.temperatureC;
  environment["humidityRh"] = state.environment.humidityRh;
  environment["humidity"] = state.environment.humidityRh;
  environment["rawLdr"] = state.environment.rawLdr;
  environment["ldr"] = state.environment.rawLdr;
  environment["adaptiveBrightness"] = state.environment.adaptiveBrightness;
  environment["accelX"] = state.environment.accelX;
  environment["accelY"] = state.environment.accelY;
  environment["accelZ"] = state.environment.accelZ;
  environment["mpuOnline"] = state.environment.mpuOnline;
  environment["mpuReadCount"] = state.environment.mpuReadCount;
  environment["mpuFailCount"] = state.environment.mpuFailCount;

  JsonObject system = doc["system"].to<JsonObject>();
  system["uptimeMs"] = uptimeMs;
  system["freeHeap"] = freeHeap;
  system["wifiMode"] = wifiMode;
  system["wifiRssi"] = wifiRssi;
  system["ip"] = ipAddress;

  JsonObject network = doc["network"].to<JsonObject>();
  network["mode"] = wifiMode;
  network["ssid"] = wifiSsid;
  network["ip"] = ipAddress;
  network["rssi"] = wifiRssi;
  network["connected"] = wifiConnected;
  network["setupAp"] = wifiApMode;
  network["apIp"] = wifiApMode ? WiFi.softAPIP().toString() : "";
  network["setupApSsid"] = "PixelClock-Setup";
  network["hostname"] = AppConfig::Hostname;

  power["currentLimit"] = state.power.maxMilliamps;

  JsonObject weather = doc["weather"].to<JsonObject>();
  weather["enabled"] = state.control.weatherEnabled;
  weather["displayMode"] = static_cast<uint8_t>(state.control.weatherDisplayMode);
  weather["city"] = state.control.weatherCity;
  weather["latitude"] = state.control.weatherLatitude;
  weather["longitude"] = state.control.weatherLongitude;
  weather["autoLocate"] = state.control.weatherAutoLocate;
  weather["updateIntervalMin"] = state.control.weatherUpdateIntervalMin;
  weather["online"] = state.weather.online;
  weather["hasData"] = state.weather.hasData;
  weather["temperature"] = state.weather.temperature;
  weather["apparentTemperature"] = state.weather.apparentTemperature;
  weather["humidity"] = state.weather.relativeHumidity;
  weather["weatherCode"] = state.weather.weatherCode;
  weather["precipitation"] = state.weather.precipitation;
  weather["cloudCover"] = state.weather.cloudCover;
  weather["windSpeed"] = state.weather.windSpeed;
  weather["todayTempMax"] = state.weather.todayTempMax;
  weather["todayTempMin"] = state.weather.todayTempMin;
  weather["todayPrecipProb"] = state.weather.todayPrecipProb;
  weather["lastUpdateMs"] = state.weather.lastUpdateMs;
  weather["lastSuccessMs"] = state.weather.lastSuccessMs;
  weather["failCount"] = state.weather.failCount;
  weather["lastError"] = state.weather.lastError;

  JsonObject game = doc["game"].to<JsonObject>();
  game["type"] = static_cast<uint8_t>(state.game.type);
  game["runState"] = static_cast<uint8_t>(state.game.runState);
  game["score"] = state.game.score;
  game["highScore"] = state.game.highScore;
  game["speedMs"] = state.control.gameSpeedMs;
  game["useMpuControl"] = state.control.gameUseMpuControl;
  game["snakeLen"] = state.game.snakeLen;
  game["foodX"] = state.game.food.x;
  game["foodY"] = state.game.food.y;
  game["ballX"] = state.game.ball.x;
  game["ballY"] = state.game.ball.y;
  game["breakoutPaddleX"] = state.game.breakoutPaddleX;
  game["breakoutBricks"] = state.game.breakoutBricks;
  game["pongPaddleX"] = state.game.pongPaddleX;
  game["reactionReady"] = state.game.reactionReady;

  JsonObject deskAi = doc["deskAi"].to<JsonObject>();
  deskAi["enabled"] = state.control.deskAiEnabled;
  deskAi["autoScene"] = state.control.deskAiAutoScene;
  deskAi["state"] = static_cast<uint8_t>(state.deskAi.state);
  deskAi["label"] = deskStateToString(state.deskAi.state);
  deskAi["confidence"] = state.deskAi.confidence;
  deskAi["baselineState"] = static_cast<uint8_t>(state.deskAi.baselineState);
  deskAi["baselineLabel"] = deskStateToString(state.deskAi.baselineState);
  deskAi["baselineConfidence"] = state.deskAi.baselineConfidence;
  deskAi["quantizedState"] = static_cast<uint8_t>(state.deskAi.quantizedState);
  deskAi["quantizedLabel"] = deskStateToString(state.deskAi.quantizedState);
  deskAi["quantizedConfidence"] = state.deskAi.quantizedConfidence;
  deskAi["inferenceCount"] = state.deskAi.inferenceCount;
  deskAi["lastInferenceMs"] = state.deskAi.lastInferenceMs;
  deskAi["inferenceMicros"] = state.deskAi.inferenceMicros;
  deskAi["quantizedInferenceMicros"] = state.deskAi.quantizedInferenceMicros;
  deskAi["lastCalibrationMs"] = state.deskAi.lastCalibrationMs;
  deskAi["lastCalibrationLabel"] = deskStateToString(state.deskAi.lastCalibrationLabel);
  deskAi["lastInferenceOffline"] = state.deskAi.lastInferenceOffline;
  deskAi["offlineInferenceCount"] = state.deskAi.offlineInferenceCount;
  deskAi["lastOfflineInferenceMs"] = state.deskAi.lastOfflineInferenceMs;
  deskAi["profileCoverage"] = state.deskAi.profileCoverage;
  deskAi["profileQuality"] = state.deskAi.profileQuality;
  deskAi["profileReady"] = state.deskAi.profileReady;
  deskAi["centroidSeparation"] = state.deskAi.centroidSeparation;
  deskAi["activeLearning"] = state.control.deskAiActiveLearning;
  deskAi["validationLocked"] = state.control.deskAiValidationLocked;
  deskAi["feedbackThreshold"] = state.control.deskAiFeedbackThreshold;
  deskAi["feedbackRequested"] = state.deskAi.feedbackRequested;
  deskAi["feedbackSuggestedState"] = static_cast<uint8_t>(state.deskAi.feedbackSuggestedState);
  deskAi["feedbackSuggestedLabel"] = deskStateToString(state.deskAi.feedbackSuggestedState);
  deskAi["feedbackRequestedMs"] = state.deskAi.feedbackRequestedMs;
  deskAi["feedbackRequestCount"] = state.deskAi.feedbackRequestCount;
  deskAi["feedbackResolvedCount"] = state.deskAi.feedbackResolvedCount;
  deskAi["demoActive"] = state.deskAi.demoActive;
  char fingerprint[9] = {};
  snprintf(fingerprint, sizeof(fingerprint), "%08lX",
           static_cast<unsigned long>(state.deskAi.modelFingerprint));
  deskAi["modelFingerprint"] = fingerprint;
  deskAi["minSamplesPerClass"] = AppConfig::DeskAiMinCalibrationSamplesPerClass;
  deskAi["recommendedSamplesPerClass"] = AppConfig::DeskAiRecommendedCalibrationSamplesPerClass;
  JsonArray features = deskAi["features"].to<JsonArray>();
  for (float feature : state.deskAi.features) {
    features.add(feature);
  }
  JsonArray scores = deskAi["scores"].to<JsonArray>();
  for (float score : state.deskAi.classScores) {
    scores.add(score);
  }
  JsonArray samples = deskAi["samples"].to<JsonArray>();
  for (uint16_t sample : state.control.deskAiSampleCounts) {
    samples.add(sample);
  }
  JsonObject evaluation = deskAi["evaluation"].to<JsonObject>();
  evaluation["total"] = state.deskAi.evaluationTotal;
  evaluation["personalizedCorrect"] = state.deskAi.personalizedCorrect;
  evaluation["baselineCorrect"] = state.deskAi.baselineCorrect;
  evaluation["quantizedCorrect"] = state.deskAi.quantizedCorrect;
  evaluation["rejectedPredictions"] = state.deskAi.rejectedPredictions;
  evaluation["lastEvaluationMs"] = state.deskAi.lastEvaluationMs;
  JsonObject lastBlind = evaluation["lastBlind"].to<JsonObject>();
  lastBlind["actualState"] = static_cast<uint8_t>(state.deskAi.lastBlindActual);
  lastBlind["actualLabel"] = deskStateToString(state.deskAi.lastBlindActual);
  lastBlind["personalizedState"] = static_cast<uint8_t>(state.deskAi.lastBlindPersonalized);
  lastBlind["personalizedLabel"] = deskStateToString(state.deskAi.lastBlindPersonalized);
  lastBlind["baselineState"] = static_cast<uint8_t>(state.deskAi.lastBlindBaseline);
  lastBlind["baselineLabel"] = deskStateToString(state.deskAi.lastBlindBaseline);
  lastBlind["quantizedState"] = static_cast<uint8_t>(state.deskAi.lastBlindQuantized);
  lastBlind["quantizedLabel"] = deskStateToString(state.deskAi.lastBlindQuantized);
  lastBlind["confidence"] = state.deskAi.lastBlindConfidence;
  lastBlind["recordedMs"] = state.deskAi.lastBlindResultMs;
  JsonArray evaluationSamples = evaluation["samples"].to<JsonArray>();
  for (uint16_t sample : state.deskAi.evaluationSamples) {
    evaluationSamples.add(sample);
  }
  JsonArray confusion = evaluation["confusion"].to<JsonArray>();
  for (size_t actual = 0; actual < DeskAiClassCount; ++actual) {
    JsonArray row = confusion.add<JsonArray>();
    for (size_t predicted = 0; predicted < DeskAiClassCount; ++predicted) {
      row.add(state.deskAi.confusion[actual][predicted]);
    }
  }
  JsonArray timeline = deskAi["timeline"].to<JsonArray>();
  const uint8_t firstTimeline = state.deskAi.timelineCount == DeskAiTimelineCapacity
                                    ? state.deskAi.timelineNext
                                    : 0;
  for (uint8_t entryIndex = 0; entryIndex < state.deskAi.timelineCount; ++entryIndex) {
    const auto &entry = state.deskAi.timeline[(firstTimeline + entryIndex) % DeskAiTimelineCapacity];
    JsonArray point = timeline.add<JsonArray>();
    point.add(entry.timestampMs);
    point.add(static_cast<uint8_t>(entry.state));
    point.add(entry.confidence);
    point.add(entry.offline);
  }

  JsonObject competition = doc["competition"].to<JsonObject>();
  competition["currentFocusMs"] = state.competition.currentFocusMs;
  competition["longestFocusMs"] = state.competition.longestFocusMs;
  competition["focusSessionCount"] = state.competition.focusSessionCount;
  competition["focusInterruptionCount"] = state.competition.focusInterruptionCount;
  competition["stateChangeCount"] = state.competition.stateChangeCount;
  competition["focusScore"] = state.competition.focusScore;
  competition["healthScore"] = state.competition.healthScore;
  competition["audioHealthy"] = state.competition.audioHealthy;
  competition["motionHealthy"] = state.competition.motionHealthy;
  competition["environmentHealthy"] = state.competition.environmentHealthy;
  competition["displayHealthy"] = state.competition.displayHealthy;
  competition["powerHealthy"] = state.competition.powerHealthy;
  competition["wifiHealthy"] = state.competition.wifiHealthy;
  competition["localOnly"] = state.competition.localOnly;
  competition["rawUploadCount"] = state.competition.rawUploadCount;
  competition["cloudInferenceCount"] = state.competition.cloudInferenceCount;
  competition["estimatedCurrentMa"] = state.competition.estimatedCurrentMa;
  competition["estimatedPowerW"] = state.competition.estimatedPowerW;
  competition["estimatedEnergyWh"] = state.competition.estimatedEnergyWh;
  competition["estimatedBaselinePowerW"] = state.competition.estimatedBaselinePowerW;
  competition["estimatedSavedPowerW"] = state.competition.estimatedSavedPowerW;
  competition["estimatedEnergySavedWh"] = state.competition.estimatedEnergySavedWh;
  competition["apiRequestCount"] = state.competition.apiRequestCount;
  competition["externalRequestCount"] = state.competition.externalRequestCount;
  competition["networkBytesReceived"] = state.competition.networkBytesReceived;
  competition["wifiDisconnectCount"] = state.competition.wifiDisconnectCount;
  competition["minFreeHeap"] = state.competition.minFreeHeap;
  competition["resetReason"] = state.competition.resetReason;
  competition["displayFps"] = state.competition.displayFps;
  JsonArray taskStackWatermark = competition["taskStackWatermark"].to<JsonArray>();
  for (uint32_t watermark : state.competition.taskStackWatermark) {
    taskStackWatermark.add(watermark);
  }
  JsonArray stateDurationMs = competition["stateDurationMs"].to<JsonArray>();
  for (uint32_t duration : state.competition.stateDurationMs) {
    stateDurationMs.add(duration);
  }

  JsonArray spectrum = doc["spectrum"].to<JsonArray>();
  for (float bin : state.audio.spectrum) {
    spectrum.add(bin);
  }

  JsonArray smoothSpectrum = doc["smoothSpectrum"].to<JsonArray>();
  for (float bin : state.audio.smoothSpectrum) {
    smoothSpectrum.add(bin);
  }

  String body;
  serializeJson(doc, body);
  return body;
}

void sendStateJson(AsyncWebServerRequest *request) {
  const String body = buildStateJson();
  recordCompetitionApiRequest(body.length());
  request->send(200, "application/json", body);
}

void handleRealtimeEvent(
    AsyncWebSocket *,
    AsyncWebSocketClient *client,
    AwsEventType type,
    void *,
    uint8_t *,
    size_t) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildStateJson());
  }
}

void handleNotificationBody(
    AsyncWebServerRequest *request,
    uint8_t *data,
    size_t len,
    size_t index,
    size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }
  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  if (doc["clear"].is<bool>() && doc["clear"].as<bool>()) {
    clearNotifications();
    request->send(200, "application/json", "{\"ok\":true,\"cleared\":true}");
    return;
  }

  const char *text = doc["text"] | "";
  CRGB color(0xFF, 0xB8, 0x6B);
  if (doc["color"].is<JsonObject>()) {
    JsonObject value = doc["color"];
    color = CRGB(
        constrain(value["r"] | 255, 0, 255),
        constrain(value["g"] | 184, 0, 255),
        constrain(value["b"] | 107, 0, 255));
  } else if (doc["color"].is<JsonArray>()) {
    JsonArray value = doc["color"];
    color = CRGB(
        constrain(value[0] | 255, 0, 255),
        constrain(value[1] | 184, 0, 255),
        constrain(value[2] | 107, 0, 255));
  }

  const uint16_t durationMs = static_cast<uint16_t>(
      constrain(doc["durationMs"] | AppConfig::DefaultNotificationDurationMs, 1200, 30000));
  const uint16_t speedMs = static_cast<uint16_t>(
      constrain(doc["speedMs"] | AppConfig::DefaultNotificationSpeedMs, 30, 240));
  if (!enqueueNotification(text, color, durationMs, speedMs)) {
    request->send(400, "application/json", "{\"error\":\"text required\"}");
    return;
  }
  request->send(200, "application/json", "{\"ok\":true}");
}

void handleControlBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }

  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  RenderState state = copySharedState();
  bool settingsChanged = false;
  bool manualBrightnessChanged = false;
  if (doc["mode"].is<uint8_t>()) {
    const uint8_t mode = doc["mode"];
    if (mode <= static_cast<uint8_t>(DisplayMode::Game)) {
      state.control.mode = static_cast<DisplayMode>(mode);
      settingsChanged = true;
    }
  }
  if (doc["clockTheme"].is<int>()) {
    const int theme = constrain(doc["clockTheme"].as<int>(), 0, static_cast<int>(ClockTheme::Minimal));
    state.control.clockTheme = static_cast<ClockTheme>(theme);
    settingsChanged = true;
  }
  if (doc["manualBrightness"].is<int>()) {
    state.control.manualBrightness =
        static_cast<uint8_t>(constrain(doc["manualBrightness"].as<int>(), 1, 255));
    manualBrightnessChanged = true;
    settingsChanged = true;
  }
  if (doc["brightness"].is<int>()) {
    state.control.manualBrightness = static_cast<uint8_t>(constrain(doc["brightness"].as<int>(), 1, 255));
    manualBrightnessChanged = true;
    settingsChanged = true;
  }
  if (doc["autoBrightness"].is<bool>()) {
    state.control.autoBrightness = doc["autoBrightness"];
    settingsChanged = true;
  } else if (manualBrightnessChanged) {
    state.control.autoBrightness = false;
    settingsChanged = true;
  }
  if (doc["lowLightThreshold"].is<uint16_t>()) {
    state.control.lowLightThreshold = doc["lowLightThreshold"];
    settingsChanged = true;
  }
  if (doc["highLightThreshold"].is<uint16_t>()) {
    state.control.highLightThreshold = doc["highLightThreshold"];
    settingsChanged = true;
  }
  if (doc["fluidParticles"].is<int>()) {
    state.control.fluidParticles = static_cast<uint8_t>(constrain(doc["fluidParticles"].as<int>(), 8, static_cast<int>(AppConfig::FluidParticleCount)));
    settingsChanged = true;
  }
  if (doc["fluidFlipRatio"].is<float>() || doc["fluidFlipRatio"].is<double>()) {
    state.control.fluidFlipRatio = constrain(doc["fluidFlipRatio"].as<float>(), 0.0f, 1.0f);
    settingsChanged = true;
  }
  if (doc["fluidSeparateParticles"].is<bool>()) {
    state.control.fluidSeparateParticles = doc["fluidSeparateParticles"];
    settingsChanged = true;
  }
  if (doc["fluidCompensateDrift"].is<bool>()) {
    state.control.fluidCompensateDrift = doc["fluidCompensateDrift"];
    settingsChanged = true;
  }
  if (doc["scrollText"].is<const char *>()) {
    copyScrollText(state.control.scrollText, doc["scrollText"].as<const char *>());
    settingsChanged = true;
  }
  if (doc["scrollSpeedMs"].is<int>()) {
    state.control.scrollSpeedMs = static_cast<uint16_t>(constrain(doc["scrollSpeedMs"].as<int>(), 30, 500));
    settingsChanged = true;
  }
  if (doc["scrollRainbow"].is<bool>()) {
    state.control.scrollRainbow = doc["scrollRainbow"];
    settingsChanged = true;
  }
  if (doc["audioVisualMode"].is<int>()) {
    const int mode = constrain(doc["audioVisualMode"].as<int>(), 0, static_cast<int>(AudioVisualMode::CenterBurst));
    state.control.audioVisualMode = static_cast<AudioVisualMode>(mode);
    settingsChanged = true;
  }
  if (doc["audioSensitivity"].is<int>()) {
    state.control.audioSensitivity = static_cast<uint8_t>(constrain(doc["audioSensitivity"].as<int>(), 0, 255));
    settingsChanged = true;
  }
  if (doc["audioSmoothing"].is<int>()) {
    state.control.audioSmoothing = static_cast<uint8_t>(constrain(doc["audioSmoothing"].as<int>(), 0, 255));
    settingsChanged = true;
  }
  if (doc["audioRainbow"].is<bool>()) {
    state.control.audioRainbow = doc["audioRainbow"];
    settingsChanged = true;
  }
  if (doc["audioBeatFlash"].is<bool>()) {
    state.control.audioBeatFlash = doc["audioBeatFlash"];
    settingsChanged = true;
  }
  if (doc["audioAutoGain"].is<bool>()) {
    state.control.audioAutoGain = doc["audioAutoGain"];
    settingsChanged = true;
  }
  if (doc["smoothTransitions"].is<bool>()) {
    state.control.smoothTransitions = doc["smoothTransitions"];
    settingsChanged = true;
  }
  if (doc["transitionStyle"].is<int>()) {
    const int style = constrain(
        doc["transitionStyle"].as<int>(),
        0,
        static_cast<int>(TransitionStyle::PixelDissolve));
    state.control.transitionStyle = static_cast<TransitionStyle>(style);
    settingsChanged = true;
  }
  if (doc["transitionDurationMs"].is<int>()) {
    state.control.transitionDurationMs = static_cast<uint16_t>(
        constrain(doc["transitionDurationMs"].as<int>(), 120, 1200));
    settingsChanged = true;
  }
  if (doc["gammaCorrection"].is<bool>()) {
    state.control.gammaCorrection = doc["gammaCorrection"];
    settingsChanged = true;
  }
  if (doc["smartScenes"].is<bool>()) {
    state.control.smartScenes = doc["smartScenes"];
    settingsChanged = true;
  }
  if (doc["deskAiEnabled"].is<bool>()) {
    state.control.deskAiEnabled = doc["deskAiEnabled"];
    settingsChanged = true;
  }
  if (doc["deskAiAutoScene"].is<bool>()) {
    state.control.deskAiAutoScene = doc["deskAiAutoScene"];
    settingsChanged = true;
  }
  if (doc["deskAiActiveLearning"].is<bool>()) {
    state.control.deskAiActiveLearning = doc["deskAiActiveLearning"];
    if (!state.control.deskAiActiveLearning) {
      state.deskAi.feedbackRequested = false;
      state.deskAi.lowConfidenceSinceMs = 0;
    }
    settingsChanged = true;
  }
  if (doc["deskAiValidationLocked"].is<bool>()) {
    const bool requested = doc["deskAiValidationLocked"];
    if (!requested) {
      state.control.deskAiValidationLocked = false;
    } else if (state.deskAi.profileReady && !state.control.competitionDemoMode) {
      state.control.deskAiValidationLocked = true;
      state.deskAi.feedbackRequested = false;
      state.deskAi.lowConfidenceSinceMs = 0;
    }
  }
  if (doc["deskAiFeedbackThreshold"].is<int>()) {
    state.control.deskAiFeedbackThreshold = static_cast<uint8_t>(
        constrain(doc["deskAiFeedbackThreshold"].as<int>(), 25, 85));
    settingsChanged = true;
  }
  if (doc["competitionDemoMode"].is<bool>()) {
    state.control.competitionDemoMode = doc["competitionDemoMode"];
    if (state.control.competitionDemoMode) {
      state.control.deskAiValidationLocked = false;
    }
    state.deskAi.feedbackRequested = false;
    state.deskAi.lowConfidenceSinceMs = 0;
  }
  if (doc["energyAwareMode"].is<bool>()) {
    state.control.energyAwareMode = doc["energyAwareMode"];
    settingsChanged = true;
  }
  if (doc["deskAiCalibration"].is<int>()) {
    const int label = constrain(doc["deskAiCalibration"].as<int>(), 1, 4);
    if (calibrateDeskAiProfile(state.control, state.deskAi, static_cast<DeskState>(label))) {
      refreshDeskAiProfileMetrics(state.control, state.deskAi);
      settingsChanged = true;
    }
  }
  if (!state.control.deskAiValidationLocked &&
      doc["deskAiResetProfile"].is<bool>() && doc["deskAiResetProfile"].as<bool>()) {
    resetDeskAiProfile(state.control);
    state.deskAi.lastCalibrationLabel = DeskState::Unknown;
    state.deskAi.lastCalibrationMs = millis();
    refreshDeskAiProfileMetrics(state.control, state.deskAi);
    settingsChanged = true;
  }
  if (state.control.deskAiValidationLocked && doc["deskAiEvaluationLabel"].is<int>()) {
    const int label = constrain(doc["deskAiEvaluationLabel"].as<int>(), 1, 4);
    recordDeskAiEvaluation(state.deskAi, static_cast<DeskState>(label));
  }
  if (doc["deskAiFeedbackLabel"].is<int>()) {
    const int label = constrain(doc["deskAiFeedbackLabel"].as<int>(), 1, 4);
    if (resolveDeskAiFeedback(state.control, state.deskAi, static_cast<DeskState>(label))) {
      enqueueNotification("AI LABEL OK", CRGB(0x3A, 0xD7, 0xFF), 2200, 62);
      settingsChanged = true;
    }
  }
  if (doc["deskAiResetEvaluation"].is<bool>() && doc["deskAiResetEvaluation"].as<bool>()) {
    resetDeskAiEvaluation(state.deskAi);
  }
  if (doc["quietStartHour"].is<int>()) {
    state.control.quietStartHour = static_cast<uint8_t>(
        constrain(doc["quietStartHour"].as<int>(), 0, 23));
    settingsChanged = true;
  }
  if (doc["quietEndHour"].is<int>()) {
    state.control.quietEndHour = static_cast<uint8_t>(
        constrain(doc["quietEndHour"].as<int>(), 0, 23));
    settingsChanged = true;
  }
  if (doc["nightBrightnessCap"].is<int>()) {
    state.control.nightBrightnessCap = static_cast<uint8_t>(
        constrain(doc["nightBrightnessCap"].as<int>(), 1, 96));
    settingsChanged = true;
  }
  bool weatherConfigChanged = false;
  bool gameConfigChanged = false;
  const bool hasWeatherAutoLocate = doc["weatherAutoLocate"].is<bool>();
  if (doc["weatherEnabled"].is<bool>()) {
    state.control.weatherEnabled = doc["weatherEnabled"];
    settingsChanged = true;
    weatherConfigChanged = true;
  }
  if (doc["weatherDisplayMode"].is<int>()) {
    const int mode = constrain(doc["weatherDisplayMode"].as<int>(), 0, static_cast<int>(WeatherDisplayMode::DetailCycle));
    state.control.weatherDisplayMode = static_cast<WeatherDisplayMode>(mode);
    settingsChanged = true;
  }
  if (hasWeatherAutoLocate) {
    state.control.weatherAutoLocate = doc["weatherAutoLocate"].as<bool>();
    settingsChanged = true;
    weatherConfigChanged = true;
  }
  if (doc["weatherCity"].is<const char *>()) {
    copyWeatherCity(state.control.weatherCity, doc["weatherCity"].as<const char *>());
    if (!hasWeatherAutoLocate) {
      state.control.weatherAutoLocate = true;
    }
    settingsChanged = true;
    weatherConfigChanged = true;
  }
  if (doc["weatherLatitude"].is<float>() || doc["weatherLatitude"].is<double>() || doc["weatherLatitude"].is<int>()) {
    state.control.weatherLatitude = constrain(doc["weatherLatitude"].as<float>(), -90.0f, 90.0f);
    if (!hasWeatherAutoLocate) {
      state.control.weatherAutoLocate = false;
    }
    settingsChanged = true;
    weatherConfigChanged = true;
  }
  if (doc["weatherLongitude"].is<float>() || doc["weatherLongitude"].is<double>() || doc["weatherLongitude"].is<int>()) {
    state.control.weatherLongitude = constrain(doc["weatherLongitude"].as<float>(), -180.0f, 180.0f);
    if (!hasWeatherAutoLocate) {
      state.control.weatherAutoLocate = false;
    }
    settingsChanged = true;
    weatherConfigChanged = true;
  }
  if (doc["weatherUpdateIntervalMin"].is<int>()) {
    state.control.weatherUpdateIntervalMin = static_cast<uint16_t>(constrain(doc["weatherUpdateIntervalMin"].as<int>(), 5, 180));
    settingsChanged = true;
  }
  const bool weatherRefresh = doc["weatherRefresh"].is<bool>() && doc["weatherRefresh"].as<bool>();
  if (doc["gameType"].is<int>()) {
    const int type = constrain(doc["gameType"].as<int>(), 0, static_cast<int>(GameType::Breakout));
    const auto nextType = static_cast<GameType>(type);
    if (state.control.gameType != nextType || state.game.type != nextType) {
      state.control.gameType = nextType;
      gameConfigChanged = true;
    }
    settingsChanged = true;
  }
  if (doc["gameUseMpuControl"].is<bool>()) {
    state.control.gameUseMpuControl = doc["gameUseMpuControl"];
    settingsChanged = true;
  }
  if (doc["gameSpeedMs"].is<int>()) {
    state.control.gameSpeedMs = static_cast<uint16_t>(constrain(doc["gameSpeedMs"].as<int>(), 60, 600));
    state.game.stepIntervalMs = state.control.gameSpeedMs;
    settingsChanged = true;
  }
  if (doc["timerMode"].is<int>()) {
    const int mode = constrain(doc["timerMode"].as<int>(), 0, static_cast<int>(TimerMode::Stopwatch));
    setTimerMode(state.control, static_cast<TimerMode>(mode));
    settingsChanged = true;
  }
  if (doc["timerMinutes"].is<int>()) {
    setCountdownMinutes(state.control, static_cast<uint16_t>(doc["timerMinutes"].as<int>()));
    settingsChanged = true;
  }
  if (doc["pomodoroFocusMin"].is<int>()) {
    setPomodoroFocusMinutes(state.control, static_cast<uint16_t>(doc["pomodoroFocusMin"].as<int>()));
    settingsChanged = true;
  }
  if (doc["pomodoroBreakMin"].is<int>()) {
    setPomodoroBreakMinutes(state.control, static_cast<uint16_t>(doc["pomodoroBreakMin"].as<int>()));
    settingsChanged = true;
  }
  if (doc["timerAction"].is<const char *>()) {
    applyTimerAction(state.control, doc["timerAction"].as<const char *>(), millis());
  }
  bool colorChanged = false;
  if (doc["color"].is<JsonObject>()) {
    JsonObject color = doc["color"];
    state.control.preferredColor = CRGB(color["r"] | state.control.preferredColor.r,
                                        color["g"] | state.control.preferredColor.g,
                                        color["b"] | state.control.preferredColor.b);
    colorChanged = true;
    settingsChanged = true;
  }
  if (doc["color"].is<JsonArray>()) {
    JsonArray color = doc["color"];
    state.control.preferredColor = CRGB(constrain(color[0] | state.control.preferredColor.r, 0, 255),
                                        constrain(color[1] | state.control.preferredColor.g, 0, 255),
                                        constrain(color[2] | state.control.preferredColor.b, 0, 255));
    colorChanged = true;
    settingsChanged = true;
  }
  if (colorChanged) {
    if (state.control.mode == DisplayMode::Clock &&
        !doc["clockTheme"].is<int>() &&
        state.control.clockTheme != ClockTheme::Classic &&
        state.control.clockTheme != ClockTheme::Breath) {
      state.control.clockTheme = ClockTheme::Classic;
    }
    if (state.control.mode == DisplayMode::Spectrum && !doc["audioRainbow"].is<bool>()) {
      state.control.audioRainbow = false;
    }
    if (state.control.mode == DisplayMode::Text && !doc["scrollRainbow"].is<bool>()) {
      state.control.scrollRainbow = false;
    }
  }
  if (gameConfigChanged) {
    gameReset(state.game, state.control);
  }

  updateControlState(state.control);
  updateDeskAiState(state.deskAi);
  updateGameState(state.game);
  pushRenderSnapshot(0);
  if (settingsChanged) {
    requestSettingsSave();
  }
  if (weatherConfigChanged || weatherRefresh) {
    requestWeatherRefresh();
  }
  realtimeSocket.textAll(buildStateJson());
  lastRealtimePushMs = millis();
  request->send(200, "application/json", "{\"ok\":true}");
}

void handleGameActionBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }

  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  const char *action = doc["action"] | "";
  RenderState state = copySharedState();
  bool settingsChanged = false;
  if (state.game.type != state.control.gameType) {
    gameReset(state.game, state.control);
  }

  if (strcmp(action, "start") == 0) {
    if (state.game.runState == GameRunState::GameOver) {
      gameReset(state.game, state.control);
    }
    gameStart(state.game);
    if (state.control.mode != DisplayMode::Game) {
      state.control.mode = DisplayMode::Game;
      settingsChanged = true;
    }
  } else if (strcmp(action, "pause") == 0) {
    gamePause(state.game);
  } else if (strcmp(action, "resume") == 0) {
    gameResume(state.game);
  } else if (strcmp(action, "reset") == 0) {
    gameReset(state.game, state.control);
  } else if (strcmp(action, "toggle") == 0) {
    if (state.game.runState == GameRunState::Running) {
      gamePause(state.game);
    } else if (state.game.runState == GameRunState::Paused) {
      gameResume(state.game);
    } else {
      if (state.game.runState == GameRunState::GameOver) {
        gameReset(state.game, state.control);
      }
      gameStart(state.game);
    }
    if (state.control.mode != DisplayMode::Game) {
      state.control.mode = DisplayMode::Game;
      settingsChanged = true;
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"unknown action\"}");
    return;
  }

  updateControlState(state.control);
  updateGameState(state.game);
  pushRenderSnapshot(0);
  if (settingsChanged) {
    requestSettingsSave();
  }
  request->send(200, "application/json", "{\"ok\":true}");
}

void handleGameDirectionBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }

  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  const char *direction = doc["direction"] | "";
  RenderState state = copySharedState();
  bool ok = true;
  if (strcmp(direction, "up") == 0) {
    gameSetDirection(state.game, GameDirection::Up);
  } else if (strcmp(direction, "down") == 0) {
    gameSetDirection(state.game, GameDirection::Down);
  } else if (strcmp(direction, "left") == 0) {
    gameSetDirection(state.game, GameDirection::Left);
  } else if (strcmp(direction, "right") == 0) {
    gameSetDirection(state.game, GameDirection::Right);
  } else {
    ok = false;
  }

  if (!ok) {
    request->send(400, "application/json", "{\"error\":\"unknown direction\"}");
    return;
  }

  updateGameState(state.game);
  pushRenderSnapshot(0);
  request->send(200, "application/json", "{\"ok\":true}");
}

bool applyPreset(ControlState &control, const char *preset) {
  if (!preset) {
    return false;
  }
  control.smartScenes = false;

  if (strcmp(preset, "desk") == 0) {
    control.mode = DisplayMode::Clock;
    control.clockTheme = ClockTheme::Classic;
    control.autoBrightness = true;
    control.manualBrightness = AppConfig::DefaultBrightness;
    control.preferredColor = CRGB(0x3A, 0xD7, 0xFF);
    return true;
  }

  if (strcmp(preset, "night") == 0) {
    control.mode = DisplayMode::Clock;
    control.clockTheme = ClockTheme::Night;
    control.autoBrightness = false;
    control.manualBrightness = 12;
    control.preferredColor = CRGB(42, 14, 4);
    return true;
  }

  if (strcmp(preset, "music") == 0) {
    control.mode = DisplayMode::Spectrum;
    control.audioVisualMode = AudioVisualMode::BassPulse;
    control.audioSensitivity = 176;
    control.audioSmoothing = 118;
    control.audioRainbow = true;
    control.audioBeatFlash = true;
    return true;
  }

  if (strcmp(preset, "fluid") == 0) {
    control.mode = DisplayMode::Fluid;
    control.fluidParticles = AppConfig::FluidDefaultActiveParticles;
    control.fluidFlipRatio = AppConfig::FluidDefaultFlipRatio;
    control.fluidSeparateParticles = true;
    control.fluidCompensateDrift = true;
    return true;
  }

  if (strcmp(preset, "focus") == 0) {
    control.mode = DisplayMode::Timer;
    control.autoBrightness = false;
    control.manualBrightness = 44;
    control.preferredColor = CRGB(180, 40, 20);
    control.pomodoroFocusMin = AppConfig::DefaultPomodoroFocusMin;
    control.pomodoroBreakMin = AppConfig::DefaultPomodoroBreakMin;
    setTimerMode(control, TimerMode::Pomodoro);
    return true;
  }

  if (strcmp(preset, "weather") == 0) {
    control.mode = DisplayMode::Weather;
    control.weatherEnabled = true;
    control.weatherDisplayMode = WeatherDisplayMode::IconTemp;
    return true;
  }

  if (strcmp(preset, "game") == 0) {
    control.mode = DisplayMode::Game;
    control.gameType = GameType::Snake;
    control.gameSpeedMs = 160;
    control.gameUseMpuControl = false;
    return true;
  }

  return false;
}

void handlePresetBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    request->_tempObject = new String();
    static_cast<String *>(request->_tempObject)->reserve(total);
  }

  auto *body = static_cast<String *>(request->_tempObject);
  if (!body) {
    request->send(500, "application/json", "{\"error\":\"body buffer\"}");
    return;
  }

  body->concat(reinterpret_cast<const char *>(data), len);
  if (index + len != total) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;
  if (error) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  const char *preset = doc["preset"] | "";
  RenderState state = copySharedState();
  if (!applyPreset(state.control, preset)) {
    request->send(400, "application/json", "{\"error\":\"unknown preset\"}");
    return;
  }
  if (state.control.mode == DisplayMode::Game || state.game.type != state.control.gameType) {
    gameReset(state.game, state.control);
  }

  updateControlState(state.control);
  updateGameState(state.game);
  pushRenderSnapshot(0);
  requestSettingsSave();
  realtimeSocket.textAll(buildStateJson());
  lastRealtimePushMs = millis();
  request->send(200, "application/json", "{\"ok\":true}");
}
} // namespace

void startWebServer() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  realtimeSocket.onEvent(handleRealtimeEvent);
  server.addHandler(&realtimeSocket);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", WebAssets::IndexHtml);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  });
  server.on("/api/state", HTTP_GET, sendStateJson);
  server.on("/api/time/sync", HTTP_POST, [](AsyncWebServerRequest *request) {
    const bool ok = requestTimeSync();
    JsonDocument doc;
    doc["ok"] = ok;
    doc["server1"] = AppConfig::NtpServer1;
    doc["server2"] = AppConfig::NtpServer2;
    doc["time"] = static_cast<uint32_t>(time(nullptr));
    if (!ok) {
      doc["error"] = "Wi-Fi not connected";
    }
    String body;
    serializeJson(doc, body);
    recordCompetitionApiRequest(body.length());
    request->send(ok ? 200 : 503, "application/json", body);
  });
  server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getWiFiStatusJson());
  });
  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", scanWiFiNetworksJson());
  });
  server.on(
      "/api/wifi/connect",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleWiFiConnectBody);
  server.on("/api/screen", HTTP_GET, sendScreenJson);
  server.on("/api/config", HTTP_GET, sendConfigJson);
  server.on(
      "/api/config",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleControlBody);
  server.on("/api/save-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    const RenderState state = copySharedState();
    saveSettingsToNvs(state.control);
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/reset-settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    clearSettingsNvs();
    RenderState state = copySharedState();
    ControlState defaults;
    state.control = defaults;
    gameReset(state.game, state.control);
    updateControlState(state.control);
    updateGameState(state.game);
    pushRenderSnapshot(0);
    request->send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ok\":true}");
    xTaskCreate(rebootTask, "web_reboot", 2048, nullptr, 1, nullptr);
  });
  server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ok\":true,\"message\":\"wifi settings cleared, restarting\"}");
    xTaskCreate(wifiResetTask, "wifi_reset", 4096, nullptr, 1, nullptr);
  });
  server.on(
      "/api/preset",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handlePresetBody);
  server.on(
      "/api/game/action",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleGameActionBody);
  server.on(
      "/api/game/direction",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleGameDirectionBody);
  server.on(
      "/api/notify",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleNotificationBody);
  server.on(
      "/api/control",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      nullptr,
      handleControlBody);
  server.begin();
  Serial.println("[web] server started");
}

void serviceWebServer() {
  realtimeSocket.cleanupClients();
  const uint32_t now = millis();
  if (realtimeSocket.count() == 0 || now - lastRealtimePushMs < 250) {
    return;
  }
  lastRealtimePushMs = now;
  realtimeSocket.textAll(buildStateJson());
}
