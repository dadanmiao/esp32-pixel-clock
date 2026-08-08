# PixelFlow Desk AI 桌面智能终端

**固件 v2.7.3 | Android App v1.8.2**

PixelFlow Desk AI 是一套基于 ESP32-S3 与 32 x 8 WS2812B 点阵的端侧桌面状态感知与自适应交互终端。系统融合声音、姿态、环境光、温湿度、天气和用户标注，在设备本地完成状态识别，并根据桌面状态自动切换显示场景。

手机 App 负责设备连接、参数控制、个性化校准、盲测评估、可解释结果展示和比赛证据导出；AI 推理在 ESP32-S3 本地完成，不依赖手机或云端。

## 当前能力

- 时钟、频谱、流体、文字、计时、天气和游戏七类显示模式。
- MAX9814 FFT 频谱、鼓点检测、音频诊断和自动增益。
- MPU6500 重力交互、HTU21D 温湿度和 LDR 自动亮度。
- 规则智能场景与 AI 自动场景，支持优先级、确认时间和可解释原因。
- 个性化浮点模型、默认基线模型和 INT8 定点模型三路对照推理。
- 个性化校准、未知拒识、时序平滑、模型锁定和独立盲测。
- Open-Meteo 天气、国内 NTP 对时和主动刷新。
- 非阻塞 Wi-Fi 配网、NVS 参数保存、Web 控制台、LiveView 和 HTTP/WebSocket API。
- 中文优先的 Capacitor Android App。

## 项目结构

```text
include/       固件头文件、配置和共享状态
src/           任务、端侧推理、显示渲染、网络和 API
data/          设备 Web 资源
mobile_app/    App 前端、Capacitor、Android 工程和构建脚本
docs/          桌面 AI 技术设计和比赛演示文档
platformio.ini PlatformIO 环境与依赖清单
```

仓库不提交 `.pio/`、`node_modules/`、Android SDK、JDK、Gradle 缓存和构建目录。这些属于本机缓存或工具链，可通过依赖清单和构建脚本恢复，不影响队友复现完整开发环境。

## 固件构建与烧录

安装 Visual Studio Code 和 PlatformIO IDE 扩展，用 VS Code 打开仓库根目录，USB 连接 ESP32-S3 后执行 PlatformIO 的 `Build` 或 `Upload`。

当前固件已使用 `platformio.ini` 中声明的依赖完成编译验证。主要代码位置：

- `include/app_config.h`：版本和全局配置。
- `include/app_state.h`：设备共享状态。
- `src/desk_ai.cpp`：特征处理、三路模型、拒识和评估。
- `src/scene_engine.cpp`：规则场景与 AI 场景决策。
- `src/display_task.cpp`：点阵渲染与游戏显示。
- `src/web_server.cpp`：Web 页面和 HTTP/WebSocket API。
- `src/wifi_manager_app.cpp`：Wi-Fi 配网、状态和主动对时。
- `src/settings_storage.cpp`：NVS 设置与版本迁移。

## Android App 构建

详细说明见 [`mobile_app/README.md`](mobile_app/README.md)。仓库已经包含 `package-lock.json`、Capacitor 配置、Android 工程、Gradle Wrapper 和 PowerShell 构建脚本。

```powershell
cd mobile_app
npm ci
powershell -ExecutionPolicy Bypass -File .\build-apk.ps1
```

APK 和固件 BIN 属于发布产物，建议上传到 GitHub Releases，不直接提交到源码历史。

## 首次配网与连接

未配置网络的设备会开启：

```text
SSID: PixelClock-Setup
密码: pixelclock
配网页面: http://192.168.4.1
```

在页面中选择 2.4 GHz Wi-Fi。配网完成后，让手机和设备连接同一局域网，在 App 中输入设备 IP，或尝试：

```text
http://pixel-fluid-clock.local
```

家庭 Wi-Fi 凭据保存在开发板 NVS 中，不包含在源码、APK 或固件发布文件里。

## 硬件映射

| 模块 | GPIO |
| --- | --- |
| WS2812B 数据线 | 15 |
| DC-DC EN | 16 |
| VBUS ADC | 8 |
| MAX9814 麦克风 | 9 |
| LDR | 10 |
| I2C SDA / SCL | 14 / 21 |
| MPU6500 INT | 11 |
| BOOT / 用户按键 | 0 |

LED 矩阵共 256 颗灯，逻辑尺寸为 32 列 x 8 行，按列 zigzag 走线。

## 关键文档

- [`TEAMMATE_HANDOFF.zh-CN.md`](TEAMMATE_HANDOFF.zh-CN.md)：队友接手、构建和协作清单。
- [`docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md`](docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md)：桌面 AI 原理、数据、三种模型和部署说明。
- [`docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md`](docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md)：比赛演示流程与证据链。
- [`DEVELOPMENT.zh-CN.md`](DEVELOPMENT.zh-CN.md)：固件架构和开发说明。
- [`SOFTWARE_V2_CHANGELOG.zh-CN.md`](SOFTWARE_V2_CHANGELOG.zh-CN.md)：软件显示迭代记录。
