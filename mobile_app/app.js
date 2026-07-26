const MODE_NAMES = ["Clock", "Spectrum", "Fluid", "Text", "Timer", "Weather", "Game"];
const CLOCK_THEMES = ["Classic", "Rainbow", "Breath", "Night", "Minimal"];
const AUDIO_MODES = ["Spectrum", "Mirror", "VU", "Bass", "Fire", "Burst"];
const GAME_STATES = ["Idle", "Running", "Paused", "GameOver"];
const SCENE_REASONS = ["手动场景", "安静时段", "音乐活动", "姿态交互", "计时进行中"];
const DESK_STATES = ["Unknown", "Focus", "Meeting", "Rest", "Away"];
const CUSTOM_SCENE_STORAGE_KEY = "pixelClock.customScenes.v1";
const CUSTOM_SCENE_LIMIT = 12;

const qs = (selector, root = document) => root.querySelector(selector);
const qsa = (selector, root = document) => Array.from(root.querySelectorAll(selector));
const byId = (id) => document.getElementById(id);

const app = {
  connected: false,
  realtimeConnected: false,
  socket: null,
  socketRetryTimer: null,
  pollTimer: null,
  screenTimer: null,
  controlRefreshTimer: null,
  matrixReady: false,
  toastTimer: null,
  customScenes: loadCustomScenes(),
  settings: {
    apiBase: localStorage.getItem("pixelClock.apiBase") || "http://pixel-fluid-clock.local",
    mockMode: localStorage.getItem("pixelClock.mockMode") !== "false",
    pollInterval: Number(localStorage.getItem("pixelClock.pollInterval") || 900),
    authToken: localStorage.getItem("pixelClock.authToken") || "",
    offlineFallback: localStorage.getItem("pixelClock.offlineFallback") !== "false",
  },
  state: createMockState(),
};

function createMockState() {
  return {
    mode: 0,
    modeName: "Clock",
    clockTheme: 0,
    scrollText: "PIXEL CLOCK",
    scrollSpeedMs: 90,
    scrollRainbow: true,
    audioVisualMode: 0,
    audioSensitivity: 128,
    audioSmoothing: 160,
    audioRainbow: true,
    audioBeatFlash: true,
    audioAutoGain: true,
    smoothTransitions: true,
    transitionStyle: 0,
    transitionDurationMs: 320,
    gammaCorrection: true,
    smartScenes: false,
    deskAiEnabled: true,
    deskAiAutoScene: false,
    quietStartHour: 23,
    quietEndHour: 7,
    nightBrightnessCap: 22,
    effectiveMode: 0,
    sceneReason: 0,
    quietHours: false,
    darkEnvironment: false,
    audioActive: false,
    motionActive: false,
    audioNoiseFloor: 0.012,
    audioAutoGainValue: 1,
    audioSignalPresent: true,
    notificationActive: false,
    notificationQueueCount: 0,
    weatherEnabled: true,
    weatherDisplayMode: 0,
    weatherCity: "Lanzhou",
    weatherLatitude: 36.0611,
    weatherLongitude: 103.8343,
    weatherUpdateIntervalMin: 30,
    gameType: 0,
    gameUseMpuControl: false,
    gameSpeedMs: 160,
    timerMode: 0,
    timerState: 0,
    timerDurationSec: 25 * 60,
    timerRemainingSec: 25 * 60,
    stopwatchElapsedSec: 0,
    pomodoroFocusMin: 25,
    pomodoroBreakMin: 5,
    pomodoroIsBreak: false,
    manualBrightness: 64,
    brightness: 64,
    effectiveBrightness: 48,
    autoBrightness: true,
    lowLightThreshold: 900,
    highLightThreshold: 3300,
    fluidParticles: 48,
    fluidFlipRatio: 0.78,
    fluidSeparateParticles: true,
    fluidCompensateDrift: true,
    color: { r: 58, g: 215, b: 255 },
    time: "--:--:--",
    temperatureC: 24.6,
    humidityRh: 46.2,
    rawLdr: 1430,
    adaptiveBrightness: 68,
    accelX: 0.02,
    accelY: -0.04,
    accelZ: 0.98,
    mpuOnline: true,
    vbus: 5.02,
    highPower: false,
    dcdcEnabled: true,
    maxMilliamps: 450,
    brightnessCap: 48,
    rms: 0.18,
    peak: 0.42,
    audioLowEnergy: 0.25,
    audioMidEnergy: 0.17,
    audioHighEnergy: 0.11,
    audioBeat: false,
    micRaw: 2048,
    micMin: 1810,
    micMax: 2288,
    uptimeMs: 0,
    freeHeap: 183240,
    wifiMode: "STA",
    wifiRssi: -54,
    wifiSsid: "Studio-WiFi",
    wifiConnected: true,
    ip: "192.168.1.66",
    network: {
      mode: "STA",
      ssid: "Studio-WiFi",
      ip: "192.168.1.66",
      rssi: -54,
      connected: true,
      hostname: "pixel-fluid-clock",
      setupApSsid: "PixelClock-Setup",
    },
    weather: {
      enabled: true,
      city: "Lanzhou",
      latitude: 36.0611,
      longitude: 103.8343,
      updateIntervalMin: 30,
      online: true,
      hasData: true,
      temperature: 25.3,
      apparentTemperature: 26.1,
      humidity: 42,
      weatherCode: 2,
      precipitation: 0,
      cloudCover: 34,
      windSpeed: 9.2,
      todayTempMax: 29,
      todayTempMin: 18,
      todayPrecipProb: 10,
      failCount: 0,
      lastError: "",
    },
    game: {
      type: 0,
      runState: 0,
      score: 0,
      highScore: 12,
      speedMs: 160,
      useMpuControl: false,
      snakeLen: 3,
      foodX: 22,
      foodY: 4,
      ballX: 16,
      ballY: 4,
    },
    deskAi: {
      enabled: true,
      autoScene: false,
      state: 1,
      label: "Focus",
      confidence: 0.82,
      baselineState: 3,
      baselineLabel: "Rest",
      baselineConfidence: 0.65,
      features: [0.12, 0.08, 0.04, 0.38, 0.13],
      scores: [0.82, 0.31, 0.54, 0.22],
      samples: [8, 8, 8, 8],
      profileCoverage: 4,
      profileQuality: 78,
      profileReady: true,
      centroidSeparation: 0.24,
      minSamplesPerClass: 4,
      recommendedSamplesPerClass: 8,
      inferenceCount: 1,
      inferenceMicros: 124,
      lastCalibrationLabel: "Unknown",
      offlineInferenceCount: 24,
      lastInferenceOffline: false,
      evaluation: {
        total: 8,
        personalizedCorrect: 7,
        baselineCorrect: 4,
        samples: [2, 2, 2, 2],
        confusion: [[2, 0, 0, 0], [0, 2, 0, 0], [0, 1, 1, 0], [0, 0, 0, 1]],
      },
      timeline: [
        [0, 1, 82, false], [30000, 1, 79, false], [60000, 2, 71, false],
        [90000, 2, 76, true], [120000, 3, 68, true], [150000, 1, 83, false],
      ],
    },
    smoothSpectrum: Array.from({ length: 32 }, (_, i) => Math.max(0, Math.sin(i / 4) * 3 + 2)),
    _mockStartedAt: Date.now(),
    _timerStartedAt: 0,
    _timerPausedRemainSec: 25 * 60,
  };
}

function init() {
  byId("apiBase").value = app.settings.apiBase;
  byId("settingsApiBase").value = app.settings.apiBase;
  byId("mockMode").checked = app.settings.mockMode;
  byId("pollInterval").value = app.settings.pollInterval;
  byId("authToken").value = app.settings.authToken;
  byId("offlineFallback").checked = app.settings.offlineFallback;

  initMatrix();
  bindNavigation();
  bindControlPanels();
  bindControls();
  renderCustomScenes();
  renderAll();
  startPolling();
  if (!app.settings.mockMode) {
    setTimeout(() => connectDevice(true), 180);
  }
}

function initMatrix() {
  const matrix = byId("matrix");
  matrix.innerHTML = "";
  for (let i = 0; i < 256; i += 1) {
    const pixel = document.createElement("div");
    pixel.className = "pixel";
    matrix.appendChild(pixel);
  }
  app.matrixReady = true;
}

function bindNavigation() {
  qsa("[data-nav]").forEach((button) => {
    button.addEventListener("click", () => {
      const pageName = button.dataset.nav;
      qsa("[data-nav]").forEach((item) => item.classList.toggle("active", item === button));
      qsa(".page").forEach((page) => page.classList.toggle("active", page.dataset.page === pageName));
    });
  });
}

function bindControlPanels() {
  qsa(".segment").forEach((button) => {
    button.addEventListener("click", () => {
      const target = button.dataset.panel;
      qsa(".segment").forEach((item) => item.classList.toggle("active", item === button));
      qsa(".control-panel").forEach((panel) => panel.classList.toggle("active", panel.id === `panel-${target}`));
    });
  });
}

function bindControls() {
  byId("connectBtn").addEventListener("click", connectDevice);
  byId("discoverBtn").addEventListener("click", discoverDevice);
  byId("refreshBtn").addEventListener("click", refreshState);
  byId("setupBtn").addEventListener("click", () => window.open("http://192.168.4.1", "_blank"));
  byId("showCurrentModeBtn").addEventListener("click", () => switchMode(app.state.mode));

  bindTextValue("apiBase", (value) => setApiBase(value));
  bindTextValue("settingsApiBase", (value) => setApiBase(value));
  byId("mockMode").addEventListener("change", (event) => {
    app.settings.mockMode = event.target.checked;
    localStorage.setItem("pixelClock.mockMode", String(app.settings.mockMode));
    app.connected = false;
    disconnectRealtime();
    updateConnectionUi();
    renderAll();
  });
  byId("offlineFallback").addEventListener("change", (event) => {
    app.settings.offlineFallback = event.target.checked;
    localStorage.setItem("pixelClock.offlineFallback", String(app.settings.offlineFallback));
  });
  byId("pollInterval").addEventListener("change", (event) => {
    app.settings.pollInterval = clamp(Number(event.target.value), 300, 5000);
    localStorage.setItem("pixelClock.pollInterval", String(app.settings.pollInterval));
    startPolling();
  });
  byId("authToken").addEventListener("change", (event) => {
    app.settings.authToken = event.target.value.trim();
    localStorage.setItem("pixelClock.authToken", app.settings.authToken);
  });

  qsa("[data-mode]").forEach((button) => {
    button.addEventListener("click", () => switchMode(Number(button.dataset.mode)));
  });

  bindSelect("modeSelect", (value) => postControl({ mode: Number(value) }));
  bindRange(
    "brightness",
    "brightnessValue",
    (value) => postControl({ manualBrightness: Number(value), autoBrightness: false }),
  );
  bindCheckbox("autoBrightness", (checked) => postControl({ autoBrightness: checked }));
  bindCheckbox("smartScenes", (checked) => postControl({ smartScenes: checked }));
  bindCheckbox("deskAiEnabled", (checked) => postControl({ deskAiEnabled: checked }));
  bindCheckbox("deskAiAutoScene", (checked) => postControl({ deskAiAutoScene: checked }));
  qsa("[data-ai-label]").forEach((button) => {
    button.addEventListener("click", async () => {
      const ok = await postControl({ deskAiCalibration: Number(button.dataset.aiLabel) });
      if (ok) toast("Calibration sample saved");
    });
  });
  byId("deskAiResetProfile").addEventListener("click", () => confirmAction(
    "Reset all local Desk AI calibration samples?",
    async () => {
      const ok = await postControl({ deskAiResetProfile: true });
      if (ok) toast("Calibration profile reset");
    },
  ));
  qsa("[data-ai-evaluation]").forEach((button) => {
    button.addEventListener("click", async () => {
      const ok = await postControl({ deskAiEvaluationLabel: Number(button.dataset.aiEvaluation) });
      if (ok) toast("Ground-truth sample recorded");
    });
  });
  byId("deskAiResetEvaluation").addEventListener("click", () => confirmAction(
    "Reset this validation run and its confusion matrix?",
    async () => {
      const ok = await postControl({ deskAiResetEvaluation: true });
      if (ok) toast("Validation run reset");
    },
  ));
  bindCheckbox("gammaCorrection", (checked) => postControl({ gammaCorrection: checked }));
  bindCheckbox("smoothTransitions", (checked) => postControl({ smoothTransitions: checked }));
  bindSelect("transitionStyle", (value) => postControl({ transitionStyle: Number(value) }));
  bindRange(
    "transitionDurationMs",
    "transitionDurationValue",
    (value) => postControl({ transitionDurationMs: Number(value) }),
    (value) => `${value} ms`,
  );
  bindNumber("lowLightThreshold", (value) => postControl({ lowLightThreshold: Number(value) }));
  bindNumber("highLightThreshold", (value) => postControl({ highLightThreshold: Number(value) }));
  bindNumber("quietStartHour", (value) => postControl({ quietStartHour: Number(value) }));
  bindNumber("quietEndHour", (value) => postControl({ quietEndHour: Number(value) }));
  bindRange(
    "nightBrightnessCap",
    "nightBrightnessCapValue",
    (value) => postControl({ nightBrightnessCap: Number(value) }),
  );

  byId("colorPicker").addEventListener("change", (event) => setColor(event.target.value));
  qsa(".swatch").forEach((button) => {
    button.addEventListener("click", () => setColor(button.dataset.color));
  });

  bindSelect("clockTheme", (value) => postControl({ clockTheme: Number(value), mode: 0 }));
  qsa("[data-clock-theme]").forEach((button) => {
    button.addEventListener("click", () => postControl({ clockTheme: Number(button.dataset.clockTheme), mode: 0 }));
  });

  bindSelect("audioVisualMode", (value) => postControl({ audioVisualMode: Number(value), mode: 1 }));
  bindRange("audioSensitivity", "audioSensitivityValue", (value) => postControl({ audioSensitivity: Number(value) }));
  bindRange("audioSmoothing", "audioSmoothingValue", (value) => postControl({ audioSmoothing: Number(value) }));
  bindCheckbox("audioRainbow", (checked) => postControl({ audioRainbow: checked }));
  bindCheckbox("audioBeatFlash", (checked) => postControl({ audioBeatFlash: checked }));
  bindCheckbox("audioAutoGain", (checked) => postControl({ audioAutoGain: checked }));

  bindRange("fluidParticles", "fluidParticlesValue", (value) => postControl({ fluidParticles: Number(value), mode: 2 }));
  bindRange("fluidFlipRatio", "fluidFlipRatioValue", (value) => postControl({ fluidFlipRatio: Number(value) / 100, mode: 2 }), (value) => `${value}%`);
  bindCheckbox("fluidSeparateParticles", (checked) => postControl({ fluidSeparateParticles: checked }));
  bindCheckbox("fluidCompensateDrift", (checked) => postControl({ fluidCompensateDrift: checked }));

  bindTextValue("scrollText", (value) => postControl({ scrollText: value }));
  bindRange("scrollSpeedMs", "scrollSpeedValue", (value) => postControl({ scrollSpeedMs: Number(value) }), (value) => `${value} ms`);
  bindCheckbox("scrollRainbow", (checked) => postControl({ scrollRainbow: checked }));
  qsa("[data-phrase]").forEach((button) => {
    button.addEventListener("click", () => {
      byId("scrollText").value = button.dataset.phrase;
      postControl({ scrollText: button.dataset.phrase, mode: 3 });
    });
  });
  byId("sendTextBtn").addEventListener("click", () => postControl({ mode: 3, scrollText: byId("scrollText").value }));
  byId("sendNotifyBtn").addEventListener("click", sendNotification);

  bindSelect("timerMode", (value) => postControl({ timerMode: Number(value), mode: 4 }));
  bindNumber("timerMinutes", (value) => postControl({ timerMinutes: Number(value), mode: 4 }));
  bindNumber("pomodoroFocusMin", (value) => postControl({ pomodoroFocusMin: Number(value), mode: 4 }));
  bindNumber("pomodoroBreakMin", (value) => postControl({ pomodoroBreakMin: Number(value), mode: 4 }));
  qsa("[data-timer-action]").forEach((button) => {
    button.addEventListener("click", () => postControl({ timerAction: button.dataset.timerAction, mode: 4 }));
  });

  bindCheckbox("weatherEnabled", (checked) => postControl({ weatherEnabled: checked }));
  bindSelect("weatherDisplayMode", (value) => postControl({ weatherDisplayMode: Number(value) }));
  bindTextValue("weatherCity", (value) => postControl({ weatherCity: value }));
  bindNumber("weatherLatitude", (value) => postControl({ weatherLatitude: Number(value) }));
  bindNumber("weatherLongitude", (value) => postControl({ weatherLongitude: Number(value) }));
  bindNumber("weatherUpdateIntervalMin", (value) => postControl({ weatherUpdateIntervalMin: Number(value) }));
  byId("showWeatherBtn").addEventListener("click", () => postControl({ mode: 5 }));
  byId("refreshWeatherBtn").addEventListener("click", () => postControl({ weatherRefresh: true, mode: 5 }));

  bindSelect("gameType", (value) => postControl({ gameType: Number(value), mode: 6 }));
  bindRange("gameSpeedMs", "gameSpeedValue", (value) => postControl({ gameSpeedMs: Number(value) }), (value) => `${value} ms`);
  bindCheckbox("gameUseMpuControl", (checked) => postControl({ gameUseMpuControl: checked }));
  qsa("[data-game-action]").forEach((button) => {
    button.addEventListener("click", () => postGameAction(button.dataset.gameAction));
  });
  qsa("[data-direction]").forEach((button) => {
    button.addEventListener("click", () => postGameDirection(button.dataset.direction));
  });

  qsa("[data-preset]").forEach((button) => {
    button.addEventListener("click", () => applyPreset(button.dataset.preset));
  });
  byId("saveCustomSceneBtn").addEventListener("click", saveCurrentCustomScene);

  byId("saveSettingsBtn").addEventListener("click", () => devicePost("/api/save-settings", null, "设置已保存"));
  byId("exportConfigBtn").addEventListener("click", exportConfig);
  byId("importConfigBtn").addEventListener("click", importConfig);
  byId("wifiScanBtn").addEventListener("click", scanWifi);
  byId("wifiResetBtn").addEventListener("click", () => confirmAction("清除 Wi-Fi 并重启设备？", () => devicePost("/api/wifi/reset", null, "设备将重启配网")));
  byId("rebootBtn").addEventListener("click", () => confirmAction("重启设备？", () => devicePost("/api/reboot", null, "重启命令已发送")));
}

function bindSelect(id, handler) {
  byId(id).addEventListener("change", (event) => handler(event.target.value));
}

function bindCheckbox(id, handler) {
  byId(id).addEventListener("change", (event) => handler(event.target.checked));
}

function bindNumber(id, handler) {
  byId(id).addEventListener("change", (event) => handler(event.target.value));
}

function bindTextValue(id, handler) {
  byId(id).addEventListener("change", (event) => handler(event.target.value));
}

function bindRange(id, displayId, handler, formatter = (value) => value) {
  const input = byId(id);
  const label = byId(displayId);
  let timer = null;
  input.addEventListener("input", (event) => {
    label.textContent = formatter(event.target.value);
    clearTimeout(timer);
    timer = setTimeout(() => handler(event.target.value), 80);
  });
  input.addEventListener("change", (event) => {
    clearTimeout(timer);
    label.textContent = formatter(event.target.value);
    handler(event.target.value);
  });
}

function setApiBase(value) {
  const cleaned = value.trim().replace(/\/+$/, "");
  const nextBase = cleaned || "http://pixel-fluid-clock.local";
  if (nextBase !== app.settings.apiBase) {
    disconnectRealtime();
  }
  app.settings.apiBase = nextBase;
  localStorage.setItem("pixelClock.apiBase", app.settings.apiBase);
  byId("apiBase").value = app.settings.apiBase;
  byId("settingsApiBase").value = app.settings.apiBase;
}

function setColor(hex) {
  const rgb = hexToRgb(hex);
  byId("colorPicker").value = hex;
  const patch = { color: rgb };
  if (app.state.mode === 0 && app.state.clockTheme !== 0 && app.state.clockTheme !== 2) {
    patch.clockTheme = 0;
  }
  if (app.state.mode === 1) {
    patch.audioRainbow = false;
  }
  if (app.state.mode === 3) {
    patch.scrollRainbow = false;
  }
  postControl(patch);
  toast("主颜色已应用");
}

function switchMode(mode) {
  postControl({ mode });
}

async function connectDevice() {
  const silent = arguments[0] === true;
  setApiBase(byId("apiBase").value);
  if (app.settings.mockMode) {
    byId("mockMode").checked = false;
    app.settings.mockMode = false;
    localStorage.setItem("pixelClock.mockMode", "false");
  }
  try {
    const data = await fetchJson("/api/state");
    receiveState(data);
    app.connected = true;
    connectRealtime();
    if (!silent) toast("设备已连接");
  } catch (error) {
    app.connected = false;
    disconnectRealtime();
    if (!silent) toast(`连接失败：${shortError(error)}`);
  }
  updateConnectionUi();
  renderAll();
}

async function discoverDevice() {
  const candidates = Array.from(new Set([
    byId("apiBase").value.trim().replace(/\/+$/, ""),
    app.settings.apiBase,
    "http://pixel-fluid-clock.local",
    "http://192.168.4.1",
  ].filter(Boolean)));

  byId("discoverBtn").disabled = true;
  byId("discoverBtn").textContent = "...";
  toast("正在查找设备");
  try {
    for (const candidate of candidates) {
      try {
        const data = await fetchJsonFrom(candidate, "/api/state", {}, 2600);
        setApiBase(candidate);
        byId("mockMode").checked = false;
        app.settings.mockMode = false;
        localStorage.setItem("pixelClock.mockMode", "false");
        receiveState(data);
        app.connected = true;
        connectRealtime();
        renderAll();
        toast(`已找到 ${data.ip || candidate}`);
        return;
      } catch (_) {
        // Try the next well-known address.
      }
    }
    toast("未找到设备，请确认手机与设备在同一 Wi-Fi");
  } finally {
    byId("discoverBtn").disabled = false;
    byId("discoverBtn").textContent = "查找";
  }
}

function connectRealtime() {
  disconnectRealtime(false);
  if (app.settings.mockMode || !app.connected || !window.WebSocket) {
    return;
  }

  const base = app.settings.apiBase.replace(/\/+$/, "");
  const socketUrl = `${base.replace(/^http/i, "ws")}/ws`;
  try {
    const socket = new WebSocket(socketUrl);
    app.socket = socket;
    socket.onopen = () => {
      app.realtimeConnected = true;
      updateConnectionUi();
    };
    socket.onmessage = (event) => {
      try {
        const previousSignature = displayControlSignature(app.state);
        receiveState(JSON.parse(event.data));
        updateTimerDerived();
        updateConnectionUi();
        renderStateText();
        renderControls();
        if (previousSignature !== displayControlSignature(app.state)) {
          scheduleScreenRefresh(30);
        }
      } catch (_) {
        // Ignore a malformed frame and keep the connection alive.
      }
    };
    socket.onerror = () => {
      app.realtimeConnected = false;
      updateConnectionUi();
    };
    socket.onclose = () => {
      if (app.socket === socket) app.socket = null;
      app.realtimeConnected = false;
      updateConnectionUi();
      if (app.connected && !app.settings.mockMode) {
        clearTimeout(app.socketRetryTimer);
        app.socketRetryTimer = setTimeout(connectRealtime, 2200);
      }
    };
  } catch (_) {
    app.realtimeConnected = false;
  }
}

function disconnectRealtime(cancelRetry = true) {
  if (cancelRetry) {
    clearTimeout(app.socketRetryTimer);
    app.socketRetryTimer = null;
  }
  const socket = app.socket;
  app.socket = null;
  app.realtimeConnected = false;
  if (socket) {
    socket.onclose = null;
    socket.close();
  }
}

async function refreshState() {
  if (app.settings.mockMode || !app.connected) {
    renderAll();
    return;
  }
  try {
    const data = await fetchJson("/api/state");
    receiveState(data);
  } catch (error) {
    app.connected = false;
    if (app.settings.offlineFallback) {
      tickMockState();
    }
    toast(`状态刷新失败：${shortError(error)}`);
  }
  updateConnectionUi();
  renderAll();
}

function receiveState(data) {
  const normalized = normalizeState(data);
  app.state = { ...app.state, ...normalized };
  app.state.network = { ...app.state.network, ...(normalized.network || {}) };
  app.state.weather = { ...app.state.weather, ...(normalized.weather || {}) };
  app.state.game = { ...app.state.game, ...(normalized.game || {}) };
  app.state.deskAi = { ...app.state.deskAi, ...(normalized.deskAi || {}) };
}

function normalizeState(data) {
  const color = Array.isArray(data.color)
    ? { r: data.color[0] || 0, g: data.color[1] || 0, b: data.color[2] || 0 }
    : data.color || app.state.color;

  const manualBrightness = data.manualBrightness ?? data.brightness ?? app.state.manualBrightness;
  const brightnessCap = data.brightnessCap ?? data.power?.brightnessCap ?? app.state.brightnessCap ?? 255;
  const autoBrightness = data.autoBrightness ?? app.state.autoBrightness;
  const effectiveBrightness = data.effectiveBrightness ??
    Math.min(autoBrightness ? (data.adaptiveBrightness ?? app.state.adaptiveBrightness) : manualBrightness, brightnessCap);

  return {
    ...data,
    color,
    brightness: manualBrightness,
    manualBrightness,
    autoBrightness,
    effectiveBrightness,
    modeName: data.modeName || MODE_NAMES[data.mode] || app.state.modeName,
    network: data.network || {
      mode: data.wifiMode,
      ssid: data.wifiSsid,
      ip: data.ip,
      rssi: data.wifiRssi,
      connected: data.wifiConnected,
      hostname: "pixel-fluid-clock",
    },
    weather: data.weather || app.state.weather,
    game: data.game || app.state.game,
    deskAiEnabled: data.deskAiEnabled ?? data.deskAi?.enabled ?? app.state.deskAiEnabled,
    deskAiAutoScene: data.deskAiAutoScene ?? data.deskAi?.autoScene ?? app.state.deskAiAutoScene,
    deskAi: data.deskAi || app.state.deskAi,
  };
}

async function postControl(patch) {
  applyLocalControlPatch(patch);
  renderOptimisticState();
  if (app.settings.mockMode || !app.connected) {
    toast("已更新模拟状态");
    return true;
  }
  try {
    await fetchJson("/api/control", {
      method: "POST",
      body: JSON.stringify(patch),
    });
    scheduleScreenRefresh(40);
    return true;
  } catch (error) {
    app.connected = false;
    toast(`控制失败：${shortError(error)}`);
    updateConnectionUi();
    return false;
  }
}

async function sendNotification() {
  const text = byId("scrollText").value.trim();
  if (!text) {
    toast("请先输入通知文字");
    return;
  }
  if (app.settings.mockMode || !app.connected) {
    app.state.notificationActive = true;
    app.state.notificationText = text;
    toast("已显示模拟通知");
    return;
  }
  try {
    await fetchJson("/api/notify", {
      method: "POST",
      body: JSON.stringify({
        text,
        color: app.state.color,
        durationMs: 6000,
        speedMs: Math.min(180, Number(app.state.scrollSpeedMs || 72)),
      }),
    });
    toast("通知已加入显示队列");
  } catch (error) {
    toast(`通知失败：${shortError(error)}`);
  }
}

function applyLocalControlPatch(patch) {
  const s = app.state;
  if (patch.mode !== undefined) {
    s.mode = Number(patch.mode);
    s.modeName = MODE_NAMES[s.mode] || "Clock";
  }
  if (patch.clockTheme !== undefined) s.clockTheme = Number(patch.clockTheme);
  if (patch.brightness !== undefined) {
    s.brightness = clamp(Number(patch.brightness), 1, 255);
    s.manualBrightness = s.brightness;
  }
  if (patch.manualBrightness !== undefined) {
    s.manualBrightness = clamp(Number(patch.manualBrightness), 1, 255);
    s.brightness = s.manualBrightness;
  }
  if (patch.autoBrightness !== undefined) s.autoBrightness = Boolean(patch.autoBrightness);
  if (patch.lowLightThreshold !== undefined) s.lowLightThreshold = Number(patch.lowLightThreshold);
  if (patch.highLightThreshold !== undefined) s.highLightThreshold = Number(patch.highLightThreshold);
  if (patch.fluidParticles !== undefined) s.fluidParticles = Number(patch.fluidParticles);
  if (patch.fluidFlipRatio !== undefined) s.fluidFlipRatio = Number(patch.fluidFlipRatio);
  if (patch.fluidSeparateParticles !== undefined) s.fluidSeparateParticles = Boolean(patch.fluidSeparateParticles);
  if (patch.fluidCompensateDrift !== undefined) s.fluidCompensateDrift = Boolean(patch.fluidCompensateDrift);
  if (patch.scrollText !== undefined) s.scrollText = String(patch.scrollText).slice(0, 63);
  if (patch.scrollSpeedMs !== undefined) s.scrollSpeedMs = Number(patch.scrollSpeedMs);
  if (patch.scrollRainbow !== undefined) s.scrollRainbow = Boolean(patch.scrollRainbow);
  if (patch.audioVisualMode !== undefined) s.audioVisualMode = Number(patch.audioVisualMode);
  if (patch.audioSensitivity !== undefined) s.audioSensitivity = Number(patch.audioSensitivity);
  if (patch.audioSmoothing !== undefined) s.audioSmoothing = Number(patch.audioSmoothing);
  if (patch.audioRainbow !== undefined) s.audioRainbow = Boolean(patch.audioRainbow);
  if (patch.audioBeatFlash !== undefined) s.audioBeatFlash = Boolean(patch.audioBeatFlash);
  if (patch.audioAutoGain !== undefined) s.audioAutoGain = Boolean(patch.audioAutoGain);
  if (patch.smoothTransitions !== undefined) s.smoothTransitions = Boolean(patch.smoothTransitions);
  if (patch.transitionStyle !== undefined) s.transitionStyle = Number(patch.transitionStyle);
  if (patch.transitionDurationMs !== undefined) s.transitionDurationMs = Number(patch.transitionDurationMs);
  if (patch.gammaCorrection !== undefined) s.gammaCorrection = Boolean(patch.gammaCorrection);
  if (patch.smartScenes !== undefined) s.smartScenes = Boolean(patch.smartScenes);
  if (patch.deskAiEnabled !== undefined) {
    s.deskAiEnabled = Boolean(patch.deskAiEnabled);
    s.deskAi.enabled = s.deskAiEnabled;
  }
  if (patch.deskAiAutoScene !== undefined) {
    s.deskAiAutoScene = Boolean(patch.deskAiAutoScene);
    s.deskAi.autoScene = s.deskAiAutoScene;
  }
  if (patch.deskAiCalibration !== undefined) {
    const label = Number(patch.deskAiCalibration);
    const samples = [...(s.deskAi.samples || [0, 0, 0, 0])];
    samples[label - 1] = (samples[label - 1] || 0) + 1;
    s.deskAi.samples = samples;
    s.deskAi.lastCalibrationLabel = DESK_STATES[label];
  }
  if (patch.deskAiResetProfile) {
    s.deskAi.samples = [0, 0, 0, 0];
    s.deskAi.lastCalibrationLabel = "Unknown";
  }
  if (patch.deskAiEvaluationLabel !== undefined) {
    const actual = Number(patch.deskAiEvaluationLabel) - 1;
    const evaluation = s.deskAi.evaluation || { total: 0, personalizedCorrect: 0, baselineCorrect: 0, samples: [0, 0, 0, 0], confusion: [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]] };
    const predicted = Math.max(0, Number(s.deskAi.state || 1) - 1);
    const baseline = Math.max(0, Number(s.deskAi.baselineState || 1) - 1);
    evaluation.total += 1;
    evaluation.samples[actual] += 1;
    evaluation.confusion[actual][predicted] += 1;
    if (actual === predicted) evaluation.personalizedCorrect += 1;
    if (actual === baseline) evaluation.baselineCorrect += 1;
    s.deskAi.evaluation = evaluation;
  }
  if (patch.deskAiResetEvaluation) {
    s.deskAi.evaluation = { total: 0, personalizedCorrect: 0, baselineCorrect: 0, samples: [0, 0, 0, 0], confusion: [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]] };
  }
  if (patch.quietStartHour !== undefined) s.quietStartHour = clamp(Number(patch.quietStartHour), 0, 23);
  if (patch.quietEndHour !== undefined) s.quietEndHour = clamp(Number(patch.quietEndHour), 0, 23);
  if (patch.nightBrightnessCap !== undefined) s.nightBrightnessCap = clamp(Number(patch.nightBrightnessCap), 1, 96);
  if (patch.weatherEnabled !== undefined) {
    s.weatherEnabled = Boolean(patch.weatherEnabled);
    s.weather.enabled = s.weatherEnabled;
  }
  if (patch.weatherDisplayMode !== undefined) s.weatherDisplayMode = Number(patch.weatherDisplayMode);
  if (patch.weatherCity !== undefined) {
    s.weatherCity = String(patch.weatherCity).slice(0, 31);
    s.weather.city = s.weatherCity;
  }
  if (patch.weatherLatitude !== undefined) {
    s.weatherLatitude = Number(patch.weatherLatitude);
    s.weather.latitude = s.weatherLatitude;
  }
  if (patch.weatherLongitude !== undefined) {
    s.weatherLongitude = Number(patch.weatherLongitude);
    s.weather.longitude = s.weatherLongitude;
  }
  if (patch.weatherUpdateIntervalMin !== undefined) s.weatherUpdateIntervalMin = Number(patch.weatherUpdateIntervalMin);
  if (patch.gameType !== undefined) {
    s.gameType = Number(patch.gameType);
    s.game.type = s.gameType;
  }
  if (patch.gameUseMpuControl !== undefined) {
    s.gameUseMpuControl = Boolean(patch.gameUseMpuControl);
    s.game.useMpuControl = s.gameUseMpuControl;
  }
  if (patch.gameSpeedMs !== undefined) {
    s.gameSpeedMs = Number(patch.gameSpeedMs);
    s.game.speedMs = s.gameSpeedMs;
  }
  if (patch.timerMode !== undefined) s.timerMode = Number(patch.timerMode);
  if (patch.timerMinutes !== undefined) {
    s.timerDurationSec = clamp(Number(patch.timerMinutes), 1, 99) * 60;
    s.timerRemainingSec = s.timerDurationSec;
    s._timerPausedRemainSec = s.timerDurationSec;
  }
  if (patch.pomodoroFocusMin !== undefined) s.pomodoroFocusMin = clamp(Number(patch.pomodoroFocusMin), 1, 99);
  if (patch.pomodoroBreakMin !== undefined) s.pomodoroBreakMin = clamp(Number(patch.pomodoroBreakMin), 1, 60);
  if (patch.timerAction !== undefined) applyLocalTimerAction(patch.timerAction);
  if (patch.color !== undefined) {
    const c = Array.isArray(patch.color)
      ? { r: patch.color[0], g: patch.color[1], b: patch.color[2] }
      : patch.color;
    s.color = { r: clamp(c.r, 0, 255), g: clamp(c.g, 0, 255), b: clamp(c.b, 0, 255) };
  }
  const requestedBrightness = s.autoBrightness ? s.adaptiveBrightness : s.manualBrightness;
  s.effectiveBrightness = Math.min(
    requestedBrightness,
    s.brightnessCap ?? 255,
    s.smartScenes && (s.quietHours || s.darkEnvironment) ? s.nightBrightnessCap : 255,
  );
  if (patch.weatherRefresh) {
    s.weather.online = true;
    s.weather.hasData = true;
    s.weather.failCount = 0;
    s.weather.lastError = "";
    s.weather.temperature = Number((s.weather.temperature + (Math.random() - 0.5)).toFixed(1));
  }
}

function applyLocalTimerAction(action) {
  const s = app.state;
  const now = Date.now();
  if (action === "start") {
    if (s.timerMode === 0) {
      s.timerDurationSec = (s.pomodoroIsBreak ? s.pomodoroBreakMin : s.pomodoroFocusMin) * 60;
      s.timerRemainingSec = s.timerDurationSec;
    }
    s.timerState = 1;
    s._timerStartedAt = now;
  } else if (action === "pause") {
    updateTimerDerived();
    s.timerState = 2;
    s._timerPausedRemainSec = s.timerRemainingSec;
  } else if (action === "resume") {
    s.timerState = 1;
    s.timerDurationSec = s._timerPausedRemainSec || s.timerRemainingSec;
    s.timerRemainingSec = s.timerDurationSec;
    s._timerStartedAt = now;
  } else if (action === "reset") {
    s.timerState = 0;
    s.stopwatchElapsedSec = 0;
    s.timerDurationSec = s.timerMode === 0 ? s.pomodoroFocusMin * 60 : s.timerDurationSec;
    s.timerRemainingSec = s.timerDurationSec;
    s._timerPausedRemainSec = s.timerDurationSec;
  }
}

async function postGameAction(action) {
  applyLocalGameAction(action);
  renderAll();
  if (app.settings.mockMode || !app.connected) {
    toast("游戏状态已更新");
    return;
  }
  try {
    await fetchJson("/api/game/action", { method: "POST", body: JSON.stringify({ action }) });
  } catch (error) {
    app.connected = false;
    toast(`游戏命令失败：${shortError(error)}`);
    updateConnectionUi();
  }
}

async function postGameDirection(direction) {
  nudgeMockGame(direction);
  renderAll();
  if (app.settings.mockMode || !app.connected) {
    return;
  }
  try {
    await fetchJson("/api/game/direction", { method: "POST", body: JSON.stringify({ direction }) });
  } catch (error) {
    app.connected = false;
    toast(`方向命令失败：${shortError(error)}`);
    updateConnectionUi();
  }
}

function applyLocalGameAction(action) {
  const game = app.state.game;
  app.state.mode = 6;
  app.state.modeName = "Game";
  if (action === "start") game.runState = 1;
  if (action === "pause") game.runState = 2;
  if (action === "resume") game.runState = 1;
  if (action === "reset") {
    game.runState = 0;
    game.score = 0;
    game.snakeLen = 3;
    game.foodX = 22;
    game.foodY = 4;
    game.ballX = 16;
    game.ballY = 4;
  }
  if (action === "toggle") {
    game.runState = game.runState === 1 ? 2 : 1;
  }
}

function nudgeMockGame(direction) {
  const game = app.state.game;
  app.state.mode = 6;
  if (direction === "left") game.ballX = clamp(game.ballX - 1, 0, 31);
  if (direction === "right") game.ballX = clamp(game.ballX + 1, 0, 31);
  if (direction === "up") game.ballY = clamp(game.ballY - 1, 0, 7);
  if (direction === "down") game.ballY = clamp(game.ballY + 1, 0, 7);
  game.score = Number(game.score || 0) + (Math.random() > 0.72 ? 1 : 0);
  game.highScore = Math.max(game.highScore || 0, game.score || 0);
}

async function applyPreset(preset) {
  applyLocalPreset(preset);
  renderOptimisticState();
  if (app.settings.mockMode || !app.connected) {
    toast("场景已应用");
    return;
  }
  try {
    await fetchJson("/api/preset", { method: "POST", body: JSON.stringify({ preset }) });
    scheduleScreenRefresh(40);
    toast("场景已发送");
  } catch (error) {
    app.connected = false;
    toast(`场景失败：${shortError(error)}`);
    updateConnectionUi();
  }
}

function applyLocalPreset(preset) {
  const presets = {
    desk: { mode: 0, clockTheme: 0, autoBrightness: true, brightness: 64, color: { r: 58, g: 215, b: 255 } },
    night: { mode: 0, clockTheme: 3, autoBrightness: false, brightness: 12, color: { r: 42, g: 14, b: 4 } },
    music: { mode: 1, audioVisualMode: 3, audioSensitivity: 176, audioSmoothing: 118, audioRainbow: true, audioBeatFlash: true },
    fluid: { mode: 2, fluidParticles: 48, fluidFlipRatio: 0.78, fluidSeparateParticles: true, fluidCompensateDrift: true },
    focus: { mode: 4, timerMode: 0, brightness: 44, autoBrightness: false, color: { r: 180, g: 40, b: 20 } },
    weather: { mode: 5, weatherEnabled: true, weatherDisplayMode: 0 },
    game: { mode: 6, gameType: 0, gameSpeedMs: 160, gameUseMpuControl: false },
  };
  applyLocalControlPatch({ smartScenes: false, ...(presets[preset] || {}) });
}

function loadCustomScenes() {
  try {
    const parsed = JSON.parse(localStorage.getItem(CUSTOM_SCENE_STORAGE_KEY) || "[]");
    if (!Array.isArray(parsed)) return [];
    return parsed
      .filter((scene) => scene && typeof scene.name === "string" && scene.patch)
      .slice(0, CUSTOM_SCENE_LIMIT);
  } catch (_) {
    return [];
  }
}

function persistCustomScenes() {
  localStorage.setItem(CUSTOM_SCENE_STORAGE_KEY, JSON.stringify(app.customScenes));
}

function captureCurrentScenePatch() {
  const s = app.state;
  return {
    smartScenes: false,
    mode: Number(s.mode),
    clockTheme: Number(s.clockTheme),
    manualBrightness: Number(s.manualBrightness),
    autoBrightness: Boolean(s.autoBrightness),
    color: { ...s.color },
    smoothTransitions: Boolean(s.smoothTransitions),
    transitionStyle: Number(s.transitionStyle),
    transitionDurationMs: Number(s.transitionDurationMs),
    gammaCorrection: Boolean(s.gammaCorrection),
    audioVisualMode: Number(s.audioVisualMode),
    audioSensitivity: Number(s.audioSensitivity),
    audioSmoothing: Number(s.audioSmoothing),
    audioRainbow: Boolean(s.audioRainbow),
    audioBeatFlash: Boolean(s.audioBeatFlash),
    audioAutoGain: Boolean(s.audioAutoGain),
    fluidParticles: Number(s.fluidParticles),
    fluidFlipRatio: Number(s.fluidFlipRatio),
    fluidSeparateParticles: Boolean(s.fluidSeparateParticles),
    fluidCompensateDrift: Boolean(s.fluidCompensateDrift),
    scrollText: String(s.scrollText || "").slice(0, 63),
    scrollSpeedMs: Number(s.scrollSpeedMs),
    scrollRainbow: Boolean(s.scrollRainbow),
    timerMode: Number(s.timerMode),
    pomodoroFocusMin: Number(s.pomodoroFocusMin),
    pomodoroBreakMin: Number(s.pomodoroBreakMin),
    weatherEnabled: Boolean(s.weatherEnabled),
    weatherDisplayMode: Number(s.weatherDisplayMode),
    weatherCity: String(s.weatherCity || "").slice(0, 31),
    weatherLatitude: Number(s.weatherLatitude),
    weatherLongitude: Number(s.weatherLongitude),
    weatherUpdateIntervalMin: Number(s.weatherUpdateIntervalMin),
    gameType: Number(s.gameType),
    gameUseMpuControl: Boolean(s.gameUseMpuControl),
    gameSpeedMs: Number(s.gameSpeedMs),
  };
}

function saveCurrentCustomScene() {
  const input = byId("customSceneName");
  const fallbackName = `我的场景 ${app.customScenes.length + 1}`;
  const name = (input.value.trim() || fallbackName).slice(0, 20);
  const existing = app.customScenes.find((scene) => scene.name.toLowerCase() === name.toLowerCase());
  if (existing) {
    existing.patch = captureCurrentScenePatch();
    existing.updatedAt = Date.now();
  } else {
    if (app.customScenes.length >= CUSTOM_SCENE_LIMIT) {
      toast(`最多保存 ${CUSTOM_SCENE_LIMIT} 个场景`);
      return;
    }
    app.customScenes.push({
      id: `scene-${Date.now()}-${Math.random().toString(16).slice(2, 7)}`,
      name,
      patch: captureCurrentScenePatch(),
      updatedAt: Date.now(),
    });
  }
  persistCustomScenes();
  input.value = "";
  renderCustomScenes();
  toast(existing ? "同名场景已更新" : "当前设置已保存");
}

async function applyCustomScene(sceneId) {
  const scene = app.customScenes.find((item) => item.id === sceneId);
  if (!scene) return;
  const ok = await postControl({ ...scene.patch, smartScenes: false });
  if (ok) toast(`已应用：${scene.name}`);
}

function renameCustomScene(sceneId) {
  const scene = app.customScenes.find((item) => item.id === sceneId);
  if (!scene) return;
  const nextName = window.prompt("场景名称", scene.name);
  if (nextName === null) return;
  const cleaned = nextName.trim().slice(0, 20);
  if (!cleaned) {
    toast("场景名称不能为空");
    return;
  }
  scene.name = cleaned;
  scene.updatedAt = Date.now();
  persistCustomScenes();
  renderCustomScenes();
}

function deleteCustomScene(sceneId) {
  const scene = app.customScenes.find((item) => item.id === sceneId);
  if (!scene || !window.confirm(`删除“${scene.name}”？`)) return;
  app.customScenes = app.customScenes.filter((item) => item.id !== sceneId);
  persistCustomScenes();
  renderCustomScenes();
  toast("场景已删除");
}

function customSceneSummary(patch) {
  const mode = MODE_NAMES[Number(patch.mode)] || "Clock";
  const brightness = patch.autoBrightness ? "自动亮度" : `亮度 ${patch.manualBrightness}`;
  if (Number(patch.mode) === 3) {
    return `${mode} / ${String(patch.scrollText || "").slice(0, 12)} / ${brightness}`;
  }
  if (Number(patch.mode) === 1) {
    return `${mode} / ${AUDIO_MODES[Number(patch.audioVisualMode)] || "Spectrum"} / ${brightness}`;
  }
  return `${mode} / ${brightness}`;
}

function renderCustomScenes() {
  const list = byId("customSceneList");
  const empty = byId("customSceneEmpty");
  if (!list || !empty) return;
  list.replaceChildren();
  byId("customSceneCount").textContent = `${app.customScenes.length} / ${CUSTOM_SCENE_LIMIT}`;
  empty.hidden = app.customScenes.length > 0;

  app.customScenes.forEach((scene) => {
    const item = document.createElement("div");
    item.className = "custom-scene-item";

    const swatch = document.createElement("span");
    swatch.className = "custom-scene-color";
    swatch.style.background = rgbToHex(scene.patch.color);

    const copy = document.createElement("div");
    copy.className = "custom-scene-copy";
    const title = document.createElement("strong");
    title.textContent = scene.name;
    const description = document.createElement("small");
    description.textContent = customSceneSummary(scene.patch);
    copy.append(title, description);

    const actions = document.createElement("div");
    actions.className = "custom-scene-actions";
    const applyButton = document.createElement("button");
    applyButton.type = "button";
    applyButton.textContent = "应用";
    applyButton.addEventListener("click", () => applyCustomScene(scene.id));
    const renameButton = document.createElement("button");
    renameButton.type = "button";
    renameButton.textContent = "改名";
    renameButton.addEventListener("click", () => renameCustomScene(scene.id));
    const deleteButton = document.createElement("button");
    deleteButton.type = "button";
    deleteButton.className = "delete-scene";
    deleteButton.textContent = "删除";
    deleteButton.addEventListener("click", () => deleteCustomScene(scene.id));
    actions.append(applyButton, renameButton, deleteButton);

    item.append(swatch, copy, actions);
    list.appendChild(item);
  });
}

async function devicePost(path, body, successMessage) {
  if (app.settings.mockMode || !app.connected) {
    appendLog(`MOCK POST ${path}`);
    toast(successMessage || "已执行模拟命令");
    return;
  }
  try {
    const data = await fetchJson(path, body ? { method: "POST", body: JSON.stringify(body) } : { method: "POST" });
    appendLog(`${path}\n${JSON.stringify(data, null, 2)}`);
    toast(successMessage || "命令已发送");
  } catch (error) {
    app.connected = false;
    appendLog(`${path}\nERROR: ${shortError(error)}`);
    toast(`命令失败：${shortError(error)}`);
    updateConnectionUi();
  }
}

async function exportConfig() {
  let data = app.state;
  if (!app.settings.mockMode && app.connected) {
    try {
      data = await fetchJson("/api/config");
    } catch (error) {
      toast(`导出失败：${shortError(error)}`);
      return;
    }
  }
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "pixel-clock-config.json";
  anchor.click();
  URL.revokeObjectURL(url);
}

function importConfig() {
  const input = document.createElement("input");
  input.type = "file";
  input.accept = "application/json,.json";
  input.addEventListener("change", async () => {
    const file = input.files?.[0];
    if (!file) return;
    const text = await file.text();
    try {
      const config = JSON.parse(text);
      if (app.settings.mockMode || !app.connected) {
        receiveState(config);
        renderAll();
        toast("配置已导入模拟状态");
      } else {
        await fetchJson("/api/config", { method: "POST", body: JSON.stringify(config) });
        toast("配置已发送到设备");
      }
    } catch (error) {
      toast(`导入失败：${shortError(error)}`);
    }
  });
  input.click();
}

async function scanWifi() {
  if (app.settings.mockMode || !app.connected) {
    appendLog("MOCK Wi-Fi scan\nStudio-WiFi -54 dBm\nPixelClock-Setup AP");
    return;
  }
  try {
    const data = await fetchJson("/api/wifi/scan");
    appendLog(JSON.stringify(data, null, 2));
  } catch (error) {
    appendLog(`Wi-Fi scan error: ${shortError(error)}`);
  }
}

async function fetchJson(path, options = {}) {
  return fetchJsonFrom(app.settings.apiBase, path, options);
}

async function fetchJsonFrom(baseValue, path, options = {}, timeoutMs = 6000) {
  const base = baseValue.replace(/\/+$/, "");
  const headers = {
    Accept: "application/json",
    ...(options.body ? { "Content-Type": "application/json" } : {}),
  };
  if (app.settings.authToken) {
    headers.Authorization = `Bearer ${app.settings.authToken}`;
  }
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), timeoutMs);
  let response;
  try {
    response = await fetch(`${base}${path}`, {
      ...options,
      signal: controller.signal,
      headers: { ...headers, ...(options.headers || {}) },
    });
  } finally {
    clearTimeout(timeout);
  }
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
  const text = await response.text();
  return text ? JSON.parse(text) : { ok: true };
}

function startPolling() {
  clearInterval(app.pollTimer);
  clearInterval(app.screenTimer);
  app.pollTimer = setInterval(() => {
    if (!app.realtimeConnected) refreshState();
  }, app.settings.pollInterval);
  app.screenTimer = setInterval(refreshScreen, 500);
}

function tickMockState() {
  const s = app.state;
  const now = new Date();
  s.time = now.toTimeString().slice(0, 8);
  s.uptimeMs = Date.now() - s._mockStartedAt;
  const t = Date.now() / 1000;
  s.rms = clamp(0.16 + Math.sin(t * 2.1) * 0.09 + Math.random() * 0.04, 0, 1);
  s.peak = clamp(s.rms + Math.random() * 0.28, 0, 1);
  s.audioLowEnergy = clamp(0.3 + Math.sin(t * 3.3) * 0.24 + Math.random() * 0.12, 0, 1);
  s.audioMidEnergy = clamp(0.22 + Math.sin(t * 2.2 + 1.5) * 0.18 + Math.random() * 0.08, 0, 1);
  s.audioHighEnergy = clamp(0.14 + Math.sin(t * 4.1 + 0.8) * 0.12 + Math.random() * 0.07, 0, 1);
  s.audioBeat = Math.random() > 0.88;
  s.micRaw = Math.round(2048 + (Math.random() - 0.5) * 320);
  s.rawLdr = Math.round(1500 + Math.sin(t / 4) * 520);
  s.temperatureC = Number((24.4 + Math.sin(t / 23) * 0.8).toFixed(1));
  s.humidityRh = Number((46 + Math.sin(t / 18) * 5).toFixed(1));
  s.vbus = Number((5.02 + Math.sin(t / 12) * 0.04).toFixed(2));
  s.smoothSpectrum = Array.from({ length: 32 }, (_, i) => {
    const v = Math.sin(t * 2.5 + i / 2.8) * 2.4 + Math.sin(t + i / 5) * 1.4 + 4;
    return clamp(v, 0, 8);
  });
  if (s.game.runState === 1 && Math.random() > 0.78) {
    s.game.score += 1;
    s.game.highScore = Math.max(s.game.highScore, s.game.score);
    s.game.snakeLen = clamp(s.game.snakeLen + 1, 3, 40);
  }
  updateTimerDerived();
}

function updateTimerDerived() {
  const s = app.state;
  if (s.timerState !== 1) {
    return;
  }
  const elapsed = Math.floor((Date.now() - s._timerStartedAt) / 1000);
  if (s.timerMode === 2) {
    s.stopwatchElapsedSec = elapsed;
    return;
  }
  s.timerRemainingSec = Math.max(0, s.timerDurationSec - elapsed);
  if (s.timerRemainingSec === 0) {
    s.timerState = 3;
  }
}

async function refreshScreen() {
  if (!app.matrixReady) return;
  if (!app.settings.mockMode && app.connected) {
    try {
      const data = await fetchJson("/api/screen");
      if (Array.isArray(data.pixels)) {
        paintPixels(data.pixels);
        return;
      }
    } catch (error) {
      app.connected = false;
      updateConnectionUi();
    }
  }
  paintPixels(generateMockPixels());
}

function generateMockPixels() {
  const pixels = Array.from({ length: 256 }, () => [5, 6, 5]);
  const xy = (x, y) => y * 32 + x;
  const s = app.state;
  const color = stateColor();
  const t = Date.now() / 1000;

  if (s.mode === 0) {
    drawBlockText(pixels, s.time.slice(0, 5).replace(":", ""), 3, 1, color);
    if (Math.floor(t) % 2 === 0) {
      pixels[xy(15, 2)] = [255, 255, 255];
      pixels[xy(15, 4)] = [255, 255, 255];
    }
    const sec = new Date().getSeconds();
    for (let x = 0; x < Math.round((sec / 59) * 32); x += 1) pixels[xy(x, 7)] = [13, 143, 121];
  } else if (s.mode === 1) {
    s.smoothSpectrum.forEach((value, x) => {
      const h = Math.round(clamp(value, 0, 8));
      for (let y = 0; y < h; y += 1) {
        pixels[xy(x, 7 - y)] = hsvToRgb((x * 9 + t * 40) % 360, 0.74, 0.82);
      }
    });
  } else if (s.mode === 2) {
    for (let i = 0; i < s.fluidParticles; i += 1) {
      const x = Math.floor((Math.sin(t * 0.9 + i * 1.7) * 0.5 + 0.5) * 31);
      const y = Math.floor((Math.cos(t * 1.1 + i * 0.8) * 0.5 + 0.5) * 7);
      pixels[xy(x, y)] = hsvToRgb((170 + i * 3 + t * 18) % 360, 0.65, 0.82);
    }
  } else if (s.mode === 3) {
    const offset = Math.floor((Date.now() / s.scrollSpeedMs) % 64);
    const textColor = s.scrollRainbow
      ? hsvToRgb((t * 65) % 360, 0.8, 1)
      : color;
    drawBlockText(pixels, s.scrollText.toUpperCase(), 32 - offset, 2, textColor);
  } else if (s.mode === 4) {
    drawBlockText(pixels, formatTimerText(), 4, 1, [232, 93, 79]);
    const lit = Math.round((1 - (s.timerRemainingSec / Math.max(1, s.timerDurationSec))) * 32);
    for (let x = 0; x < lit; x += 1) pixels[xy(x, 7)] = [226, 163, 35];
  } else if (s.mode === 5) {
    drawWeatherGlyph(pixels, s.weather.weatherCode);
    drawBlockText(pixels, String(Math.round(s.weather.temperature)), 12, 1, [255, 255, 255]);
    for (let x = 0; x < Math.round((s.weather.cloudCover / 100) * 32); x += 1) pixels[xy(x, 7)] = [36, 120, 191];
  } else if (s.mode === 6) {
    pixels[xy(s.game.foodX || 22, s.game.foodY || 4)] = [232, 93, 79];
    pixels[xy(s.game.ballX || 16, s.game.ballY || 4)] = [255, 255, 255];
    for (let i = 1; i < Math.min(12, s.game.snakeLen || 3); i += 1) {
      pixels[xy(clamp((s.game.ballX || 16) - i, 0, 31), s.game.ballY || 4)] = [34, 161, 96];
    }
  }
  return pixels;
}

function paintPixels(pixels) {
  const nodes = byId("matrix").children;
  const count = Math.min(nodes.length, pixels.length);
  const brightnessRatio = clamp(
    Number(app.state.effectiveBrightness || 1) / Math.max(1, Number(app.state.brightnessCap || 255)),
    0,
    1,
  );
  const previewScale = 0.16 + brightnessRatio * 0.84;
  for (let i = 0; i < count; i += 1) {
    const p = pixels[i] || [0, 0, 0];
    const r = Math.round((p[0] || 0) * previewScale);
    const g = Math.round((p[1] || 0) * previewScale);
    const b = Math.round((p[2] || 0) * previewScale);
    nodes[i].style.background = `rgb(${r}, ${g}, ${b})`;
    nodes[i].style.boxShadow = (r + g + b) > 80
      ? `0 0 7px rgba(${r}, ${g}, ${b}, .44)`
      : "inset 0 0 0 1px rgba(255, 255, 255, .04)";
  }
}

function drawBlockText(pixels, text, x0, y0, color) {
  const xy = (x, y) => y * 32 + x;
  const glyphs = {
    "0": [7, 5, 5, 5, 7],
    "1": [2, 6, 2, 2, 7],
    "2": [7, 1, 7, 4, 7],
    "3": [7, 1, 7, 1, 7],
    "4": [5, 5, 7, 1, 1],
    "5": [7, 4, 7, 1, 7],
    "6": [7, 4, 7, 5, 7],
    "7": [7, 1, 1, 1, 1],
    "8": [7, 5, 7, 5, 7],
    "9": [7, 5, 7, 1, 7],
    "A": [7, 5, 7, 5, 5],
    "B": [6, 5, 6, 5, 6],
    "C": [7, 4, 4, 4, 7],
    "D": [6, 5, 5, 5, 6],
    "E": [7, 4, 7, 4, 7],
    "F": [7, 4, 7, 4, 4],
    "G": [7, 4, 5, 5, 7],
    "H": [5, 5, 7, 5, 5],
    "I": [7, 2, 2, 2, 7],
    "K": [5, 5, 6, 5, 5],
    "L": [4, 4, 4, 4, 7],
    "M": [5, 7, 7, 5, 5],
    "N": [5, 7, 7, 7, 5],
    "O": [7, 5, 5, 5, 7],
    "P": [7, 5, 7, 4, 4],
    "R": [7, 5, 6, 5, 5],
    "S": [7, 4, 7, 1, 7],
    "T": [7, 2, 2, 2, 2],
    "U": [5, 5, 5, 5, 7],
    "W": [5, 5, 7, 7, 5],
    "X": [5, 5, 2, 5, 5],
    "Y": [5, 5, 7, 2, 2],
    " ": [0, 0, 0, 0, 0],
  };
  let cursor = x0;
  text.split("").forEach((ch) => {
    const rows = glyphs[ch] || glyphs[" "];
    rows.forEach((bits, row) => {
      for (let col = 0; col < 3; col += 1) {
        if (bits & (1 << (2 - col))) {
          const x = cursor + col;
          const y = y0 + row;
          if (x >= 0 && x < 32 && y >= 0 && y < 8) {
            pixels[xy(x, y)] = color;
          }
        }
      }
    });
    cursor += 4;
  });
}

function drawWeatherGlyph(pixels, code) {
  const xy = (x, y) => y * 32 + x;
  const set = (x, y, c) => {
    if (x >= 0 && x < 32 && y >= 0 && y < 8) pixels[xy(x, y)] = c;
  };
  if (code === 0) {
    [[3, 1], [2, 2], [3, 2], [4, 2], [1, 3], [2, 3], [3, 3], [4, 3], [5, 3], [2, 4], [3, 4], [4, 4], [3, 5]].forEach(([x, y]) => set(x, y, [226, 163, 35]));
    return;
  }
  [[2, 3], [3, 2], [4, 2], [5, 3], [1, 4], [2, 4], [3, 4], [4, 4], [5, 4], [6, 4]].forEach(([x, y]) => set(x, y, [120, 130, 140]));
  if (code >= 51) {
    [[2, 6], [4, 6], [6, 6], [3, 7], [5, 7]].forEach(([x, y]) => set(x, y, [36, 120, 191]));
  }
}

function displayControlSignature(state) {
  return JSON.stringify([
    state.mode,
    state.clockTheme,
    state.scrollText,
    state.scrollSpeedMs,
    state.scrollRainbow,
    state.audioVisualMode,
    state.audioRainbow,
    state.fluidParticles,
    state.fluidFlipRatio,
    state.timerMode,
    state.timerState,
    state.weatherDisplayMode,
    state.gameType,
    state.manualBrightness,
    state.autoBrightness,
    state.color?.r,
    state.color?.g,
    state.color?.b,
  ]);
}

function renderOptimisticState() {
  updateTimerDerived();
  updateConnectionUi();
  renderStateText();
  renderControls();
  paintPixels(generateMockPixels());
}

function scheduleScreenRefresh(delay = 50) {
  clearTimeout(app.controlRefreshTimer);
  app.controlRefreshTimer = setTimeout(() => {
    app.controlRefreshTimer = null;
    refreshScreen();
  }, delay);
}

function renderAll() {
  if (app.settings.mockMode || !app.connected) {
    tickMockState();
  } else {
    updateTimerDerived();
  }
  updateConnectionUi();
  renderStateText();
  renderControls();
  refreshScreen();
}

function renderStateText() {
  const s = app.state;
  const visibleMode = s.smartScenes ? (s.effectiveMode ?? s.mode) : s.mode;
  byId("modeTitle").textContent = MODE_NAMES[visibleMode] || "Clock";
  byId("clockText").textContent = s.time || "--:--:--";
  byId("statBrightness").textContent = `${s.effectiveBrightness ?? s.manualBrightness ?? "--"} / ${s.brightnessCap ?? "--"}`;
  byId("statVbus").textContent = `${formatNumber(s.vbus, 2)} V`;
  byId("statWeather").textContent = `${formatNumber(s.weather?.temperature, 1)} C`;
  byId("statAudio").textContent = formatNumber(s.rms, 2);
  const ai = s.deskAi || {};
  const aiState = DESK_STATES[ai.state] || ai.label || "Unknown";
  const aiConfidence = Math.round(clamp(Number(ai.confidence || 0), 0, 1) * 100);
  byId("homeAiLabel").textContent = aiState;
  byId("homeAiConfidence").textContent = `${aiConfidence}%`;
  byId("homeAiReason").textContent = aiExplanation(aiState, ai.features || []);
  byId("aiLabel").textContent = aiState;
  byId("aiConfidence").textContent = `${aiConfidence}%`;
  byId("aiDecision").textContent = aiExplanation(aiState, ai.features || []);
  const aiFeatures = ai.features || [];
  renderAiFeature("Audio", aiFeatures[0]);
  renderAiFeature("Bass", aiFeatures[1]);
  renderAiFeature("Motion", aiFeatures[2]);
  renderAiFeature("Light", aiFeatures[3]);
  renderAiFeature("Engagement", aiFeatures[4]);
  byId("aiInference").textContent = `${ai.inferenceMicros ?? "--"} us / ${ai.inferenceCount ?? 0} local inferences`;
  byId("aiSamples").textContent = `Samples F:${ai.samples?.[0] ?? 0} M:${ai.samples?.[1] ?? 0} R:${ai.samples?.[2] ?? 0} A:${ai.samples?.[3] ?? 0}`;
  const profileCoverage = Number(ai.profileCoverage || 0);
  const profileReady = Boolean(ai.profileReady);
  const minSamples = Number(ai.minSamplesPerClass || 4);
  const recommendedSamples = Number(ai.recommendedSamplesPerClass || 8);
  byId("aiProfileStatus").textContent = profileReady ? "Ready for blind test" : "Calibration needed";
  byId("aiProfileCoverage").textContent = `${profileCoverage} / 4`;
  byId("aiProfileQuality").textContent = `${Math.round(Number(ai.profileQuality || 0))}%`;
  byId("aiProfileSeparation").textContent = Number(ai.centroidSeparation || 0).toFixed(2);
  byId("aiProfileHint").textContent = profileReady
    ? `Profile passes the coverage and separation gate. Keep calibration frozen, then collect independent blind-test labels for the comparison.`
    : `Collect at least ${minSamples} samples for each state. ${recommendedSamples} per state is recommended for a more stable personal profile.`;
  const evaluation = ai.evaluation || {};
  const evaluationTotal = Number(evaluation.total || 0);
  byId("aiPersonalizedAccuracy").textContent = evaluationTotal
    ? `${Math.round((Number(evaluation.personalizedCorrect || 0) / evaluationTotal) * 100)}%`
    : "--";
  byId("aiBaselineAccuracy").textContent = evaluationTotal
    ? `${Math.round((Number(evaluation.baselineCorrect || 0) / evaluationTotal) * 100)}%`
    : "--";
  byId("aiEvaluationTotal").textContent = String(evaluationTotal);
  byId("aiConfusionHint").textContent = evaluationTotal
    ? `Confusion matrix: rows are actual states; columns are device predictions. F:${evaluation.samples?.[0] ?? 0} M:${evaluation.samples?.[1] ?? 0} R:${evaluation.samples?.[2] ?? 0} A:${evaluation.samples?.[3] ?? 0}.`
    : "No validation samples yet. Collect each class several times for a fair comparison.";
  const recallLabels = ["Focus", "Meeting", "Rest", "Away"];
  const recalls = recallLabels.map((label, index) => {
    const total = Number(evaluation.samples?.[index] || 0);
    const correct = Number(evaluation.confusion?.[index]?.[index] || 0);
    return total ? `${label} ${Math.round((correct / total) * 100)}%` : `${label} --`;
  });
  const improvement = evaluationTotal
    ? Math.round(((Number(evaluation.personalizedCorrect || 0) - Number(evaluation.baselineCorrect || 0)) / evaluationTotal) * 100)
    : 0;
  byId("aiRecallHint").textContent = evaluationTotal
    ? `Blind-test recall: ${recalls.join(" / ")}. Personalized model ${improvement >= 0 ? "+" : ""}${improvement} percentage points versus the default baseline.`
    : "Per-state recall will appear after the first blind validation samples.";
  byId("aiOfflineProof").textContent = ai.lastInferenceOffline
    ? `Offline inference active: ${ai.offlineInferenceCount ?? 0}`
    : `Offline inference count: ${ai.offlineInferenceCount ?? 0}`;
  renderAiTimeline(ai.timeline || [], s.uptimeMs || 0);
  byId("timerDisplay").textContent = formatTimerText();
  byId("weatherTemp").textContent = `${formatNumber(s.weather?.temperature, 1)} C`;
  byId("weatherMeta").textContent = `${s.weather?.city || s.weatherCity} / ${s.weather?.humidity ?? "--"}% / wind ${formatNumber(s.weather?.windSpeed, 1)}`;
  byId("gameScore").textContent = s.game?.score ?? 0;
  byId("gameBest").textContent = s.game?.highScore ?? 0;
  byId("deviceIp").textContent = s.network?.ip || s.ip || "0.0.0.0";
  byId("deviceSsid").textContent = s.network?.ssid || s.wifiSsid || "--";
  byId("deviceRssi").textContent = s.network?.rssi ? `${s.network.rssi} dBm` : "--";
  byId("devicePower").textContent = s.highPower ? "12V high power" : "5V / limited";
  byId("deviceDcdc").textContent = s.dcdcEnabled ? "enabled" : "off";
  byId("deviceHeap").textContent = s.freeHeap ? `${Math.round(s.freeHeap / 1024)} KB` : "--";
  byId("deviceUptime").textContent = formatDuration(s.uptimeMs || 0);
  byId("lowMeter").style.transform = `scaleX(${clamp(s.audioLowEnergy || 0, 0.03, 1)})`;
  byId("midMeter").style.transform = `scaleX(${clamp(s.audioMidEnergy || 0, 0.03, 1)})`;
  byId("highMeter").style.transform = `scaleX(${clamp(s.audioHighEnergy || 0, 0.03, 1)})`;
  const sceneFlags = [];
  if (s.quietHours) sceneFlags.push("安静");
  if (s.darkEnvironment) sceneFlags.push("低照度");
  if (s.audioActive) sceneFlags.push("音频");
  if (s.motionActive) sceneFlags.push("姿态");
  byId("sceneStatus").textContent = `${SCENE_REASONS[s.sceneReason] || "手动场景"}${sceneFlags.length ? ` / ${sceneFlags.join(" · ")}` : ""}`;
  qsa("[data-mode]").forEach((button) => button.classList.toggle("active", Number(button.dataset.mode) === s.mode));
  qsa("[data-clock-theme]").forEach((button) => button.classList.toggle("active", Number(button.dataset.clockTheme) === s.clockTheme));
}

function renderControls() {
  const s = app.state;
  setValue("modeSelect", s.mode);
  setValue("brightness", s.manualBrightness);
  byId("brightnessValue").textContent = String(s.manualBrightness);
  byId("brightnessHint").textContent =
    `实际 ${s.effectiveBrightness ?? "--"} / 供电上限 ${s.brightnessCap ?? "--"}，拖动后自动切换手动`;
  setChecked("autoBrightness", s.autoBrightness);
  setChecked("smartScenes", s.smartScenes);
  setChecked("deskAiEnabled", s.deskAiEnabled);
  setChecked("deskAiAutoScene", s.deskAiAutoScene);
  setChecked("gammaCorrection", s.gammaCorrection);
  setChecked("smoothTransitions", s.smoothTransitions);
  setValue("transitionStyle", s.transitionStyle);
  setValue("transitionDurationMs", s.transitionDurationMs);
  byId("transitionDurationValue").textContent = `${s.transitionDurationMs} ms`;
  setValue("lowLightThreshold", s.lowLightThreshold);
  setValue("highLightThreshold", s.highLightThreshold);
  setValue("quietStartHour", s.quietStartHour);
  setValue("quietEndHour", s.quietEndHour);
  setValue("nightBrightnessCap", s.nightBrightnessCap);
  byId("nightBrightnessCapValue").textContent = String(s.nightBrightnessCap);
  setValue("colorPicker", rgbToHex(s.color));
  setValue("clockTheme", s.clockTheme);
  setValue("audioVisualMode", s.audioVisualMode);
  setValue("audioSensitivity", s.audioSensitivity);
  byId("audioSensitivityValue").textContent = String(s.audioSensitivity);
  setValue("audioSmoothing", s.audioSmoothing);
  byId("audioSmoothingValue").textContent = String(s.audioSmoothing);
  setChecked("audioRainbow", s.audioRainbow);
  setChecked("audioBeatFlash", s.audioBeatFlash);
  setChecked("audioAutoGain", s.audioAutoGain);
  setValue("fluidParticles", s.fluidParticles);
  byId("fluidParticlesValue").textContent = String(s.fluidParticles);
  setValue("fluidFlipRatio", Math.round((s.fluidFlipRatio || 0) * 100));
  byId("fluidFlipRatioValue").textContent = `${Math.round((s.fluidFlipRatio || 0) * 100)}%`;
  setChecked("fluidSeparateParticles", s.fluidSeparateParticles);
  setChecked("fluidCompensateDrift", s.fluidCompensateDrift);
  setValue("scrollText", s.scrollText);
  setValue("scrollSpeedMs", s.scrollSpeedMs);
  byId("scrollSpeedValue").textContent = `${s.scrollSpeedMs} ms`;
  setChecked("scrollRainbow", s.scrollRainbow);
  setValue("timerMode", s.timerMode);
  setValue("timerMinutes", Math.round((s.timerDurationSec || 300) / 60));
  setValue("pomodoroFocusMin", s.pomodoroFocusMin);
  setValue("pomodoroBreakMin", s.pomodoroBreakMin);
  setChecked("weatherEnabled", s.weatherEnabled);
  setValue("weatherDisplayMode", s.weatherDisplayMode);
  setValue("weatherCity", s.weatherCity);
  setValue("weatherLatitude", s.weatherLatitude);
  setValue("weatherLongitude", s.weatherLongitude);
  setValue("weatherUpdateIntervalMin", s.weatherUpdateIntervalMin);
  setValue("gameType", s.gameType);
  setValue("gameSpeedMs", s.gameSpeedMs);
  byId("gameSpeedValue").textContent = `${s.gameSpeedMs} ms`;
  setChecked("gameUseMpuControl", s.gameUseMpuControl);
}

function setValue(id, value) {
  const el = byId(id);
  if (!el || document.activeElement === el) return;
  el.value = value ?? "";
}

function setChecked(id, checked) {
  const el = byId(id);
  if (!el) return;
  el.checked = Boolean(checked);
}

function updateConnectionUi() {
  const dot = byId("connDot");
  const text = byId("connText");
  dot.classList.remove("connected", "error");
  if (app.settings.mockMode) {
    text.textContent = "Mock 模式";
  } else if (app.connected) {
    dot.classList.add("connected");
    text.textContent = app.realtimeConnected ? "设备在线 / 实时" : "设备在线 / 轮询";
  } else {
    dot.classList.add("error");
    text.textContent = "未连接";
  }
}

function formatTimerText() {
  const s = app.state;
  const sec = s.timerMode === 2 ? (s.stopwatchElapsedSec || 0) : (s.timerRemainingSec || 0);
  const minutes = Math.floor(sec / 60);
  const seconds = Math.floor(sec % 60);
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function formatNumber(value, digits = 1) {
  if (value === undefined || value === null || Number.isNaN(Number(value))) return "--";
  return Number(value).toFixed(digits);
}

function renderAiFeature(name, value) {
  const normalized = clamp(Number(value || 0), 0, 1);
  const bar = byId(`aiFeature${name}`);
  const label = byId(`aiFeature${name}Value`);
  if (bar) bar.style.transform = `scaleX(${Math.max(0.03, normalized)})`;
  if (label) label.textContent = `${Math.round(normalized * 100)}%`;
}

function aiExplanation(state, features) {
  const audio = Math.round(clamp(Number(features[0] || 0), 0, 1) * 100);
  const motion = Math.round(clamp(Number(features[2] || 0), 0, 1) * 100);
  const light = Math.round(clamp(Number(features[3] || 0), 0, 1) * 100);
  const explanations = {
    Focus: `Low audio ${audio}% and motion ${motion}% match your calibrated focus profile.`,
    Meeting: `Audio ${audio}% with active engagement matches your meeting profile.`,
    Rest: `Low activity with ambient light ${light}% matches your rest profile.`,
    Away: "Sustained low audio and motion indicate the desk is unattended.",
  };
  return explanations[state] || "Collecting local microphone, motion, and light features for the first decision.";
}

function renderAiTimeline(timeline, uptimeMs) {
  const root = byId("aiTimeline");
  if (!root) return;
  root.innerHTML = "";
  if (!timeline.length) {
    root.textContent = "Waiting for local inference history.";
    return;
  }
  const recent = timeline.slice(-30);
  recent.forEach((entry) => {
    const [timestampMs, state, confidence, offline] = entry;
    const cell = document.createElement("span");
    cell.className = `timeline-cell state-${state}${offline ? " offline" : ""}`;
    const ageSec = Math.max(0, Math.round((uptimeMs - Number(timestampMs || 0)) / 1000));
    cell.title = `${DESK_STATES[state] || "Unknown"} / ${confidence}% / ${offline ? "offline" : "connected"} / ${ageSec}s ago`;
    cell.textContent = String(state || 0);
    root.appendChild(cell);
  });
}

function formatDuration(ms) {
  const total = Math.floor(ms / 1000);
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = total % 60;
  if (h > 0) return `${h}h ${m}m`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

function rgbToHex(color) {
  const c = color || { r: 255, g: 255, b: 255 };
  return `#${[c.r, c.g, c.b].map((v) => clamp(Number(v), 0, 255).toString(16).padStart(2, "0")).join("")}`;
}

function hexToRgb(hex) {
  const n = parseInt(hex.replace("#", ""), 16);
  return {
    r: (n >> 16) & 255,
    g: (n >> 8) & 255,
    b: n & 255,
  };
}

function stateColor() {
  const c = app.state.color || { r: 58, g: 215, b: 255 };
  return [c.r, c.g, c.b];
}

function hsvToRgb(h, s, v) {
  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;
  let rp = 0;
  let gp = 0;
  let bp = 0;
  if (h < 60) [rp, gp, bp] = [c, x, 0];
  else if (h < 120) [rp, gp, bp] = [x, c, 0];
  else if (h < 180) [rp, gp, bp] = [0, c, x];
  else if (h < 240) [rp, gp, bp] = [0, x, c];
  else if (h < 300) [rp, gp, bp] = [x, 0, c];
  else [rp, gp, bp] = [c, 0, x];
  return [Math.round((rp + m) * 255), Math.round((gp + m) * 255), Math.round((bp + m) * 255)];
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, Number(value)));
}

function confirmAction(message, action) {
  if (window.confirm(message)) action();
}

function appendLog(text) {
  const log = byId("deviceLog");
  log.textContent = `${new Date().toLocaleTimeString()}\n${text}\n\n${log.textContent}`.slice(0, 3000);
}

function toast(message) {
  const el = byId("toast");
  el.textContent = message;
  el.classList.add("show");
  clearTimeout(app.toastTimer);
  app.toastTimer = setTimeout(() => el.classList.remove("show"), 1800);
}

function shortError(error) {
  const message = error?.message || String(error);
  if (message.includes("Failed to fetch")) {
    return "网络或 CORS";
  }
  return message.slice(0, 60);
}

window.addEventListener("DOMContentLoaded", init);
