# ESP32-S3 Desktop Pixel Clock and Fluid Rhythm Terminal

Author: Yang

This is a PlatformIO Arduino framework project for a 32x8 WS2812B desktop pixel clock, audio spectrum display, and gravity-driven fluid animation terminal.

## Hardware Pin Map

| Module | GPIO |
| --- | --- |
| WS2812B data | GPIO 15 |
| DC-DC EN | GPIO 16 |
| VBUS ADC1 | GPIO 8 |
| MAX9814 ADC1 | GPIO 9 |
| LDR ADC1 | GPIO 10 |
| I2C SDA | GPIO 14 |
| I2C SCL | GPIO 21 |
| MPU6500 INT | GPIO 11 |
| BOOT / user key | GPIO 0 |

## LED Matrix

- LED count: 256.
- Logical size: 32 columns x 8 rows.
- First LED: top-left corner.
- Wiring: column-major zigzag. Even columns run top-to-bottom; odd columns run bottom-to-top.

## Task Layout

Core 0:

- `audio_fft_core0`: ADC continuous DMA sampling and FFT.
- `i2c_env_core0`: HTU21D, MPU6500, LDR adaptation, and time snapshot.
- `power_core0`: VBUS detection and DC-DC power policy.
- AsyncTCP/WebServer is configured to run on Core 0 by `CONFIG_ASYNC_TCP_RUNNING_CORE=0`.

Core 1:

- `fastled_render_core1`: FastLED refresh and animation rendering.

## ADC Strategy

ESP32-S3 should not have multiple tasks fighting over ADC1. This framework samples MAX9814, VBUS, and LDR through one ADC continuous DMA driver in `audio_task.cpp`. Other tasks consume the latest cached VBUS/LDR readings through `adc_sample_bus`.

In the current firmware, MAX9814 audio defaults to `analogRead(GPIO9)` because ADC DMA on Arduino-ESP32 2.x / ESP-IDF 4.4 was unstable with this board package. The DMA path remains in `audio_task.cpp` and can be re-enabled with `AudioUseAdcDma`.

For the full hardware and software development notes, see `DEVELOPMENT.md`.

## Wi-Fi

Set Wi-Fi credentials with build flags in `platformio.ini`, for example:

```ini
build_flags =
  -D WIFI_SSID=\"YourSSID\"
  -D WIFI_PASSWORD=\"YourPassword\"
```

If credentials are empty or the connection fails, the firmware starts an AP named `PixelClock-Setup`.

## Calibration

Tune these values in `include/app_config.h`:

- `VbusDividerRatio`: set to `6.1` for the 51k upper and 10k lower divider.
- `Vbus12vThreshold`: voltage threshold for high-power mode.
- `lowLightThreshold` / `highLightThreshold`: adjustable from the Web console.

GPIO16 stays low during boot. After Wi-Fi/Web, ADC, I2C, power, and display tasks are started, the firmware waits `DcdcEnableDelayMs`, then enables the high-power DC-DC rail in both 5 V and 12 V modes. The detected VBUS voltage only changes the LED brightness and current limits.

Brightness caps:

- 12 V mode: `BrightnessCap12v = 255`.
- 5 V mode: `BrightnessCap5v = 48`.
- Unknown/undervoltage mode: `BrightnessCapUnknown = 24`.

In 12 V mode the display task applies:

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 15000);
```

In 5 V mode it applies:

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 450);
```
