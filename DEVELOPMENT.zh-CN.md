# 桌面像素时钟与流体律动终端开发文档

作者：Yang

## 项目概述

本项目是一个基于 ESP32-S3 的桌面像素时钟终端，集成环境感知、音频响应和 FLIP 风格流体动画。固件使用 PlatformIO + Arduino 框架，并基于 FreeRTOS 将任务分配到 ESP32-S3 的双核心上运行。

主要功能：

- 32 x 8 WS2812B 像素矩阵，共 256 颗 LED。
- NTP 网络校时时钟。
- Web 控制台和 Android App，支持模式切换和参数实时调节。
- 基于 MAX9814 麦克风的音频频谱显示。
- 基于轻量 FLIP/PIC 粒子-网格求解器的重力流体动画。
- 滚动文字、番茄钟/倒计时/秒表、天气和像素小游戏。
- VBUS 动态检测，区分 5V / 12V 电源限制。
- 环境光自适应亮度。
- HTU21D 温湿度和 MPU6500 姿态传感。
- Wi-Fi 配网、NVS 参数保存、LiveView 和 JSON API。

## 硬件连接

| 功能 | 器件 | ESP32-S3 GPIO | 说明 |
| --- | --- | --- | --- |
| LED 数据线 | WS2812B | GPIO15 | 256 颗 LED，GRB 色序 |
| DC-DC 使能 | 大功率电源 EN | GPIO16 | 固件启动完成后拉高 |
| VBUS 检测 | 分压 ADC 输入 | GPIO8 / ADC1_CH7 | 51k 上电阻，10k 下电阻，分压比 6.1 |
| 麦克风 | MAX9814 OUT | GPIO9 / ADC1_CH8 | 默认使用 `analogRead` 采样 |
| 环境光 | LDR 分压 | GPIO10 / ADC1_CH9 | 软件低通滤波 |
| I2C SDA | MPU6500、HTU21D | GPIO14 | 共用 I2C 总线 |
| I2C SCL | MPU6500、HTU21D | GPIO21 | 共用 I2C 总线 |
| IMU 中断 | MPU6500 INT | GPIO11 | 输入上拉 |
| 用户按键 | BOOT | GPIO0 | 短按切换显示模式 |

## LED 矩阵布局

- 物理灯珠数量：256。
- 逻辑显示尺寸：32 列 x 8 行。
- 第一个灯珠位于左上角。
- 走线顺序：按列串联的 zigzag。
- 偶数列从上到下。
- 奇数列从下到上。

坐标映射函数位于 `src/display_task.cpp` 中的 `xy(x, y)`。

## 电源设计

VBUS 通过 51k / 10k 分压后进入 GPIO8：

```text
VBUS -- 51k -- ADC(GPIO8) -- 10k -- GND
```

分压比为：

```text
(51k + 10k) / 10k = 6.1
```

固件行为：

- 启动早期 GPIO16 保持低电平。
- Wi-Fi/Web、传感器、电源、音频和显示任务启动后，`markSystemFullyStarted()` 启用 DC-DC。
- 5V 和 12V 模式下都会启用 DC-DC。
- VBUS 只用于改变 LED 电流限制和亮度上限。

默认限制：

| VBUS 状态 | FastLED 电流限制 | 亮度上限 |
| --- | ---: | ---: |
| 12V 模式 | 15000 mA | 255 |
| 5V 模式 | 450 mA | 48 |
| 未知 / 欠压 | 250 mA | 24 |

这些参数可在 `include/app_config.h` 中调整。

## 软件架构

### 平台

- PlatformIO
- Arduino 框架
- FreeRTOS
- 主板配置：`esp32-s3-devkitc-1`
- Web 服务：`ESPAsyncWebServer`
- LED 渲染：`FastLED`
- FFT：`arduinoFFT`

### Core 0 任务

| 任务 | 优先级 | 职责 |
| --- | ---: | --- |
| `power_core0` | 4 | VBUS 采样、GPIO16 EN 策略、电源限制 |
| `i2c_env_core0` | 3 | MPU6500、HTU21D、LDR 滤波、时间快照 |
| `audio_fft_core0` | 1 | MAX9814 采样、FFT、音频诊断 |
| `weather_core0` | 1 | Open-Meteo 请求、天气解析和刷新状态 |
| AsyncTCP/WebServer | Core 0 | HTTP 控制台、JSON API 和 WebSocket |

主循环还以非阻塞方式维护计时结束检测、智能场景、通知队列、Wi-Fi 和延迟保存，不创建额外高频任务。

音频默认使用普通 `analogRead(GPIO9)`，因为 Arduino-ESP32 2.x / ESP-IDF 4.4 下当前板包的 ADC DMA 不稳定。DMA 代码仍保留，可通过 `AudioUseAdcDma` 重新启用。

### Core 1 任务

| 任务 | 优先级 | 职责 |
| --- | ---: | --- |
| `fastled_render_core1` | 4 | FastLED 刷新、时钟、频谱和流体动画 |

## 共享状态模型

跨核心数据通过 `RenderState` 传递：

- `ControlState`：模式、主题、颜色、亮度、流体、文字、计时、天气、音频和游戏参数。
- `AudioState`：RMS、Peak、频谱、低中高频能量、Beat 和麦克风诊断。
- `EnvironmentState`：温度、湿度、加速度、陀螺仪、LDR、MPU 状态。
- `PowerState`：VBUS、电流上限、亮度上限、DC-DC 状态。
- `WeatherState`：当前天气、今日预报、刷新时间和错误状态。
- `GameState`：游戏类型、运行状态、分数和游戏实体。
- `ContextState`：安静时段、音频活动、姿态活动、智能场景原因和当前有效模式。
- `NotificationState`：当前通知及固定深度的待显示队列。

同步方式：

- 使用 Mutex 保护权威状态快照。
- 使用 Queue 将渲染快照发送到 Core 1。
- 音频、环境、电源、天气、控制、游戏、场景和通知分别更新自己的状态分区，避免一个任务用旧快照覆盖其他任务刚写入的数据。

相关文件：

- `include/app_state.h`
- `src/app_state.cpp`

## Web 控制台

Web 控制台由 `src/web_server.cpp` 提供。

设备会在 80 端口启动 HTTP 服务，并提供配网 AP：

```text
PixelClock-Setup
```

默认配网信息：

```text
密码：pixelclock
地址：http://192.168.4.1
```

家庭 Wi-Fi 凭据通过配网页面写入 NVS。设备连接家庭网络后仍会维持 AP+STA，便于维护和重新配网。

控制台功能：

- 切换显示模式：Clock、Spectrum、Fluid、Text、Timer、Weather、Game。
- 切换时钟主题和音频可视化模式。
- 设置偏好颜色。
- 手动亮度和自动亮度。
- LDR 低/高光照阈值。
- 设置滚动文字、计时器、天气位置和游戏参数。
- 流体参数：
  - 粒子数量。
  - PIC / FLIP 混合比例。
  - Separate Particles。
  - Compensate Drift。
- 实时诊断：
  - VBUS。
  - 电流上限。
  - 亮度上限。
  - Audio RMS / Peak。
  - MIC ADC 原始最小/最大值。
  - ADC 采样计数。
  - 加速度。
  - MPU online/fail 计数。
- 点阵 LiveView、配置导入导出、保存/恢复设置、Wi-Fi 状态和重启操作。
- WebSocket `/ws` 每 500 ms 推送实时状态；客户端不可用时可继续轮询 `GET /api/state`。
- `POST /api/notify` 可显示临时滚动通知，而不改变当前显示模式。

## 软件 V2.1 显示链路

所有模式先绘制到目标帧，再经过转场、通知叠加、夜间颜色处理、Gamma 校正和渐进亮度，最后写入 FastLED 输出帧。

```text
模式渲染 -> 转场合成 -> 通知覆盖 -> 夜间/Gamma -> 亮度与电源限制 -> FastLED.show()
```

- 转场支持淡入淡出、横向擦除和像素溶解。
- 智能场景可根据计时、安静时段、环境光、声音和姿态选择有效模式，默认关闭。
- 音频分析包含自适应噪声底、自动增益、近似对数频带和独立的上升/下降平滑。
- 完整变更和默认参数见 `SOFTWARE_V2_CHANGELOG.zh-CN.md`。

## 显示模式

### Clock

在 32 x 8 矩阵上使用紧凑的 3 x 5 字体显示 HH:MM。底部一行显示秒进度。HTU21D 可用时，右侧会显示温度条。

### Spectrum

使用 32 个频段，对应矩阵 32 列宽度。MAX9814 的音频从 GPIO9 采样，经 FFT 转为频谱柱显示。

### Fluid

Fluid 模式受 `FLIP.15.html` 和 Ten Minute Physics 的 FLIP Fluid 示例启发。

当前实现的概念：

- 粒子积分。
- 粒子分离。
- 粒子速度转网格。
- 密度场。
- 压力松弛，近似不可压缩。
- FLIP/PIC 混合回写粒子速度。
- 边界碰撞。
- 柔和像素 splat 渲染液体体积。

运行时参数：

| 参数 | 默认值 | 作用 |
| --- | ---: | --- |
| Particles | 48 | 控制可见液体体积 |
| PIC / FLIP | 78% FLIP | 越低越稳定，越高越灵动 |
| Separate Particles | 开 | 防止粒子挤成团，但会增大扩散面积 |
| Compensate Drift | 开 | 减少密度压缩漂移 |

如果晃动时液体铺满全屏，可以降低粒子数量或降低 FLIP 比例。

推荐初始参数：

- Particles：36 到 48。
- PIC / FLIP：55% 到 70%。
- Separate Particles：开。
- Compensate Drift：开。

### Text

使用 5 x 7 字体显示滚动文字，支持速度、偏好颜色和彩虹模式。文字使用固定长度数组存入 `ControlState`，避免跨任务动态分配。

### Timer

支持番茄钟、普通倒计时和秒表。Timer 使用 `millis()` 和累计时间计算，不通过阻塞延时驱动。

### Weather

天气任务在 Core 0 请求 Open-Meteo，并将温度、湿度、天气代码、降水、云量、风速和当日预报写入 `WeatherState`。显示任务只消费状态快照，不在渲染线程执行网络请求。

### Game

支持 Snake、Gravity Ball、Reaction、Pong 和 Breakout。游戏使用非阻塞步进逻辑，可由 Web/App 方向键或 MPU6500 控制。

## 传感器说明

### MAX9814

固件期望 MAX9814 OUT 接到 GPIO9。

Web 诊断项：

- `Audio`：RMS / Peak。
- `MIC ADC`：当前原始值、最小/最大范围、直流偏置。
- `Audio Src`：默认显示 `analogRead`。

如果 MAX9814 输出端实测正常，但 Web 音频仍为零，应检查 OUT 是否真正连到 GPIO9，并确认与 ESP32-S3 共地。

### MPU6500

默认 I2C 地址为 `0x68`。

Web 控制台显示：

```text
MPU online readCount/failCount
```

如果一直 offline：

- 检查 SDA GPIO14 和 SCL GPIO21。
- 检查 I2C 上拉电阻。
- 检查 VCC 和 GND。
- 确认 AD0 / 地址配置。

### HTU21D

I2C 地址为 `0x40`。温湿度轮询频率低于 IMU。

## 主要文件

| 文件 | 用途 |
| --- | --- |
| `platformio.ini` | PlatformIO 环境和库依赖 |
| `include/pinmap.h` | 硬件引脚映射 |
| `include/app_config.h` | 编译期配置 |
| `include/app_state.h` | 共享状态结构 |
| `src/main.cpp` | 启动流程和任务创建 |
| `src/power_manager.cpp` | VBUS 检测和 DC-DC 策略 |
| `src/audio_task.cpp` | MAX9814 采样和 FFT |
| `src/sensor_task.cpp` | I2C 传感器和 LDR 处理 |
| `src/display_task.cpp` | FastLED 渲染和动画模式 |
| `src/weather_task.cpp` | Open-Meteo 请求和天气状态 |
| `src/timer_logic.cpp` | 番茄钟、倒计时和秒表 |
| `src/game_logic.cpp` | 像素小游戏逻辑 |
| `src/scene_engine.cpp` | 智能场景判定和有效模式选择 |
| `src/notification_manager.cpp` | 临时通知队列和生命周期 |
| `src/settings_storage.cpp` | NVS 参数保存和恢复 |
| `src/wifi_manager_app.cpp` | 非阻塞配网和 Wi-Fi 状态 |
| `src/web_server.cpp` | Web 控制台和 JSON API |
| `src/adc_sample_bus.cpp` | 最新 ADC 样本缓存 |
| `mobile_app/` | Android App 和浏览器原型 |
| `HARDWARE_V2_ROADMAP.zh-CN.md` | 竞赛硬件与产品化迭代方案 |
| `SOFTWARE_V2_CHANGELOG.zh-CN.md` | 软件与显示 V2.1 变更说明 |

## 构建与上传

构建：

```bash
platformio run
```

上传：

```bash
platformio run --target upload
```

串口监视：

```bash
platformio device monitor
```

生成的固件位于：

```text
.pio/build/esp32-s3-devkitc-1/firmware.bin
```

## 调试检查清单

1. 确认启动完成后 GPIO16 变为高电平。
2. 确认 VBUS 读数接近实际 5V 或 12V。
3. 在 Clock 模式确认 LED 矩阵坐标映射正确。
4. 确认 `MIC ADC` 的最小/最大值会随声音变化。
5. 确认 `MPU` 显示 online，倾斜时 Accel 变化。
6. 调节 Fluid：
   - 液体面积过大时降低粒子数量。
   - 运动过激时降低 FLIP 比例。
   - 液体扩散过强时可尝试关闭 Separate Particles。
   - 压力修正过硬时可尝试关闭 Compensate Drift。
