# Pixel Clock Mobile App

这是 ESP32-S3 桌面像素终端的移动端控制 App。项目同时保留浏览器版本，并已使用 Capacitor 打包为 Android APK。

## 安装 APK

已构建的测试安装包（当前版本 `1.3`）：

```text
dist/PixelClock-debug.apk
```

可把 APK 发送到安卓手机后点击安装，也可以开启手机 USB 调试、连接电脑后运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\install-apk.ps1
```

首次使用 USB 安装时，需要解锁手机并确认“允许 USB 调试”。

如需重新构建：

```powershell
powershell -ExecutionPolicy Bypass -File .\build-apk.ps1
```

当前 APK 使用 Android 调试证书签名，适合真机测试和内部安装。公开发布前应改用单独保管的正式签名密钥。

## 浏览器预览

直接打开 `index.html`。默认启用 Mock 模式，可以不连接硬件先体验界面和交互。

## 真机联调

1. 确认设备已经连入同一局域网。
2. 在首页或设置页填写设备地址，例如：

```text
http://pixel-fluid-clock.local
http://192.168.1.66
```

3. 关闭 `Mock` 开关，点击 `连接`。

也可以直接点击首页的 `查找`。App 会依次尝试当前地址、`pixel-fluid-clock.local` 和配网地址 `192.168.4.1`。连接新版固件后，状态栏显示 `设备在线 / 实时`；WebSocket 不可用时会自动降级为 `设备在线 / 轮询`。

APK 已启用 Capacitor 原生 HTTP 访问和局域网明文 HTTP，能够连接 `http://192.168.x.x` 形式的设备地址。浏览器版仍可能受 CORS 限制。

## 对接的固件 API

```text
GET  /api/state
WS   /ws
POST /api/control
POST /api/notify
GET  /api/screen
GET  /api/config
POST /api/config
POST /api/preset
POST /api/game/action
POST /api/game/direction
POST /api/save-settings
POST /api/reset-settings
POST /api/reboot
GET  /api/wifi/status
GET  /api/wifi/scan
POST /api/wifi/reset
```

## 页面结构

- 首页：连接、LiveView、状态摘要、快速模式切换
- 控制：显示、时钟、音乐、流体、文字、计时、天气、游戏
- 场景：桌面时钟、夜间模式、音乐律动、流体摆件、专注、天气、游戏
- 设备：网络、电源、运行状态、配置和重启操作
- 设置：设备地址、轮询间隔、Token 预留、接口映射

## 后续方向

- 智能场景可根据安静时段、环境亮度、音乐和姿态自动选择显示模式。
- 文字页的“临时通知”不会改变当前模式，适合提醒和计时结束消息。
- 转场、Gamma 色彩校正、夜间亮度上限和音频自动增益可在控制页调整。
- 后续可加入完整局域网网段扫描和设备绑定 Token。
- 真机联调后根据屏幕反馈微调 LiveView、滑条范围和场景预设。
- 使用正式签名密钥构建可长期升级的 Release APK。
