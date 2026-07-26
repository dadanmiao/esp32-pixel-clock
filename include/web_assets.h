/*
 * Author: Yang
 * AWTRIX-style web console assets stored in flash.
 */
#pragma once

#include <Arduino.h>

namespace WebAssets {
const char IndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Pixel Clock Console</title>
  <style>
    :root { color-scheme:dark; --bg:#0f1215; --rail:#0b0e11; --panel:#171c21; --panel2:#11161a; --line:#2b353d; --text:#eef3f5; --muted:#94a2aa; --accent:#35c9e8; --ok:#70e3a3; --warn:#f3b45a; --bad:#ff6b6b; }
    * { box-sizing:border-box; }
    body { margin:0; min-height:100vh; background:var(--bg); color:var(--text); font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; }
    .shell { display:grid; grid-template-columns:226px 1fr; min-height:100vh; }
    aside { background:var(--rail); border-right:1px solid var(--line); padding:16px 12px; position:sticky; top:0; height:100vh; overflow:auto; }
    .brand { font-size:20px; font-weight:800; letter-spacing:0; }
    .sub { color:var(--muted); font-size:12px; margin:4px 0 16px; }
    .nav { display:grid; gap:6px; }
    .nav button { text-align:left; min-height:38px; padding:0 12px; border-radius:6px; border:1px solid transparent; background:transparent; color:var(--muted); cursor:pointer; font-weight:700; }
    .nav button.active { color:var(--text); background:var(--panel); border-color:var(--line); }
    main { padding:18px; min-width:0; }
    header { display:flex; align-items:flex-end; justify-content:space-between; gap:12px; margin-bottom:14px; }
    h1 { margin:0; font-size:28px; letter-spacing:0; }
    h2 { margin:0 0 12px; font-size:16px; letter-spacing:0; }
    h3 { margin:18px 0 10px; color:var(--muted); font-size:13px; letter-spacing:0; }
    .clock { color:var(--warn); font-size:22px; font-variant-numeric:tabular-nums; }
    .page { display:none; }
    .page.active { display:block; }
    .tiles, .grid { display:grid; gap:12px; }
    .tiles { grid-template-columns:repeat(auto-fit,minmax(150px,1fr)); margin-bottom:12px; }
    .grid { grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); align-items:start; }
    .card, .tile { background:var(--panel); border:1px solid var(--line); border-radius:8px; }
    .card { padding:14px; }
    .tile { padding:12px; min-height:78px; }
    .tile span { display:block; color:var(--muted); font-size:12px; }
    .tile b { display:block; margin-top:6px; font-size:22px; font-variant-numeric:tabular-nums; overflow-wrap:anywhere; }
    .kv { display:grid; grid-template-columns:1fr auto; gap:10px; border-bottom:1px solid #242d34; padding:8px 0; font-size:13px; }
    .kv:last-child { border-bottom:0; }
    .kv span:first-child { color:var(--muted); }
    .kv b { text-align:right; font-weight:700; font-variant-numeric:tabular-nums; }
    .formgrid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:12px; }
    .field { min-width:0; }
    label { display:block; color:var(--muted); font-size:12px; margin:0 0 6px; }
    input, select, button { width:100%; min-height:38px; border:1px solid var(--line); border-radius:6px; background:var(--panel2); color:var(--text); font:inherit; }
    input[type="text"], input[type="number"], select { padding:0 10px; }
    input[type="range"] { accent-color:var(--accent); }
    input[type="color"] { padding:3px; }
    input[type="checkbox"] { width:auto; min-height:0; }
    button { cursor:pointer; padding:0 10px; font-weight:800; }
    button.primary, button.active { border-color:var(--accent); background:#11313a; }
    button.danger { border-color:#75333a; background:#32171a; color:#ffd8d8; }
    .seg { display:grid; grid-template-columns:repeat(auto-fit,minmax(86px,1fr)); gap:8px; }
    .dpad { display:grid; grid-template-columns:repeat(3, minmax(56px, 1fr)); gap:8px; max-width:260px; margin:0 auto; }
    .dpad .up { grid-column:2; }
    .dpad .left { grid-column:1; }
    .dpad .right { grid-column:3; }
    .dpad .down { grid-column:2; }
    .check { display:flex; align-items:center; gap:8px; min-height:38px; color:var(--muted); font-size:13px; }
    .row { display:flex; gap:8px; align-items:center; }
    .row > * { flex:1; }
    .value { color:var(--warn); font-variant-numeric:tabular-nums; }
    .matrix { display:grid; grid-template-columns:repeat(32,12px); gap:2px; overflow:auto; padding-bottom:4px; }
    .pixel { width:12px; height:12px; border-radius:3px; background:#000; border:1px solid #1f2529; }
    .bars { display:grid; grid-template-columns:repeat(32,1fr); gap:2px; height:96px; align-items:end; }
    .bar { min-height:3px; border-radius:3px 3px 0 0; background:linear-gradient(#35c9e8,#7ee787); }
    @media (max-width:820px) {
      .shell { grid-template-columns:1fr; }
      aside { position:static; height:auto; }
      .nav { grid-template-columns:repeat(2,minmax(0,1fr)); }
      header, .formgrid { display:block; }
      .field, .check { margin-bottom:12px; }
    }
  </style>
</head>
<body>
<div class="shell">
  <aside>
    <div class="brand">Pixel Clock</div>
    <div class="sub">ESP32-S3 Console</div>
    <div class="nav">
      <button class="active" data-page="dashboard">Dashboard</button>
      <button data-page="display">Display</button>
      <button data-page="clockPage">Clock</button>
      <button data-page="audioPage">Audio</button>
      <button data-page="fluidPage">Fluid</button>
      <button data-page="textPage">Text</button>
      <button data-page="timerPage">Timer</button>
      <button data-page="weatherPage">Weather</button>
      <button data-page="gamePage">Game</button>
      <button data-page="sensors">Sensors</button>
      <button data-page="powerPage">Power</button>
      <button data-page="network">Network</button>
      <button data-page="liveview">LiveView</button>
      <button data-page="backup">Backup</button>
      <button data-page="systemPage">System</button>
    </div>
  </aside>

  <main>
    <header>
      <div>
        <h1 id="pageTitle">Dashboard</h1>
        <div class="sub" id="pageSub">Device overview</div>
      </div>
      <div class="clock" id="clock">--:--:--</div>
    </header>

    <section id="dashboard" class="page active">
      <div class="tiles">
        <div class="tile"><span>Mode</span><b id="dashMode">--</b></div>
        <div class="tile"><span>Brightness</span><b id="dashBrightness">--</b></div>
        <div class="tile"><span>VBUS</span><b id="dashVbus">--</b></div>
        <div class="tile"><span>Audio</span><b id="dashAudio">--</b></div>
        <div class="tile"><span>Weather</span><b id="dashWeather">--</b></div>
        <div class="tile"><span>MPU</span><b id="dashMpu">--</b></div>
        <div class="tile"><span>Heap</span><b id="dashHeap">--</b></div>
      </div>
      <div class="grid">
        <div class="card">
          <h2>Power</h2>
          <div class="kv"><span>Current Limit</span><b id="dashCurrent">--</b></div>
          <div class="kv"><span>Brightness Cap</span><b id="dashCap">--</b></div>
          <div class="kv"><span>DC-DC</span><b id="dashDcdc">--</b></div>
        </div>
        <div class="card">
          <h2>Sensors</h2>
          <div class="kv"><span>Temperature</span><b id="dashTemp">--</b></div>
          <div class="kv"><span>Humidity</span><b id="dashHum">--</b></div>
          <div class="kv"><span>LDR</span><b id="dashLdr">--</b></div>
        </div>
      </div>
    </section>

    <section id="display" class="page">
      <div class="grid">
        <div class="card">
          <h2>Display Mode</h2>
          <div class="seg" id="modeButtons">
            <button data-mode="0">Clock</button>
            <button data-mode="1">Spectrum</button>
            <button data-mode="2">Fluid</button>
            <button data-mode="3">Text</button>
            <button data-mode="4">Timer</button>
            <button data-mode="5">Weather</button>
            <button data-mode="6">Game</button>
          </div>
          <h3>Color</h3>
          <input id="color" type="color" value="#3ad7ff">
        </div>
        <div class="card">
          <h2>Brightness</h2>
          <div class="formgrid">
            <label class="check"><input id="autoBrightness" type="checkbox"> Auto Brightness</label>
            <div class="field">
              <label>Manual <span id="brightnessValue" class="value">64</span></label>
              <input id="brightness" type="range" min="1" max="255" value="64">
            </div>
          </div>
        </div>
      </div>
    </section>

    <section id="clockPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Clock Theme</h2>
          <select id="clockTheme">
            <option value="0">Classic</option>
            <option value="1">Rainbow</option>
            <option value="2">Breath</option>
            <option value="3">Night</option>
            <option value="4">Minimal</option>
          </select>
        </div>
      </div>
    </section>

    <section id="audioPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Audio Visualizer</h2>
          <div class="formgrid">
            <div class="field">
              <label>Visual Mode</label>
              <select id="audioVisualMode">
                <option value="0">Spectrum</option>
                <option value="1">Mirror Spectrum</option>
                <option value="2">VU Meter</option>
                <option value="3">Bass Pulse</option>
                <option value="4">Fire Spectrum</option>
                <option value="5">Center Burst</option>
              </select>
            </div>
            <button id="showSpectrumMode" class="primary" type="button">Show Spectrum</button>
            <div class="field">
              <label>Sensitivity <span id="audioSensitivityValue" class="value">128</span></label>
              <input id="audioSensitivity" type="range" min="0" max="255" value="128">
            </div>
            <div class="field">
              <label>Smoothing <span id="audioSmoothingValue" class="value">160</span></label>
              <input id="audioSmoothing" type="range" min="0" max="255" value="160">
            </div>
            <label class="check"><input id="audioRainbow" type="checkbox"> Rainbow</label>
            <label class="check"><input id="audioBeatFlash" type="checkbox"> Beat Flash</label>
          </div>
        </div>
        <div class="card">
          <h2>Audio Diagnostics</h2>
          <div class="kv"><span>RMS / Peak</span><b id="audioRmsPeak">--</b></div>
          <div class="kv"><span>Low / Mid / High</span><b id="audioBands">--</b></div>
          <div class="kv"><span>MIC ADC</span><b id="audioMic">--</b></div>
          <div class="kv"><span>Source</span><b id="audioSource">--</b></div>
        </div>
      </div>
    </section>

    <section id="fluidPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>FLIP Fluid</h2>
          <div class="formgrid">
            <div class="field">
              <label>Particles <span id="particlesValue" class="value">48</span></label>
              <input id="particles" type="range" min="8" max="64" value="48">
            </div>
            <div class="field">
              <label>PIC / FLIP <span id="flipValue" class="value">78%</span></label>
              <input id="flip" type="range" min="0" max="100" value="78">
            </div>
            <label class="check"><input id="separate" type="checkbox"> Separate Particles</label>
            <label class="check"><input id="compensate" type="checkbox"> Compensate Drift</label>
          </div>
        </div>
        <div class="card">
          <h2>Motion</h2>
          <div class="kv"><span>Accel</span><b id="fluidAccel">--</b></div>
          <div class="kv"><span>MPU</span><b id="fluidMpu">--</b></div>
        </div>
      </div>
    </section>

    <section id="textPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Scrolling Text</h2>
          <div class="field">
            <label>Text</label>
            <div class="row"><input id="scrollText" type="text" maxlength="63" value="PIXEL CLOCK"><button id="applyScrollText" type="button">Apply</button></div>
          </div>
          <div class="field">
            <label>Speed <span id="scrollSpeedValue" class="value">90 ms</span></label>
            <input id="scrollSpeed" type="range" min="30" max="500" value="90">
          </div>
          <label class="check"><input id="scrollRainbow" type="checkbox"> Rainbow Text</label>
          <h3>Presets</h3>
          <div class="seg">
            <button data-text="HELLO">HELLO</button>
            <button data-text="FOCUS">FOCUS</button>
            <button data-text="GOOD NIGHT">GOOD NIGHT</button>
            <button data-text="TIME UP">TIME UP</button>
          </div>
        </div>
      </div>
    </section>

    <section id="timerPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Timer</h2>
          <div class="formgrid">
            <div class="field">
              <label>Timer Mode</label>
              <select id="timerMode">
                <option value="0">Pomodoro</option>
                <option value="1">Countdown</option>
                <option value="2">Stopwatch</option>
              </select>
            </div>
            <div class="field">
              <label>Status</label>
              <input id="timerStatus" type="text" value="--:--" readonly>
            </div>
            <div class="field">
              <label>Countdown Minutes</label>
              <div class="row"><input id="timerMinutes" type="number" min="1" max="99" value="5"><button id="setCountdown" type="button">Set</button></div>
            </div>
            <div class="field">
              <label>Focus / Break</label>
              <div class="row"><input id="pomodoroFocusMin" type="number" min="1" max="99" value="25"><input id="pomodoroBreakMin" type="number" min="1" max="60" value="5"></div>
            </div>
          </div>
          <h3>Actions</h3>
          <div class="seg">
            <button id="timerStart" type="button">Start</button>
            <button id="timerPause" type="button">Pause</button>
            <button id="timerResume" type="button">Resume</button>
            <button id="timerReset" type="button">Reset</button>
          </div>
        </div>
      </div>
    </section>

    <section id="weatherPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Weather Settings</h2>
          <div class="formgrid">
            <label class="check"><input id="weatherEnabled" type="checkbox"> Enable Weather</label>
            <button id="showWeatherMode" class="primary" type="button">Show Weather</button>
            <div class="field">
              <label>City</label>
              <input id="weatherCity" type="text" maxlength="31" value="Lanzhou">
            </div>
            <div class="field">
              <label>Display Mode</label>
              <select id="weatherDisplayMode">
                <option value="0">Icon + Temp</option>
                <option value="1">Temp Only</option>
                <option value="2">Detail Cycle</option>
              </select>
            </div>
            <div class="field">
              <label>Latitude</label>
              <input id="weatherLatitude" type="number" min="-90" max="90" step="0.0001" value="36.0611">
            </div>
            <div class="field">
              <label>Longitude</label>
              <input id="weatherLongitude" type="number" min="-180" max="180" step="0.0001" value="103.8343">
            </div>
            <div class="field">
              <label>Update Interval / min</label>
              <input id="weatherUpdateIntervalMin" type="number" min="5" max="180" value="30">
            </div>
            <button id="weatherRefresh" type="button">Refresh Now</button>
          </div>
        </div>
        <div class="card">
          <h2>Current Weather</h2>
          <div class="kv"><span>Status</span><b id="weatherStatus">--</b></div>
          <div class="kv"><span>Temperature</span><b id="weatherTemp">--</b></div>
          <div class="kv"><span>Feels Like</span><b id="weatherFeels">--</b></div>
          <div class="kv"><span>Humidity</span><b id="weatherHumidity">--</b></div>
          <div class="kv"><span>Code / Cloud</span><b id="weatherCodeCloud">--</b></div>
          <div class="kv"><span>Rain / Wind</span><b id="weatherRainWind">--</b></div>
          <div class="kv"><span>Today</span><b id="weatherToday">--</b></div>
          <div class="kv"><span>Last Success</span><b id="weatherLast">--</b></div>
          <div class="kv"><span>Failures</span><b id="weatherFail">--</b></div>
          <div class="kv"><span>Error</span><b id="weatherError">--</b></div>
        </div>
      </div>
    </section>

    <section id="gamePage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Pixel Game</h2>
          <div class="formgrid">
            <div class="field">
              <label>Game Type</label>
              <select id="gameType">
                <option value="0">Snake</option>
                <option value="1">Gravity Ball</option>
                <option value="2">Reaction</option>
                <option value="3">Pong</option>
                <option value="4">Breakout</option>
              </select>
            </div>
            <button id="showGameMode" class="primary" type="button">Show Game</button>
            <div class="field">
              <label>Speed <span id="gameSpeedValue" class="value">160 ms</span></label>
              <input id="gameSpeedMs" type="range" min="60" max="600" value="160">
            </div>
            <label class="check"><input id="gameUseMpuControl" type="checkbox"> Use MPU Control</label>
          </div>
          <h3>Actions</h3>
          <div class="seg">
            <button id="gameStart" type="button">Start</button>
            <button id="gamePause" type="button">Pause</button>
            <button id="gameResume" type="button">Resume</button>
            <button id="gameReset" type="button">Reset</button>
          </div>
        </div>
        <div class="card">
          <h2>Controls</h2>
          <div class="dpad">
            <button class="up" data-game-dir="up" type="button">Up</button>
            <button class="left" data-game-dir="left" type="button">Left</button>
            <button class="right" data-game-dir="right" type="button">Right</button>
            <button class="down" data-game-dir="down" type="button">Down</button>
          </div>
        </div>
        <div class="card">
          <h2>Status</h2>
          <div class="kv"><span>State</span><b id="gameRunState">--</b></div>
          <div class="kv"><span>Score</span><b id="gameScore">--</b></div>
          <div class="kv"><span>High Score</span><b id="gameHighScore">--</b></div>
          <div class="kv"><span>Length / Bricks</span><b id="gameSnakeLen">--</b></div>
        </div>
      </div>
    </section>

    <section id="sensors" class="page">
      <div class="grid">
        <div class="card">
          <h2>Environment</h2>
          <div class="kv"><span>Temperature</span><b id="sensorTemp">--</b></div>
          <div class="kv"><span>Humidity</span><b id="sensorHum">--</b></div>
          <div class="kv"><span>LDR Raw / Brightness</span><b id="sensorLdr">--</b></div>
          <div class="kv"><span>Accel</span><b id="sensorAccel">--</b></div>
          <div class="kv"><span>MPU Count</span><b id="sensorMpu">--</b></div>
        </div>
        <div class="card">
          <h2>LDR Thresholds</h2>
          <div class="field">
            <label>Low <span id="lowValue" class="value">900</span></label>
            <input id="low" type="range" min="0" max="4095" value="900">
          </div>
          <div class="field">
            <label>High <span id="highValue" class="value">3300</span></label>
            <input id="high" type="range" min="0" max="4095" value="3300">
          </div>
        </div>
      </div>
    </section>

    <section id="powerPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Power State</h2>
          <div class="kv"><span>VBUS</span><b id="powerVbus">--</b></div>
          <div class="kv"><span>High Power</span><b id="powerHigh">--</b></div>
          <div class="kv"><span>DC-DC Enabled</span><b id="powerDcdc">--</b></div>
          <div class="kv"><span>Current Limit</span><b id="powerLimit">--</b></div>
          <div class="kv"><span>Brightness Cap</span><b id="powerCap">--</b></div>
        </div>
      </div>
    </section>

    <section id="network" class="page">
      <div class="grid">
        <div class="card">
          <h2>Network</h2>
          <div class="kv"><span>Status</span><b id="netStatus">--</b></div>
          <div class="kv"><span>Mode</span><b id="netMode">--</b></div>
          <div class="kv"><span>SSID</span><b id="netSsid">--</b></div>
          <div class="kv"><span>IP</span><b id="netIp">--</b></div>
          <div class="kv"><span>RSSI</span><b id="netRssi">--</b></div>
          <div class="kv"><span>Hostname</span><b id="netHost">--</b></div>
          <div class="kv"><span>Setup AP</span><b id="netAp">--</b></div>
          <h3>Provisioning</h3>
          <div class="formgrid">
            <div class="field">
              <label>Nearby Wi-Fi</label>
              <select id="wifiScanList"><option value="">Scan first</option></select>
            </div>
            <button id="scanWifi" type="button">Scan Wi-Fi</button>
            <div class="field">
              <label>SSID</label>
              <input id="wifiSsidInput" type="text" maxlength="32" placeholder="Wi-Fi SSID">
            </div>
            <div class="field">
              <label>Password</label>
              <input id="wifiPasswordInput" type="password" maxlength="64" placeholder="Wi-Fi password">
            </div>
          </div>
          <div class="seg">
            <button id="connectWifi" class="primary" type="button">Save & Connect</button>
            <button id="resetWifi" class="danger" type="button">Clear Wi-Fi & Restart</button>
          </div>
        </div>
      </div>
    </section>

    <section id="liveview" class="page">
      <div class="grid">
        <div class="card">
          <h2>Matrix LiveView</h2>
          <div id="matrix" class="matrix"></div>
        </div>
        <div class="card">
          <h2>Spectrum Monitor</h2>
          <div id="bars" class="bars"></div>
        </div>
      </div>
    </section>

    <section id="backup" class="page">
      <div class="grid">
        <div class="card">
          <h2>Backup</h2>
          <div class="seg">
            <button id="exportConfig" class="primary" type="button">Export Config</button>
            <button id="importConfigBtn" type="button">Import Config</button>
          </div>
          <input id="configFile" type="file" accept=".json" style="margin-top:10px">
        </div>
      </div>
    </section>

    <section id="systemPage" class="page">
      <div class="grid">
        <div class="card">
          <h2>Presets</h2>
          <div class="seg">
            <button data-preset="desk">Desk</button>
            <button data-preset="night">Night</button>
            <button data-preset="music">Music</button>
            <button data-preset="fluid">Fluid</button>
            <button data-preset="focus">Focus</button>
            <button data-preset="weather">Weather</button>
            <button data-preset="game">Game</button>
          </div>
        </div>
        <div class="card">
          <h2>System</h2>
          <div class="seg">
            <button id="saveSettings" class="primary" type="button">Save Settings</button>
            <button id="resetSettings" class="danger" type="button">Restore Defaults</button>
            <button id="rebootDevice" class="danger" type="button">Reboot</button>
          </div>
        </div>
      </div>
    </section>
  </main>
</div>

<script>
const modes = ["Clock", "Spectrum", "Fluid", "Text", "Timer", "Weather", "Game"];
const timerStates = ["Idle", "Running", "Paused", "Finished"];
const gameStates = ["Idle", "Running", "Paused", "Game Over"];
const q = id => document.getElementById(id);
let state = {};
let scrollTextTimer = 0;
let pendingScrollText = null;
let queuedScrollText = null;
let scrollTextInFlight = false;
let rangeTimers = {};

function setText(id, value) { const el = q(id); if (el) el.textContent = value; }
function setValue(id, value) { const el = q(id); if (el && document.activeElement !== el) el.value = value; }
function setChecked(id, value) { const el = q(id); if (el) el.checked = !!value; }
function fixed(v, d) { return Number.isFinite(v) ? v.toFixed(d) : "--"; }
function fmtSeconds(total) { total = Math.max(0, Math.floor(total || 0)); return String(Math.floor(total / 60)).padStart(2, "0") + ":" + String(total % 60).padStart(2, "0"); }
function fmtUptime(ms) { const s = Math.floor((ms || 0) / 1000); return Math.floor(s / 3600) + "h " + String(Math.floor((s % 3600) / 60)).padStart(2, "0") + "m"; }
function fmtAge(ms) { if (!ms || !state.uptimeMs) return "--"; const s = Math.max(0, Math.floor((state.uptimeMs - ms) / 1000)); return s < 60 ? s + "s ago" : Math.floor(s / 60) + "m ago"; }
function rgbToHex(c) { return "#" + [c?.r ?? 0, c?.g ?? 0, c?.b ?? 0].map(v => Math.max(0, Math.min(255, v)).toString(16).padStart(2, "0")).join(""); }
function colorToRgb(hex) { const n = parseInt(hex.slice(1), 16); return { r:(n >> 16) & 255, g:(n >> 8) & 255, b:n & 255 }; }

async function sendPatch(patch, refreshAfter = true) {
  await fetch("/api/control", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify(patch) });
  if (refreshAfter) await refresh();
}
async function postGameAction(action) {
  await fetch("/api/game/action", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify({ action }) });
  await refresh();
}
async function postGameDirection(direction) {
  await fetch("/api/game/direction", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify({ direction }) });
}
function sendRange(key, value) {
  clearTimeout(rangeTimers[key]);
  rangeTimers[key] = setTimeout(() => sendPatch({ [key]: value }, false), 180);
}
function sendScrollTextNow(value) {
  clearTimeout(scrollTextTimer);
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
    pendingScrollText = null;
    scrollTextInFlight = false;
  }
}
function queueScrollText(value) {
  pendingScrollText = value;
  clearTimeout(scrollTextTimer);
  scrollTextTimer = setTimeout(() => sendScrollTextNow(value), 260);
}

document.querySelectorAll(".nav button").forEach(btn => {
  btn.onclick = () => {
    document.querySelectorAll(".nav button").forEach(b => b.classList.remove("active"));
    document.querySelectorAll(".page").forEach(p => p.classList.remove("active"));
    btn.classList.add("active");
    q(btn.dataset.page).classList.add("active");
    q("pageTitle").textContent = btn.textContent;
  };
});
document.querySelectorAll("button[data-mode]").forEach(btn => btn.onclick = () => sendPatch({ mode:Number(btn.dataset.mode) }));
document.querySelectorAll("button[data-preset]").forEach(btn => btn.onclick = async () => { await fetch("/api/preset", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify({ preset:btn.dataset.preset }) }); await refresh(); });
document.querySelectorAll("button[data-text]").forEach(btn => btn.onclick = () => { q("scrollText").value = btn.dataset.text; sendScrollTextNow(btn.dataset.text); });

q("color").oninput = e => sendPatch({ color:colorToRgb(e.target.value) }, false);
q("clockTheme").onchange = e => sendPatch({ clockTheme:Number(e.target.value) });
q("audioVisualMode").onchange = e => sendPatch({ audioVisualMode:Number(e.target.value), mode:1 });
q("showSpectrumMode").onclick = () => sendPatch({ mode:1 });
q("audioSensitivity").oninput = e => { setText("audioSensitivityValue", e.target.value); sendRange("audioSensitivity", Number(e.target.value)); };
q("audioSmoothing").oninput = e => { setText("audioSmoothingValue", e.target.value); sendRange("audioSmoothing", Number(e.target.value)); };
q("audioRainbow").onchange = e => sendPatch({ audioRainbow:e.target.checked });
q("audioBeatFlash").onchange = e => sendPatch({ audioBeatFlash:e.target.checked });
q("scrollText").oninput = e => queueScrollText(e.target.value);
q("scrollText").onchange = e => sendScrollTextNow(e.target.value);
q("scrollText").onblur = e => sendScrollTextNow(e.target.value);
q("scrollText").oncompositionend = e => queueScrollText(e.target.value);
q("scrollText").onkeydown = e => { if (e.key === "Enter") sendScrollTextNow(e.target.value); };
q("applyScrollText").onclick = () => sendScrollTextNow(q("scrollText").value);
q("scrollSpeed").oninput = e => { setText("scrollSpeedValue", e.target.value + " ms"); sendRange("scrollSpeedMs", Number(e.target.value)); };
q("scrollRainbow").onchange = e => sendPatch({ scrollRainbow:e.target.checked });
q("timerMode").onchange = e => sendPatch({ timerMode:Number(e.target.value) });
q("setCountdown").onclick = () => sendPatch({ timerMinutes:Number(q("timerMinutes").value), mode:4 });
q("pomodoroFocusMin").onchange = e => sendPatch({ pomodoroFocusMin:Number(e.target.value) });
q("pomodoroBreakMin").onchange = e => sendPatch({ pomodoroBreakMin:Number(e.target.value) });
q("timerStart").onclick = () => sendPatch({ timerAction:"start", mode:4 });
q("timerPause").onclick = () => sendPatch({ timerAction:"pause", mode:4 });
q("timerResume").onclick = () => sendPatch({ timerAction:"resume", mode:4 });
q("timerReset").onclick = () => sendPatch({ timerAction:"reset", mode:4 });
q("weatherEnabled").onchange = e => sendPatch({ weatherEnabled:e.target.checked });
q("showWeatherMode").onclick = () => sendPatch({ mode:5, weatherRefresh:true });
q("weatherCity").onchange = e => sendPatch({ weatherCity:e.target.value, weatherRefresh:true });
q("weatherDisplayMode").onchange = e => sendPatch({ weatherDisplayMode:Number(e.target.value) });
q("weatherLatitude").onchange = e => sendPatch({ weatherLatitude:Number(e.target.value), weatherRefresh:true });
q("weatherLongitude").onchange = e => sendPatch({ weatherLongitude:Number(e.target.value), weatherRefresh:true });
q("weatherUpdateIntervalMin").onchange = e => sendPatch({ weatherUpdateIntervalMin:Number(e.target.value) });
q("weatherRefresh").onclick = () => sendPatch({ weatherRefresh:true }, false);
q("gameType").onchange = e => sendPatch({ gameType:Number(e.target.value) });
q("showGameMode").onclick = () => sendPatch({ mode:6 });
q("gameSpeedMs").oninput = e => { setText("gameSpeedValue", e.target.value + " ms"); sendRange("gameSpeedMs", Number(e.target.value)); };
q("gameUseMpuControl").onchange = e => sendPatch({ gameUseMpuControl:e.target.checked });
q("gameStart").onclick = () => postGameAction("start");
q("gamePause").onclick = () => postGameAction("pause");
q("gameResume").onclick = () => postGameAction("resume");
q("gameReset").onclick = () => postGameAction("reset");
document.querySelectorAll("button[data-game-dir]").forEach(btn => btn.onclick = () => postGameDirection(btn.dataset.gameDir));
q("brightness").oninput = e => { setText("brightnessValue", e.target.value); sendRange("manualBrightness", Number(e.target.value)); };
q("autoBrightness").onchange = e => sendPatch({ autoBrightness:e.target.checked });
q("low").oninput = e => { setText("lowValue", e.target.value); sendRange("lowLightThreshold", Number(e.target.value)); };
q("high").oninput = e => { setText("highValue", e.target.value); sendRange("highLightThreshold", Number(e.target.value)); };
q("particles").oninput = e => { setText("particlesValue", e.target.value); sendRange("fluidParticles", Number(e.target.value)); };
q("flip").oninput = e => { setText("flipValue", e.target.value + "%"); sendRange("fluidFlipRatio", Number(e.target.value) / 100); };
q("separate").onchange = e => sendPatch({ fluidSeparateParticles:e.target.checked });
q("compensate").onchange = e => sendPatch({ fluidCompensateDrift:e.target.checked });
q("saveSettings").onclick = async () => { await fetch("/api/save-settings", { method:"POST" }); await refresh(); };
q("resetSettings").onclick = async () => { if (!confirm("Restore default settings?")) return; await fetch("/api/reset-settings", { method:"POST" }); await refresh(); };
q("rebootDevice").onclick = async () => { if (!confirm("Reboot device?")) return; await fetch("/api/reboot", { method:"POST" }); };
q("scanWifi").onclick = async () => {
  q("scanWifi").textContent = "Scanning...";
  try {
    const res = await fetch("/api/wifi/scan");
    const data = await res.json();
    q("wifiScanList").innerHTML = '<option value="">Manual SSID</option>';
    (data.networks || []).forEach(net => {
      const option = document.createElement("option");
      option.value = net.ssid;
      option.textContent = `${net.ssid || "(hidden)"}  ${net.rssi} dBm  ${net.auth}`;
      q("wifiScanList").appendChild(option);
    });
  } finally {
    q("scanWifi").textContent = "Scan Wi-Fi";
  }
};
q("wifiScanList").onchange = e => { if (e.target.value) q("wifiSsidInput").value = e.target.value; };
q("connectWifi").onclick = async () => {
  const ssid = q("wifiSsidInput").value.trim();
  if (!ssid) return;
  await fetch("/api/wifi/connect", { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify({ ssid, password:q("wifiPasswordInput").value }) });
  q("wifiPasswordInput").value = "";
  setTimeout(refresh, 1200);
};
q("resetWifi").onclick = async () => { if (!confirm("Clear saved Wi-Fi and restart into setup mode?")) return; await fetch("/api/wifi/reset", { method:"POST" }); };
q("exportConfig").onclick = () => { location.href = "/api/config"; };
q("importConfigBtn").onclick = async () => {
  const file = q("configFile").files[0];
  if (!file) return;
  const text = await file.text();
  await fetch("/api/config", { method:"POST", headers:{ "Content-Type":"application/json" }, body:text });
  await refresh();
};

function ensureLiveView() {
  const matrix = q("matrix");
  if (matrix && !matrix.dataset.ready) {
    for (let i = 0; i < 256; i++) {
      const p = document.createElement("div");
      p.className = "pixel";
      matrix.appendChild(p);
    }
    matrix.dataset.ready = "1";
  }
  const bars = q("bars");
  if (bars && !bars.dataset.ready) {
    for (let i = 0; i < 32; i++) {
      const b = document.createElement("div");
      b.className = "bar";
      bars.appendChild(b);
    }
    bars.dataset.ready = "1";
  }
}

async function refreshScreen() {
  ensureLiveView();
  try {
    const res = await fetch("/api/screen");
    const screen = await res.json();
    const pixels = q("matrix").children;
    for (let i = 0; i < Math.min(pixels.length, screen.pixels?.length || 0); i++) {
      const c = screen.pixels[i];
      pixels[i].style.background = `rgb(${c[0]},${c[1]},${c[2]})`;
    }
  } catch (e) {}
}

async function refresh() {
  const res = await fetch("/api/state");
  state = await res.json();
  const timerValue = state.timerMode === 2 ? state.stopwatchElapsedSec : state.timerRemainingSec;
  const timerLabel = fmtSeconds(timerValue) + " " + (timerStates[state.timerState] || "Idle");

  document.querySelectorAll("button[data-mode]").forEach(btn => btn.classList.toggle("active", Number(btn.dataset.mode) === state.mode));
  setText("clock", state.time || "--:--:--");
  setText("dashMode", modes[state.mode] || "Clock");
  setText("dashBrightness", (state.autoBrightness ? "Auto " : "Manual ") + state.manualBrightness);
  setText("dashVbus", fixed(state.vbus, 2) + " V");
  setText("dashAudio", fixed(state.rms, 3) + " / " + fixed(state.peak, 3));
  setText("dashWeather", state.weather?.hasData ? fixed(state.weather.temperature, 1) + " C" : (state.weather?.lastError || "--"));
  setText("dashMpu", state.mpuOnline ? "online" : "offline");
  setText("dashHeap", state.freeHeap ?? "--");
  setText("dashCurrent", state.maxMilliamps + " mA");
  setText("dashCap", state.brightnessCap + " / 255");
  setText("dashDcdc", state.dcdcEnabled ? "on" : "off");
  setText("dashTemp", Number.isFinite(state.temperatureC) ? state.temperatureC.toFixed(1) + " C" : "--");
  setText("dashHum", Number.isFinite(state.humidityRh) ? state.humidityRh.toFixed(0) + " %" : "--");
  setText("dashLdr", state.rawLdr + " / " + state.adaptiveBrightness);

  setValue("color", rgbToHex(state.color));
  setValue("clockTheme", state.clockTheme ?? 0);
  setValue("audioVisualMode", state.audioVisualMode ?? 0);
  setValue("audioSensitivity", state.audioSensitivity);
  setText("audioSensitivityValue", state.audioSensitivity);
  setValue("audioSmoothing", state.audioSmoothing);
  setText("audioSmoothingValue", state.audioSmoothing);
  setChecked("audioRainbow", state.audioRainbow);
  setChecked("audioBeatFlash", state.audioBeatFlash);
  if (document.activeElement !== q("scrollText") && pendingScrollText === null && queuedScrollText === null && !scrollTextInFlight) setValue("scrollText", state.scrollText || "");
  setValue("scrollSpeed", state.scrollSpeedMs);
  setText("scrollSpeedValue", state.scrollSpeedMs + " ms");
  setChecked("scrollRainbow", state.scrollRainbow);
  setValue("timerMode", state.timerMode ?? 0);
  setValue("timerMinutes", state.timerDurationSec ? Math.max(1, Math.round(state.timerDurationSec / 60)) : 5);
  setValue("pomodoroFocusMin", state.pomodoroFocusMin);
  setValue("pomodoroBreakMin", state.pomodoroBreakMin);
  setValue("timerStatus", timerLabel);
  setChecked("weatherEnabled", state.weather?.enabled ?? state.weatherEnabled);
  setValue("weatherCity", state.weather?.city ?? state.weatherCity ?? "Lanzhou");
  setValue("weatherDisplayMode", state.weather?.displayMode ?? state.weatherDisplayMode ?? 0);
  setValue("weatherLatitude", state.weather?.latitude ?? state.weatherLatitude ?? 36.0611);
  setValue("weatherLongitude", state.weather?.longitude ?? state.weatherLongitude ?? 103.8343);
  setValue("weatherUpdateIntervalMin", state.weather?.updateIntervalMin ?? state.weatherUpdateIntervalMin ?? 30);
  setValue("gameType", state.game?.type ?? state.gameType ?? 0);
  setValue("gameSpeedMs", state.game?.speedMs ?? state.gameSpeedMs ?? 160);
  setText("gameSpeedValue", (state.game?.speedMs ?? state.gameSpeedMs ?? 160) + " ms");
  setChecked("gameUseMpuControl", state.game?.useMpuControl ?? state.gameUseMpuControl);
  setText("gameRunState", gameStates[state.game?.runState ?? 0] || "--");
  setText("gameScore", state.game?.score ?? "--");
  setText("gameHighScore", state.game?.highScore ?? "--");
  const gameType = state.game?.type ?? state.gameType ?? 0;
  const bricks = Number(state.game?.breakoutBricks ?? 0) >>> 0;
  setText("gameSnakeLen", gameType === 4 ? bricks.toString(2).replace(/0/g, "").length : (gameType === 0 ? (state.game?.snakeLen ?? "--") : "--"));
  setValue("brightness", state.manualBrightness);
  setText("brightnessValue", state.manualBrightness);
  setChecked("autoBrightness", state.autoBrightness);
  setValue("low", state.lowLightThreshold);
  setText("lowValue", state.lowLightThreshold);
  setValue("high", state.highLightThreshold);
  setText("highValue", state.highLightThreshold);
  setValue("particles", state.fluidParticles);
  setText("particlesValue", state.fluidParticles);
  setValue("flip", Math.round(state.fluidFlipRatio * 100));
  setText("flipValue", Math.round(state.fluidFlipRatio * 100) + "%");
  setChecked("separate", state.fluidSeparateParticles);
  setChecked("compensate", state.fluidCompensateDrift);

  setText("audioRmsPeak", fixed(state.rms, 4) + " / " + fixed(state.peak, 4));
  setText("audioBands", fixed(state.audioLowEnergy, 3) + " / " + fixed(state.audioMidEnergy, 3) + " / " + fixed(state.audioHighEnergy, 3));
  setText("audioMic", state.micRaw + " [" + state.micMin + "-" + state.micMax + "]");
  setText("audioSource", state.usingAnalogFallback ? "analogRead" : "ADC DMA");
  setText("fluidAccel", fixed(state.accelX, 2) + ", " + fixed(state.accelY, 2) + ", " + fixed(state.accelZ, 2));
  setText("fluidMpu", (state.mpuOnline ? "online" : "offline") + " " + state.mpuReadCount + "/" + state.mpuFailCount);
  setText("sensorTemp", Number.isFinite(state.temperatureC) ? state.temperatureC.toFixed(1) + " C" : "--");
  setText("sensorHum", Number.isFinite(state.humidityRh) ? state.humidityRh.toFixed(0) + " %" : "--");
  setText("sensorLdr", state.rawLdr + " / " + state.adaptiveBrightness);
  setText("sensorAccel", fixed(state.accelX, 2) + ", " + fixed(state.accelY, 2) + ", " + fixed(state.accelZ, 2));
  setText("sensorMpu", (state.mpuOnline ? "online" : "offline") + " " + state.mpuReadCount + "/" + state.mpuFailCount);
  setText("powerVbus", fixed(state.vbus, 2) + " V");
  setText("powerHigh", state.highPower ? "12V/high" : "limited");
  setText("powerDcdc", state.dcdcEnabled ? "on" : "off");
  setText("powerLimit", state.maxMilliamps + " mA");
  setText("powerCap", state.brightnessCap + " / 255");
  setText("weatherStatus", state.weather?.online ? "online" : (state.weather?.hasData ? "cached" : "waiting"));
  setText("weatherTemp", state.weather?.hasData ? fixed(state.weather.temperature, 1) + " C" : "--");
  setText("weatherFeels", state.weather?.hasData ? fixed(state.weather.apparentTemperature, 1) + " C" : "--");
  setText("weatherHumidity", state.weather?.hasData ? state.weather.humidity + " %" : "--");
  setText("weatherCodeCloud", state.weather?.hasData ? state.weather.weatherCode + " / " + state.weather.cloudCover + " %" : "--");
  setText("weatherRainWind", state.weather?.hasData ? fixed(state.weather.precipitation, 1) + " mm / " + fixed(state.weather.windSpeed, 1) + " km/h" : "--");
  setText("weatherToday", state.weather?.hasData ? state.weather.todayTempMin + "-" + state.weather.todayTempMax + " C / " + state.weather.todayPrecipProb + "%" : "--");
  setText("weatherLast", fmtAge(state.weather?.lastSuccessMs));
  setText("weatherFail", state.weather?.failCount ?? "--");
  setText("weatherError", state.weather?.lastError || "");
  setText("netStatus", state.wifiConnected ? "connected" : (state.network?.setupAp ? "setup AP" : "offline"));
  setText("netMode", state.network?.mode || state.wifiMode || "--");
  setText("netSsid", state.network?.ssid || state.wifiSsid || "--");
  setText("netIp", state.ip || "--");
  setText("netRssi", state.wifiRssi ? state.wifiRssi + " dBm" : "--");
  setText("netHost", state.network?.hostname || "--");
  setText("netAp", state.network?.setupAp ? ((state.network?.setupApSsid || "PixelClock-Setup") + " / " + (state.network?.apIp || "192.168.4.1")) : "off");

  ensureLiveView();
  [...q("bars").children].forEach((bar, i) => {
    const level = ((state.smoothSpectrum?.[i] ?? state.spectrum?.[i]) || 0);
    bar.style.height = Math.max(3, level * 96) + "px";
  });
}

ensureLiveView();
refresh();
setInterval(refresh, 800);
setInterval(refreshScreen, 1000);
</script>
</body>
</html>
)HTML";
} // namespace WebAssets
