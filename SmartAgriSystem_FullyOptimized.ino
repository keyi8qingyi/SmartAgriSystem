/*
 * 项目名称：多实例智能农业系统
 * 文件说明：在原始 SmartAgricultureSystem 类的基础上，增加了多实例动态创建与切换控制逻辑。
 * 作者：ChatGPT 帮助整合
 */

 #include <Adafruit_NeoPixel.h>
 #include <WiFi.h>
 #include <DHT.h>
 #include <Wire.h>
 #include <BH1750.h>
 
 #define MAX_SYSTEMS 3
 #define RELAY_PIN 13
 #define NEOPIXEL_PIN 10
 #define MOISTURE_PIN A2
 #define DHT_PIN 4
 
 /*
 * SmartAgricultureSystem 优化版
 * 原作者功能基础上，重构为更清晰、易维护的结构。
 * 保留所有原有功能：LED 动画、光照控制、DHT、湿度控制、水泵控制、串口交互等。
 */

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>

class SmartAgricultureSystem {
public:
  // 构造函数：初始化所有引脚和组件参数
  SmartAgricultureSystem(
    int relayPin = 13,
    int neoPixelPin = 10,
    int moisturePin = A2,
    int dhtPin = 4,
    const char* ssid = "aaa",
    const char* password = "ABCabc888"
  );

  // 初始化系统，设置传感器、WiFi、LED
  void begin();

  // 循环调用函数，包含传感器更新、控制逻辑、动画等
  void update();

  // 串口命令处理入口
  void handleCommand(char cmd);

private:
  /*** 硬件与连接配置 ***/
  static constexpr uint8_t LED_COUNT = 12;
  static constexpr uint16_t MAX_LUX = 65535;
  static constexpr uint16_t WIFI_TIMEOUT = 30000;

  struct Pins {
    int relay;
    int neoPixel;
    int moisture;
    int dht;
  } pins;

  /*** 系统状态与控制参数 ***/
  struct ControlParams {
    uint16_t lightThreshold = 100;
    int moistureThreshold = 30;
    unsigned long pumpDuration = 10000;
    unsigned long moistureInterval = 3600000;
    unsigned long manualCooldown = 5000;
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

  struct SensorData {
    float temperature = 0;
    float humidity = 0;
    uint16_t light = 0;
    int soilMoisture = 0;
  } sensor;

  struct LedAnim {
    unsigned long lastUpdate = 0;
    uint8_t position = 0;
    bool clearFlag = false;
    const uint32_t colors[3] = {0xFF0000, 0x00FF00, 0x0000FF};
  } ledAnim;

  /*** 硬件对象 ***/
  BH1750 lightSensor;
  DHT dhtSensor;
  Adafruit_NeoPixel ledStrip;
  const char* wifiSSID;
  const char* wifiPassword;
  IPAddress ipAddress;

  /*** 功能函数 ***/
  void initSensors();               // 初始化传感器
  void updateSensors();            // 读取并更新传感器数据
  void controlLight();             // 自动控制LED灯带
  void controlMoisture();          // 自动控制水泵灌溉
  void startPump();                // 启动水泵
  void stopPump();                 // 停止水泵
  void runLedAnimation();          // 播放LED动画
  void manualMoistureCheck();      // 手动检测湿度
  void toggleAutoControl();        // 切换自动模式
  void setLedState(bool enable);   // 开启/关闭LED灯带
  void handleSettings();           // 设置控制参数

  /*** 输出函数 ***/
  void printSoilMoisture() const;
  void printWifiInfo() const;
  void printLightData() const;
  void printDHTData() const;
  void printHelp() const;
};

   } ledAnim;
 
   void initI2CSensors() {
     Wire.begin();
     if (!lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
       Serial.println("[ERROR] BH1750 initialization failed!");
       while(true);
     }
   }
 
   void updateSensors() {
     if (lightSensor.measurementReady()) {
       sensorData.light = lightSensor.readLightLevel();
     }
 
     float h = dhtSensor.readHumidity();
     float t = dhtSensor.readTemperature();
 
     if (isnan(h) || isnan(t)) {
       if (++state.dhtErrorCount > 5) {
         Serial.println("[ERROR] DHT11 sensor malfunction!");
       }
       return;
     }
 
     state.dhtErrorCount = 0;
     sensorData.humidity = h;
     sensorData.temperature = t;
 
     int raw = analogRead(pins.moisture);
     sensorData.soilMoisture = constrain(map(raw, 0, 4095, 0, 100), 0, 100);
   }
 
   void runLedAnimation() {
     if (!state.ledEnabled) return;
 
     if (millis() - ledAnim.lastUpdate >= 200) {
       ledAnim.lastUpdate = millis();
 
       if (!ledAnim.clearFlag) {
         ledStrip.setBrightness(50);
         ledStrip.setPixelColor(ledAnim.position, ledAnim.colors[ledAnim.position % 3]);
         if (ledAnim.position > 0) {
           ledStrip.setPixelColor(ledAnim.position - 1, 0xFFFF00);
         }
         ledStrip.show();
         if (++ledAnim.position >= LED_COUNT) {
           ledAnim.clearFlag = true;
           ledAnim.position = 0;
         }
       } else {
         ledStrip.clear();
         ledStrip.show();
         ledAnim.clearFlag = false;
       }
     }
   }
 
   void checkLightCondition() {
     if (!state.autoControl || state.manualOverride) return;
     bool condition = sensorData.light < params.lightThreshold;
     if (condition != state.ledEnabled) {
       setLedState(condition);
       Serial.print("[Auto] LED ");
       Serial.println(condition ? "ON" : "OFF");
     }
   }
 
   void checkMoistureCondition() {
     if (!state.autoControl || state.manualOverride || state.manualPumpActive) return;
     if (state.pumpRunning) return;
 
     unsigned long currentMillis = millis();
     if (currentMillis - timers.lastMoistureCheck >= params.moistureInterval) {
       timers.lastMoistureCheck = currentMillis;
       if (sensorData.soilMoisture < params.moistureThreshold) {
         startPump();
       }
     }
 
     if (state.pumpRunning && (currentMillis - timers.pumpStartTime >= params.pumpDuration)) {
       stopPump();
     }
   }
 
   void startPump() {
     digitalWrite(pins.relay, HIGH);
     state.pumpState = true;
     state.pumpRunning = true;
     timers.pumpStartTime = millis();
     Serial.println("[Pump] ON");
   }
 
   void stopPump() {
     digitalWrite(pins.relay, LOW);
     state.pumpState = false;
     state.pumpRunning = false;
     state.manualPumpActive = false;
     Serial.println("[Pump] OFF");
   }
 
 public:
   SmartAgricultureSystem(
     int relayPin = RELAY_PIN,
     int neoPixelPin = NEOPIXEL_PIN,
     int moisturePin = MOISTURE_PIN,
     int dhtPin = DHT_PIN,
     const char* ssid = "aaa",
     const char* password = "ABCabc888"
   ) : pins{relayPin, neoPixelPin, moisturePin, dhtPin},
       dhtSensor(dhtPin, DHT11),
       ledStrip(LED_COUNT, neoPixelPin, NEO_GRB + NEO_KHZ800),
       wifiSSID(ssid),
       wifiPassword(password) {}
 
   void begin() {
     pinMode(pins.relay, OUTPUT);
     digitalWrite(pins.relay, LOW);
 
     ledStrip.begin();
     ledStrip.clear();
     ledStrip.show();
 
     initI2CSensors();
     dhtSensor.begin();
 
     WiFi.begin(wifiSSID, wifiPassword);
     Serial.print("Connecting to WiFi");
     unsigned long start = millis();
     while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
       delay(500);
       Serial.print(".");
     }
 
     if (WiFi.status() == WL_CONNECTED) {
       ipAddress = WiFi.localIP();
       Serial.println("
 Connected! IP: " + ipAddress.toString());
     } else {
       Serial.println("
 WiFi connection failed!");
     }
   }
 
   void update() {
     unsigned long currentMillis = millis();
 
     if (state.manualPumpActive && (currentMillis - timers.pumpStartTime >= params.pumpDuration)) {
       stopPump();
     }
 
     if (currentMillis - timers.lastSensorUpdate >= 2000) {
       updateSensors();
       timers.lastSensorUpdate = currentMillis;
     }
 
     checkLightCondition();
     checkMoistureCondition();
     runLedAnimation();
 
     if (Serial.available() > 0) {
       handleCommand(Serial.read());
     }
   }
 
   void manualMoistureCheck() {
     unsigned long currentMillis = millis();
     if (currentMillis - state.lastManualCheck < params.manualCooldown) {
       Serial.println("[Manual] Please wait for cooldown");
       return;
     }
 
     state.lastManualCheck = currentMillis;
     updateSensors();
 
     if (sensorData.soilMoisture < params.moistureThreshold) {
       Serial.println("[Manual] Starting pump for 10s");
       startPump();
       state.manualPumpActive = true;
       timers.pumpStartTime = currentMillis;
     } else {
       Serial.println("[Manual] Moisture level is adequate");
     }
   }
 
   void handleCommand(char cmd) {
     switch(cmd) {
       case 'H': startPump(); state.manualOverride = true; break;
       case 'L': stopPump(); state.manualOverride = true; break;
       case 'P': printSoilMoisture(); break;
       case 'W': printWifiInfo(); break;
       case 'G': printLightData(); break;
       case 'T': printDHTData(); break;
       case 'I': setLedState(true); break;
       case 'O': setLedState(false); break;
       case 'A': toggleAutoControl(); break;
       case 'S': handleSettings(); break;
       case 'M': manualMoistureCheck(); break;
       default: if (cmd != '\n') printHelp(); break;
     }
   }
 
 private:
   void handleSettings() {
     delay(10);
     if (Serial.available() < 2) return;
     char type = Serial.read();
     int value = Serial.parseInt();
 
     switch(type) {
       case 'L': if(value >= 0 && value <= MAX_LUX) { params.lightThreshold = value; Serial.print("[Set] Light threshold: "); Serial.println(value); } break;
       case 'M': params.moistureThreshold = constrain(value, 0, 100); Serial.print("[Set] Moisture threshold: "); Serial.println(params.moistureThreshold); break;
       case 'D': params.pumpDuration = constrain(value, 1, 60) * 1000; Serial.print("[Set] Pump duration: "); Serial.println(value); break;
       case 'I': params.moistureInterval = constrain(value, 1, 24) * 3600000; Serial.print("[Set] Check interval: "); Serial.println(value); break;
       default: Serial.println("[Error] Invalid setting type"); break;
     }
   }
 
   void setLedState(bool enable) {
     ledAnim.clearFlag = false;
     ledAnim.position = 0;
     state.ledEnabled = enable;
     if (!enable) {
       ledStrip.clear();
       ledStrip.show();
     }
     Serial.print("[LED] ");
     Serial.println(enable ? "Enabled" : "Disabled");
   }
 
   void toggleAutoControl() {
     state.autoControl = !state.autoControl;
     state.manualOverride = false;
     Serial.print("[Auto] ");
     Serial.println(state.autoControl ? "Enabled" : "Disabled");
   }
 
 public:
   void printSoilMoisture() const {
     Serial.print("[Soil] Moisture: ");
     Serial.print(sensorData.soilMoisture);
     Serial.println("%");
   }
 
   void printWifiInfo() const {
     Serial.println("[WiFi] Connection Info:");
     Serial.print("SSID:\t"); Serial.println(wifiSSID);
     Serial.print("IP:\t"); Serial.println(ipAddress);
     Serial.print("RSSI:\t"); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
   }
 
   void printLightData() const {
     Serial.print("[GY30] Illuminance: ");
     Serial.print(sensorData.light);
     Serial.println(" lx");
   }
 
   void printDHTData() const {
     Serial.print("[DHT11] Temperature: ");
     Serial.print(sensorData.temperature, 1);
     Serial.print("°C | Humidity: ");
     Serial.print(sensorData.humidity, 1);
     Serial.println("%");
   }
 
   void printHelp() const {
     Serial.println("\nAvailable Commands:");
     Serial.println("H - Start pump");
     Serial.println("L - Stop pump");
     Serial.println("M - Manual moisture check and control");
     Serial.println("P - Print soil moisture");
     Serial.println("W - Print WiFi info");
     Serial.println("G - Print light data");
     Serial.println("T - Print DHT data");
     Serial.println("I/O - Enable/Disable LED");
     Serial.println("A - Toggle auto control");
     Serial.println("S<type><value> - Settings");
     Serial.println("  SL100 - Set light threshold");
     Serial.println("  SM30 - Set moisture threshold");
     Serial.println("  SD10 - Set pump duration(sec)");
     Serial.println("  SI2 - Set check interval(hour)");
   }
 };
 
 // ==== 实例管理器逻辑（新增） ====
 
 SmartAgricultureSystem* systems[MAX_SYSTEMS] = {nullptr};
 int currentSystemIndex = -1;
 
 void createSystem() {
   for (int i = 0; i < MAX_SYSTEMS; i++) {
     if (!systems[i]) {
       systems[i] = new SmartAgricultureSystem();
       systems[i]->begin();
       currentSystemIndex = i;
       Serial.print("[Manager] Created instance at index ");
       Serial.println(i);
       return;
     }
   }
   Serial.println("[Manager] No available slot.");
 }
 
 void deleteSystem(int index) {
   if (index >= 0 && index < MAX_SYSTEMS && systems[index]) {
     delete systems[index];
     systems[index] = nullptr;
     Serial.print("[Manager] Deleted instance ");
     Serial.println(index);
     if (currentSystemIndex == index) currentSystemIndex = -1;
   } else {
     Serial.println("[Manager] Invalid index");
   }
 }
 
 void switchSystem(int index) {
   if (index >= 0 && index < MAX_SYSTEMS && systems[index]) {
     currentSystemIndex = index;
     Serial.print("[Manager] Switched to ");
     Serial.println(index);
   } else {
     Serial.println("[Manager] Invalid index");
   }
 }
 
 void setup() {
   Serial.begin(115200);
   Serial.println("多实例农业系统就绪。命令：C=创建，D<idx>=删除，X<idx>=切换，其他为原命令");
 }
 
 

// 连接MQTT服务器
void reconnect_mqtt() {
  while (!client.connected()) {
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      // 成功连接
    } else {
      delay(5000);
    }
  }
}

// 上传数据到OneNet
void uploadToOneNet(float temperature, float humidity, float soilMoisture) {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  String payload = String("{") +
                   "\"datastreams\":[{" +
                   "\"id\":\"temperature\",\"datapoints\":[{\"value\":" + temperature + "}]}," +
                   "{\"id\":\"humidity\",\"datapoints\":[{\"value\":" + humidity + "}]}," +
                   "{\"id\":\"soil\",\"datapoints\":[{\"value\":" + soilMoisture + "}]}]}";
  String topic = String("$dp");

  client.publish(topic.c_str(), payload.c_str());
}



void loop() {
  system.update();

  float temp = system.getTemperature();
  float hum = system.getHumidity();
  float soil = system.getSoilMoisture();

  // 自动控制继电器（本地阈值逻辑）
  if (soil < soilThreshold && !relayState) {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
  } else if (soil >= soilThreshold && relayState) {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
  }

  uploadToOneNet(temp, hum, soil);


  float temp = system.getTemperature();    // 获取温度
  float hum = system.getHumidity();        // 获取湿度
  float soil = system.getSoilMoisture();   // 获取土壤湿度

  uploadToOneNet(temp, hum, soil);         // 上传数据到 OneNet

  delay(60000); // 每分钟上传一次
}



#include <ArduinoJson.h>

float soilThreshold = 30.0;  // 初始湿度阈值，可远程更改
bool relayState = false;     // 当前继电器状态

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    return;
  }

  if (doc.containsKey("relay")) {
    relayState = doc["relay"];
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  }

  if (doc.containsKey("threshold")) {
    soilThreshold = doc["threshold"];
  }
}
