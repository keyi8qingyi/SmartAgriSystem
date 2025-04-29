#ifndef SMART_AGRICULTURE_SYSTEM_H
#define SMART_AGRICULTURE_SYSTEM_H

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>

/**
 * @brief 智能农业系统类（每个实例独立管理传感器采集、LED动画、水泵灌溉）
 */
class SmartAgricultureSystem {
public:
    // === 构造函数 ===
    SmartAgricultureSystem(
        int relayPin = 13,
        int neoPixelPin = 10,
        int moisturePin = A2,
        int dhtPin = 4,
        const char* ssid = "aaa",
        const char* password = "ABCabc888"
    );

    // === 外部调用接口 ===
    void begin();                      // 系统初始化
    void update();                     // 周期性更新（传感器采集 + 自动控制 + LED动画）
    void handleCommand(char cmd);       // 串口命令解析

    // 用于外部读取数据（比如 OneNet 上传）
    float getTemperature() const;
    float getHumidity() const;
    float getSoilMoisture() const;

private:
    /*** 常量定义 ***/
    static constexpr uint8_t LED_COUNT = 12;       // LED灯珠数
    static constexpr uint16_t MAX_LUX = 65535;     // 光照传感器最大量程
    static constexpr uint16_t WIFI_TIMEOUT = 30000;// WiFi连接超时时间(ms)

    /*** 引脚配置 ***/
    struct Pins {
        int relay;
        int neoPixel;
        int moisture;
        int dht;
    } pins;

    /*** 数据结构定义 ***/
    struct SensorData {
        float temperature = 0;
        float humidity = 0;
        uint16_t light = 0;
        int soilMoisture = 0;
    } sensor;

    struct ControlParams {
        uint16_t lightThreshold = 100;     // 光照阈值
        int moistureThreshold = 30;        // 土壤湿度阈值
        unsigned long pumpDuration = 10000; // 水泵持续时间(ms)
        unsigned long moistureInterval = 3600000; // 湿度检测间隔(ms)
        unsigned long manualCooldown = 5000;      // 手动检测冷却时间(ms)
    } params;

    struct State {
        bool ledEnabled = false;
        bool pumpRunning = false;
        bool autoControl = true;
        bool manualOverride = false;
        bool manualPumpActive = false;
        uint8_t dhtErrorCount = 0;
        unsigned long lastManualCheck = 0;
    } state;

    struct Timers {
        unsigned long lastSensorUpdate = 0;
        unsigned long lastMoistureCheck = 0;
        unsigned long pumpStartTime = 0;
    } timers;

    struct LedAnim {
        unsigned long lastUpdate = 0;
        uint8_t position = 0;
        bool clearFlag = false;
        const uint32_t colors[3] = {0xFF0000, 0x00FF00, 0x0000FF};
    } ledAnim;

    /*** 硬件对象 ***/
    Adafruit_NeoPixel ledStrip;
    BH1750 lightSensor;
    DHT dhtSensor;
    const char* wifiSSID;
    const char* wifiPassword;
    IPAddress ipAddress;

    /*** 内部私有方法 ***/
    void initSensors();               // 初始化传感器
    void updateSensors();             // 更新各传感器数据
    void controlLight();              // 自动控制LED灯带开关
    void controlMoisture();           // 自动控制水泵
    void startPump();                 // 启动水泵
    void stopPump();                  // 停止水泵
    void runLedAnimation();           // 播放LED动画
    void manualMoistureCheck();        // 手动触发土壤湿度检测
    void toggleAutoControl();          // 切换自动/手动控制模式
    void setLedState(bool enable);     // 设置LED开关状态
    void handleSettings();             // 处理串口设置命令（如阈值修改）

    /*** 打印信息到串口 ***/
    void printSoilMoisture() const;
    void printWifiInfo() const;
    void printLightData() const;
    void printDHTData() const;
    void printHelp() const;
};

#endif // SMART_AGRICULTURE_SYSTEM_H

