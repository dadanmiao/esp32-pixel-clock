# PixelFlow Desk AI 队友交付说明

当前协作基线为：

```text
固件: v2.7.3
Android App: v1.8.2 (versionCode 14)
主分支: main
```

## GitHub 中包含的完整开发内容

- `include/`、`src/`、`data/`：ESP32-S3 固件源码和设备端页面。
- `platformio.ini`：PlatformIO 板卡环境、编译选项和固件依赖。
- `mobile_app/`：App 前端、Capacitor 配置、Android 工程、Gradle Wrapper 和构建脚本。
- `package-lock.json`：可复现的前端依赖版本。
- `docs/`：AI 技术设计、比赛演示和验证说明。
- 根目录 README、开发文档和版本迭代说明。

以下内容不应提交到 Git：`.pio/`、`node_modules/`、Android SDK、JDK、Gradle 缓存、构建目录、Codex 会话和个人 Wi-Fi 凭据。它们体积大、与本机相关，或包含隐私，不属于源码环境。

## 队友首次接手

```powershell
git clone https://github.com/dadanmiao/esp32-pixel-clock.git
cd esp32-pixel-clock
git checkout main
```

固件开发：安装 Visual Studio Code 与 PlatformIO IDE，打开仓库根目录后执行 `Build`。USB 连接同型号 ESP32-S3 后执行 `Upload`。

App 开发：安装 Node.js、JDK 21 和 Android SDK，然后运行：

```powershell
cd mobile_app
npm ci
powershell -ExecutionPolicy Bypass -File .\build-apk.ps1
```

构建脚本会生成测试 APK。无需把 `node_modules`、本机 SDK 或构建缓存从你的电脑传给队友。

## 首次连接设备

未配置网络的开发板会开启热点：

```text
名称: PixelClock-Setup
密码: pixelclock
地址: http://192.168.4.1
```

在配网页面选择 2.4 GHz Wi-Fi。手机与设备进入同一局域网后，在 App 中使用设备 IP 或 `http://pixel-fluid-clock.local` 连接。

每块开发板独立保存自己的 Wi-Fi 与显示配置。源码和构建产物不包含你当前开发板 NVS 中的家庭 Wi-Fi 密码。

## 主要模块

- `src/desk_ai.cpp`：桌面状态特征、个性化/基线/INT8 模型、拒识和评估。
- `src/scene_engine.cpp`：规则智能场景和 AI 自动场景。
- `src/audio_task.cpp`：麦克风采样、FFT 与音频特征。
- `src/sensor_task.cpp`：姿态、光照、温湿度和时间状态。
- `src/display_task.cpp`：时钟、频谱、流体、文字、天气、计时和游戏。
- `src/web_server.cpp`：状态接口、控制接口、WebSocket 和比赛证据接口。
- `mobile_app/app.js`：App 通信、控制、校准、盲测和结果呈现。

完整 AI 原理优先阅读 [`docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md`](docs/DESK_AI_TECHNICAL_DESIGN.zh-CN.md)，比赛演示顺序阅读 [`docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md`](docs/COMPETITION_V2_7_DEMO_GUIDE.zh-CN.md)。

## 稳定版本发布

Git 仓库保存源码和文档；每个稳定版本在 GitHub Releases 中附加：

```text
PixelFlow-Firmware-v2.7.3.bin
PixelClock-debug.apk
```

固件 BIN 是应用分区镜像，适用于 PlatformIO 生成流程或 OTA。若需从地址 `0x0` 直接烧录，应另外生成包含 bootloader、partition table 和应用的 full-flash 镜像，不能把应用分区 BIN 当作完整镜像使用。

## 协作约定

开发新功能时从最新 `main` 创建分支，提交前先编译对应端，随后通过 Pull Request 合并。修改固件或 App 发布行为时同步更新版本号和相关文档。不要覆盖队友尚未合并的改动，也不要提交真实网络密码、签名密钥或本机工具目录。
