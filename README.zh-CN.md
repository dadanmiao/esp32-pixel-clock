# ESP32-S3 桌面像素时钟与流体律动终端

作者：Yang

这是一个基于 PlatformIO + Arduino 框架的 ESP32-S3 多模态桌面像素终端，用于驱动 32 x 8 WS2812B 像素矩阵，并通过声音、姿态、环境、网络和手机 App 完成交互。

当前功能：

- 七种显示模式：Clock、Spectrum、Fluid、Text、Timer、Weather、Game。
- 五种时钟主题和六种音频可视化模式。
- MAX9814 FFT 频谱、Beat 检测和音频诊断。
- MPU6500 重力交互、HTU21D 温湿度和 LDR 自动亮度。
- Open-Meteo 天气、NTP 校时、番茄钟和滚动文字。
- 非阻塞 Wi-Fi 配网、Web 控制台、NVS 参数保存和 LiveView。
- Android 手机 App，可通过局域网 HTTP/JSON API 和 WebSocket 控制设备。
- 软件 V2.1：平滑转场、Gamma 校正、智能场景、通知覆盖层和音频自动增益。

本次不改硬件的软件与显示升级详情见：

```text
SOFTWARE_V2_CHANGELOG.zh-CN.md
```

面向 2026 乐鑫嵌入式竞赛的硬件、交互、外观和产品化迭代方案见：

```text
HARDWARE_V2_ROADMAP.zh-CN.md
```

## 硬件引脚映射

| 模块 | GPIO |
| --- | --- |
| WS2812B 数据线 | GPIO 15 |
| 大功率 DC-DC EN | GPIO 16 |
| VBUS 电压检测 ADC1 | GPIO 8 |
| MAX9814 麦克风 ADC1 | GPIO 9 |
| LDR 光敏电阻 ADC1 | GPIO 10 |
| I2C SDA | GPIO 14 |
| I2C SCL | GPIO 21 |
| MPU6500 INT | GPIO 11 |
| BOOT / 用户按键 | GPIO 0 |

## LED 矩阵

- 灯珠数量：256。
- 逻辑尺寸：32 列 x 8 行。
- 第一个灯珠位于左上角。
- 走线方式：按列串联的 zigzag。
- 偶数列从上到下。
- 奇数列从下到上。

## 任务布局

Core 0：

- `audio_fft_core0`：MAX9814 采样、FFT 和音频诊断。
- `i2c_env_core0`：HTU21D、MPU6500、LDR 自适应亮度和时间快照。
- `power_core0`：VBUS 检测和 DC-DC 电源策略。
- `weather_core0`：Open-Meteo 天气请求和状态更新。
- AsyncTCP/WebServer 通过 `CONFIG_ASYNC_TCP_RUNNING_CORE=0` 配置在 Core 0。

Core 1：

- `fastled_render_core1`：FastLED 刷新和动画渲染。

## ADC 策略

ESP32-S3 上应避免多个任务同时抢占 ADC1。

当前固件中，MAX9814 音频默认使用 `analogRead(GPIO9)`，原因是 Arduino-ESP32 2.x / ESP-IDF 4.4 下，本项目板包的 ADC DMA 多通道采样不稳定。DMA 路径仍保留在 `audio_task.cpp` 中，可以通过 `AudioUseAdcDma` 重新启用。

完整硬件和软件开发说明见：

```text
DEVELOPMENT.zh-CN.md
```

## Wi-Fi 与手机 App

设备启动后会保留配网 AP：

```text
SSID: PixelClock-Setup
密码: pixelclock
地址: http://192.168.4.1
```

在配网页面选择家庭 2.4 GHz Wi-Fi 后，凭据会保存到 NVS。设备后台可通过局域网 IP 或以下主机名访问：

```text
http://pixel-fluid-clock.local
```

Android App 位于：

```text
mobile_app/
```

App 默认 Mock 模式。输入设备地址并点击“连接”后会自动退出 Mock 模式。

## 校准

以下参数位于 `include/app_config.h`：

- `VbusDividerRatio`：当前为 `6.1`，对应 51k 上拉电阻和 10k 下拉电阻。
- `Vbus12vThreshold`：12V 高功率模式判断阈值。
- `lowLightThreshold` / `highLightThreshold`：可在 Web 控制台调节的环境光阈值。

GPIO16 在启动早期保持低电平。Wi-Fi/Web、ADC、I2C、电源和显示任务启动后，固件等待 `DcdcEnableDelayMs`，随后在 5V 和 12V 模式下都会启用大功率 DC-DC。VBUS 电压只影响 LED 电流限制和亮度上限。

默认亮度上限：

- 12V 模式：`BrightnessCap12v = 255`
- 5V 模式：`BrightnessCap5v = 48`
- 未知 / 欠压模式：`BrightnessCapUnknown = 24`

12V 模式下：

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 15000);
```

5V 模式下：

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 450);
```

注意：256 颗 WS2812B 全白理论电流约为 15.36A。只有在电源、降压模块、PCB、连接器、线材和散热均通过验证后，才能使用 15A 级限制。普通桌面版本建议采用更保守的 4A 到 6A 上限，并增加电流与温升闭环保护。
