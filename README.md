# PixelFlow Desk AI

**Firmware v2.7.3 | Android App v1.8.2**

PixelFlow Desk AI is an ESP32-S3 edge-AI desktop terminal built around a 32 x 8 WS2812B matrix. It combines audio, motion, ambient light, temperature, humidity, weather, local inference, adaptive scenes, games, and an Android control app.

The device performs inference locally. Its explainable five-feature prototype classifier compares a personalized floating-point model, a built-in baseline, and an INT8 quantized model. The Android app handles configuration, labeling, blind-test evaluation, visualization, and evidence export; it does not run the model.

## Highlights

- Clock, spectrum, fluid, text, timer, weather, and game displays.
- MAX9814 FFT analysis, beat detection, and automatic gain control.
- MPU6500 gravity interaction, HTU21D environment sensing, and LDR auto brightness.
- Rule-based and AI-driven adaptive scenes with explainable decisions.
- Personalized calibration, unknown rejection, temporal smoothing, model locking, and blind testing.
- Open-Meteo weather, China-friendly NTP servers, and manual refresh controls.
- Non-blocking Wi-Fi provisioning, NVS settings, Web console, LiveView, and HTTP/WebSocket APIs.
- Capacitor Android app with Chinese-first UI and local-network device control.

## Repository Layout

```text
include/       Firmware headers, configuration, and shared state
src/           ESP32-S3 tasks, inference, rendering, networking, and APIs
data/          Device Web assets and filesystem resources
mobile_app/    Android app source, Capacitor project, Gradle wrapper, and build scripts
docs/          Competition guide and Desk AI technical documentation
platformio.ini Reproducible PlatformIO environment and library dependencies
```

Generated caches and local toolchains are intentionally excluded: `.pio/`, `node_modules/`, Android SDK/JDK copies, Gradle caches, and build outputs. Restore them from the tracked manifests and scripts.

## Firmware Build

1. Install Visual Studio Code and the PlatformIO IDE extension.
2. Open this repository root.
3. Connect the ESP32-S3 over USB.
4. Run PlatformIO `Build` or `Upload` for the `esp32-s3-devkitc-1` environment.

The current firmware has been verified to compile with the dependencies pinned in `platformio.ini`.

## Android Build

The app build instructions are in [`mobile_app/README.md`](mobile_app/README.md). The repository contains source code, `package-lock.json`, the Capacitor configuration, Android project, Gradle wrapper, and PowerShell build scripts.

```powershell
cd mobile_app
npm ci
powershell -ExecutionPolicy Bypass -File .\build-apk.ps1
```

Release APK and firmware BIN files should be attached to a GitHub Release rather than committed to the source repository.

## First Connection

On an unconfigured device, connect a phone to:

```text
SSID: PixelClock-Setup
Password: pixelclock
Portal: http://192.168.4.1
```

Select a 2.4 GHz Wi-Fi network. After provisioning, keep the phone and device on the same LAN and connect through the assigned IP address or:

```text
http://pixel-fluid-clock.local
```

Saved Wi-Fi credentials live in the device NVS and are not included in this repository.

## Hardware

| Module | GPIO |
| --- | --- |
| WS2812B data | 15 |
| DC-DC enable | 16 |
| VBUS ADC | 8 |
| MAX9814 microphone | 9 |
| LDR | 10 |
| I2C SDA / SCL | 14 / 21 |
| MPU6500 interrupt | 11 |
| BOOT / user key | 0 |

The LED matrix contains 256 pixels arranged as 32 columns x 8 rows with column-major zigzag wiring.

## Documentation

- [`README.zh-CN.md`](README.zh-CN.md): Chinese project overview and setup.
- [`TEAMMATE_HANDOFF.zh-CN.md`](TEAMMATE_HANDOFF.zh-CN.md): teammate handoff checklist.
- [`docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md`](docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md): full Desk AI design and three-model explanation.
- [`docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md`](docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md): competition demonstration workflow.
- [`DEVELOPMENT.zh-CN.md`](DEVELOPMENT.zh-CN.md): firmware architecture and development notes.
