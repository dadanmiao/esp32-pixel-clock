# Pixel Clock 队友交付说明

## 包内内容

- `include/`、`src/`、`data/`：ESP32-S3 固件源码。
- `platformio.ini`：PlatformIO 工程配置和依赖。
- `mobile_app/`：Android App 的前端、Capacitor 和 Android 工程源码。
- `dist/PixelClock-fullflash-v2.2.0.bin`：包含引导程序、分区表和应用的完整镜像，可从 `0x0` 烧录。
- `dist/PixelClock-firmware-v2.2.0.bin`：应用分区镜像，供 PlatformIO、OTA 或高级调试使用。
- `dist/PixelClock-mobile-v1.3-debug.apk`：可直接安装的 Android App。
- `README.zh-CN.md`、`DEVELOPMENT.zh-CN.md`：项目和开发说明。

## 最快使用方法

1. 使用 PlatformIO 打开工程并点击 `Upload`；也可以把 `dist/PixelClock-fullflash-v2.2.0.bin` 从地址 `0x0` 烧录到同型号 ESP32-S3。
2. 在 Android 手机上安装 `dist/PixelClock-mobile-v1.3-debug.apk`。
3. 开发板首次启动后，手机连接热点 `PixelClock-Setup`。
4. 热点密码为 `pixelclock`，浏览器打开 `http://192.168.4.1`。
5. 为开发板选择自己的 2.4 GHz Wi-Fi 并输入密码。
6. 手机切回同一 Wi-Fi，在 App 中点击“查找”，或填写开发板获得的 IP 地址。

每块开发板会在自己的 NVS 中保存 Wi-Fi 和显示配置。源码、固件和 APK 不包含当前开发板已经保存的家庭 Wi-Fi 密码。

## 查看和修改固件代码

1. 安装 Visual Studio Code。
2. 安装 PlatformIO IDE 扩展。
3. 在 VS Code 中打开本交付包的工程根目录。
4. 用 USB 连接 ESP32-S3。
5. 在 PlatformIO 中选择 `Build` 编译，选择 `Upload` 烧录。

不要把应用分区文件 `PixelClock-firmware-v2.2.0.bin` 当作完整镜像从 `0x0` 烧录。

主要代码位置：

- `include/app_config.h`：版本和全局配置。
- `include/app_state.h`：设备共享状态。
- `src/display_task.cpp`：时钟、频谱、流体、文字、天气和游戏显示。
- `src/audio_task.cpp`：麦克风采样、FFT 和音频特征。
- `src/web_server.cpp`：设备网页和 HTTP/WebSocket API。
- `src/wifi_manager_app.cpp`：首次配网和 Wi-Fi 管理。
- `src/settings_storage.cpp`：NVS 设置保存。

## 构建 Android App

App 源码位于 `mobile_app/`。工程已包含 Capacitor Android 项目，但交付包不包含本机 Android SDK、JDK、Node.js 和 `node_modules`。

在准备好 Node.js、JDK 21 和 Android SDK 后，可以安装依赖并重新构建；也可以直接安装交付包中的 APK 进行测试。

## 推荐协作方式

日常协作建议把源码放入 Git 仓库：

1. 一人建立 GitHub、Gitee 或局域网 Git 私有仓库。
2. 提交源码和文档，不提交 `.pio/`、`node_modules/`、Android SDK、JDK、构建目录和个人 Wi-Fi 配置。
3. 邀请队友加入仓库，通过提交、拉取和分支同步代码。
4. 每次稳定版本把固件 BIN 和 APK 作为发布附件保存。

压缩包适合首次交付和离线备份，Git 仓库更适合后续共同开发。
