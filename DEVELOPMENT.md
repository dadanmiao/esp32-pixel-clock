# Desktop Pixel Clock and Fluid Rhythm Terminal

Author: Yang

## Overview

This project is an ESP32-S3 based desktop pixel clock with environmental sensing, audio response, and a FLIP-style fluid animation. The firmware uses PlatformIO with the Arduino framework and FreeRTOS tasks pinned across the ESP32-S3 cores.

Main features:

- 32 x 8 WS2812B pixel matrix, 256 LEDs total.
- NTP synchronized clock display.
- Web control console with live mode switching and tuning.
- Audio spectrum display from a MAX9814 microphone.
- Gravity-driven fluid animation using a lightweight FLIP/PIC particle-grid solver.
- Dynamic VBUS detection for 5 V / 12 V power limits.
- Ambient light adaptive brightness.
- I2C environmental and motion sensing with HTU21D and MPU6500.

## Hardware Connections

| Function | Device | ESP32-S3 GPIO | Notes |
| --- | --- | --- | --- |
| LED matrix data | WS2812B | GPIO15 | 256 LEDs, GRB order |
| DC-DC enable | High power converter EN | GPIO16 | High after firmware startup |
| VBUS sense | Divider ADC input | GPIO8 / ADC1_CH7 | 51k upper, 10k lower, ratio 6.1 |
| Microphone | MAX9814 OUT | GPIO9 / ADC1_CH8 | Default firmware samples by `analogRead` |
| Ambient light | LDR divider | GPIO10 / ADC1_CH9 | Low-pass filtered in software |
| I2C SDA | MPU6500, HTU21D | GPIO14 | Shared I2C bus |
| I2C SCL | MPU6500, HTU21D | GPIO21 | Shared I2C bus |
| IMU interrupt | MPU6500 INT | GPIO11 | Configured as input pull-up |
| User key | BOOT button | GPIO0 | Short press cycles display modes |

## LED Matrix Layout

- Physical LED count: 256.
- Logical display size: 32 columns x 8 rows.
- First LED: top-left corner.
- Wiring order: column-major zigzag.
- Even columns run top to bottom.
- Odd columns run bottom to top.

The coordinate mapper is implemented in `src/display_task.cpp` as `xy(x, y)`.

## Power Design

VBUS is measured through a 51k / 10k divider:

```text
VBUS -- 51k -- ADC(GPIO8) -- 10k -- GND
```

The divider ratio is:

```text
(51k + 10k) / 10k = 6.1
```

Firmware behavior:

- GPIO16 is held low during early boot.
- After Wi-Fi/Web, sensor, power, audio, and display tasks start, `markSystemFullyStarted()` enables the DC-DC rail.
- The DC-DC rail is enabled in both 5 V and 12 V modes.
- VBUS only changes LED power and brightness limits.

Default limits:

| Detected VBUS | FastLED current limit | Brightness cap |
| --- | ---: | ---: |
| 12 V mode | 15000 mA | 255 |
| 5 V mode | 450 mA | 48 |
| Unknown / undervoltage | 250 mA | 24 |

Tune these in `include/app_config.h`.

## Software Architecture

### Platform

- PlatformIO
- Arduino framework
- FreeRTOS
- Main board profile: `esp32-s3-devkitc-1`
- Async WebServer via `ESPAsyncWebServer`
- LED rendering via `FastLED`
- FFT via `arduinoFFT`

### Core 0 Tasks

| Task | Priority | Responsibility |
| --- | ---: | --- |
| `power_core0` | 4 | VBUS sampling, GPIO16 EN policy, power limits |
| `i2c_env_core0` | 3 | MPU6500, HTU21D, LDR filtering, time snapshot |
| `audio_fft_core0` | 1 | MAX9814 sampling, FFT, audio diagnostics |
| AsyncTCP/WebServer | Core 0 | HTTP console and JSON API |

Audio uses ordinary `analogRead(GPIO9)` by default because ADC DMA on Arduino-ESP32 2.x / ESP-IDF 4.4 was unstable on this board package. The DMA code remains present and can be re-enabled with `AudioUseAdcDma`.

### Core 1 Task

| Task | Priority | Responsibility |
| --- | ---: | --- |
| `fastled_render_core1` | 4 | FastLED refresh, clock, spectrum, and fluid animation |

## Shared State Model

Cross-core data is carried in `RenderState`:

- `ControlState`: mode, color, brightness, fluid tuning.
- `AudioState`: RMS, peak, spectrum, MIC diagnostics.
- `EnvironmentState`: temperature, humidity, acceleration, gyro, LDR, MPU status.
- `PowerState`: VBUS, current cap, brightness cap, DC-DC state.

Synchronization uses:

- Mutex for the authoritative state snapshot.
- Queue for render snapshots sent to Core 1.

Files:

- `include/app_state.h`
- `src/app_state.cpp`

## Web Console

The Web console is served by `src/web_server.cpp`.

If Wi-Fi credentials are configured, the device joins the network and starts HTTP on port 80. If no credentials are present or connection fails, it starts an AP:

```text
PixelClock-Setup
```

Wi-Fi credentials are set in `platformio.ini` build flags:

```ini
-D WIFI_SSID=\"YourSSID\"
-D WIFI_PASSWORD=\"YourPassword\"
```

Console functions:

- Switch display mode: Clock, Spectrum, Fluid.
- Set preferred color.
- Manual brightness and auto brightness.
- LDR low/high thresholds.
- Fluid tuning:
  - Particle count.
  - PIC / FLIP blend ratio.
  - Separate Particles.
  - Compensate Drift.
- Live diagnostics:
  - VBUS.
  - Power current cap.
  - Brightness cap.
  - Audio RMS / Peak.
  - MIC ADC raw min/max.
  - ADC sample counts.
  - Acceleration.
  - MPU online/fail counters.

## Display Modes

### Clock

Shows HH:MM on the 32 x 8 matrix using a compact 3 x 5 font. A seconds indicator moves along the bottom row. Temperature is shown as a small right-side bar when HTU21D is available.

### Spectrum

Uses 32 frequency bands, matching the matrix width. Audio is sampled from MAX9814 on GPIO9, converted into FFT bands, and rendered as vertical bars.

### Fluid

The fluid mode is inspired by `FLIP.15.html` from Ten Minute Physics.

Implemented concepts:

- Particle integration.
- Particle separation.
- Particle-to-grid velocity transfer.
- Density field.
- Pressure relaxation for approximate incompressibility.
- FLIP/PIC velocity blending back to particles.
- Boundary collision.
- Soft pixel splatting for visible liquid volume.

Runtime parameters:

| Parameter | Default | Effect |
| --- | ---: | --- |
| Particles | 48 | Controls visible liquid volume |
| PIC / FLIP | 78% FLIP | Lower is calmer, higher is more energetic |
| Separate Particles | On | Prevents clumping, increases spread |
| Compensate Drift | On | Reduces density compression drift |

If the fluid spreads across the whole display during shaking, reduce particle count or lower the FLIP ratio.

Recommended starting point:

- Particles: 36 to 48.
- PIC / FLIP: 55% to 70%.
- Separate Particles: On.
- Compensate Drift: On.

## Sensor Notes

### MAX9814

The firmware expects MAX9814 OUT on GPIO9.

Useful Web diagnostics:

- `Audio`: RMS / Peak.
- `MIC ADC`: current raw value, min/max range, DC bias.
- `Audio Src`: `analogRead` by default.

If MAX9814 output is valid but Web audio remains zero, verify the OUT trace reaches GPIO9 and shares ground with ESP32-S3.

### MPU6500

I2C address is assumed to be `0x68`.

The Web console shows:

```text
MPU online readCount/failCount
```

If it remains offline:

- Check SDA GPIO14 and SCL GPIO21.
- Check pull-ups.
- Check VCC and GND.
- Confirm AD0/address wiring.

### HTU21D

I2C address is `0x40`. Temperature and humidity are polled at a slower interval than the IMU.

## Main Files

| File | Purpose |
| --- | --- |
| `platformio.ini` | PlatformIO environment and library dependencies |
| `include/pinmap.h` | Hardware pin assignments |
| `include/app_config.h` | Compile-time configuration |
| `include/app_state.h` | Shared state structures |
| `src/main.cpp` | Boot sequence and task creation |
| `src/power_manager.cpp` | VBUS detection and DC-DC policy |
| `src/audio_task.cpp` | MAX9814 sampling and FFT |
| `src/sensor_task.cpp` | I2C sensors and LDR processing |
| `src/display_task.cpp` | FastLED rendering and animation modes |
| `src/web_server.cpp` | Web dashboard and JSON API |
| `src/adc_sample_bus.cpp` | Shared latest ADC sample cache |

## Build And Upload

Build:

```bash
platformio run
```

Upload:

```bash
platformio run --target upload
```

Serial monitor:

```bash
platformio device monitor
```

The generated firmware is:

```text
.pio/build/esp32-s3-devkitc-1/firmware.bin
```

## Tuning Checklist

1. Confirm GPIO16 goes high after boot.
2. Confirm VBUS reads near the expected 5 V or 12 V.
3. Confirm LED matrix coordinate mapping with clock mode.
4. Confirm `MIC ADC` min/max changes with sound.
5. Confirm `MPU` shows online and Accel changes when tilted.
6. Tune fluid:
   - Lower particle count if liquid covers too much area.
   - Lower FLIP ratio if it behaves too violently.
   - Disable Separate Particles only if the liquid looks too expanded.
   - Disable Compensate Drift only if pressure correction feels too stiff.

