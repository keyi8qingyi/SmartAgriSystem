# SmartAgriSystem

SmartAgriSystem 是一个基于 ESP32 的智能农业系统，旨在通过 IoT 技术实现对植物生长环境的远程监控和管理。系统包括两大部分：ESP32 部分的代码用于采集传感器数据并上传至物联网平台，微信小程序部分则用于用户的远程控制和监控界面。

## 项目结构

- **ESP32 部分**：使用 ESP32 开发板，集成了传感器（如温湿度传感器、土壤湿度传感器、光照传感器等），并通过 Wi-Fi 连接物联网平台（如 OneNet）进行数据上传。
- **微信小程序部分**：提供一个简单易用的用户界面，用户可以通过该小程序远程查看植物环境数据，控制系统功能（如自动浇水、光照调节等）。

## 技术栈

- **硬件**：ESP32 开发板
- **编程语言**：C / Arduino，微信小程序使用 JavaScript
- **物联网平台**：OneNet
- **前端**：微信小程序
- **传感器**：
  - DHT11 温湿度传感器
  - GY30 光照传感器
  - 土壤湿度传感器

## 安装和运行

### ESP32 部分

1. 下载并安装 [Arduino IDE](https://www.arduino.cc/en/software)。
2. 在 Arduino IDE 中添加 ESP32 开发板支持，具体步骤可以参考 [ESP32 安装教程](https://github.com/espressif/arduino-esp32#installation-instructions)。
3. 将 ESP32 代码上传至你的 ESP32 开发板。

### 微信小程序部分

1. 下载并安装 [微信开发者工具](https://mp.weixin.qq.com/debug/wxadoc/dev/download/).
2. 克隆仓库后，在微信开发者工具中打开项目。
3. 根据需求配置物联网平台的 API 和设备信息。
4. 编译并上传到微信小程序平台进行发布。

## 使用方法

- **ESP32 部分**：ESP32 会自动读取传感器数据，并定时上传至物联网平台。
- **微信小程序**：用户可以通过小程序查看传感器数据，手动或自动控制系统（如浇水、光照等功能）。

## 贡献

欢迎提出任何功能需求或 bug 修复。请确保遵循以下步骤：

1. Fork 本项目
2. 创建功能分支 (`git checkout -b feature/your-feature-name`)
3. 提交更改 (`git commit -am 'Add new feature'`)
4. 推送到分支 (`git push origin feature/your-feature-name`)
5. 创建 Pull Request

## 许可证

此项目采用 MIT 许可证，详情请参阅 [LICENSE](LICENSE) 文件。
