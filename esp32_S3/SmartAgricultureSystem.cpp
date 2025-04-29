#include "SmartAgricultureSystem.h"

// ===== 构造函数 =====
SmartAgricultureSystem::SmartAgricultureSystem(int relayPin, int neoPixelPin, int moisturePin, int dhtPin, const char* ssid, const char* password)
  : pins{relayPin, neoPixelPin, moisturePin, dhtPin},
    dhtSensor(dhtPin, DHT11),
    ledStrip(LED_COUNT, neoPixelPin, NEO_GRB + NEO_KHZ800),
    wifiSSID(ssid),
    wifiPassword(password)
{}

// ===== 系统初始化 =====
void SmartAgricultureSystem::begin() {
  pinMode(pins.relay, OUTPUT);
  digitalWrite(pins.relay, LOW);

  ledStrip.begin();
  ledStrip.clear();
  ledStrip.show();

  initSensors();
  dhtSensor.begin();

  WiFi.begin(wifiSSID, wifiPassword);
  Serial.print("[WiFi] Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    ipAddress = WiFi.localIP();
    Serial.println("\n[WiFi] Connected! IP: " + ipAddress.toString());
  } else {
    Serial.println("\n[WiFi] Connection failed!");
  }
}

// ===== 传感器初始化 =====
void SmartAgricultureSystem::initSensors() {
  Wire.begin(8,9);
  if (!lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[ERROR] BH1750 initialization failed!");
    while (true);
  }
}

// ===== 传感器数据更新 =====
void SmartAgricultureSystem::updateSensors() {
  if (lightSensor.measurementReady()) {
    sensor.light = lightSensor.readLightLevel();
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
  sensor.humidity = h;
  sensor.temperature = t;

  int raw = analogRead(pins.moisture);
  sensor.soilMoisture = constrain(map(raw, 0, 4095, 0, 100), 0, 100);
}

// ===== LED灯动画 =====
void SmartAgricultureSystem::runLedAnimation() {
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

// ===== 主更新函数 =====
void SmartAgricultureSystem::update() {
  unsigned long currentMillis = millis();

  if (state.manualPumpActive && (currentMillis - timers.pumpStartTime >= params.pumpDuration)) {
    stopPump();
  }

  if (currentMillis - timers.lastSensorUpdate >= 2000) {
    updateSensors();
    timers.lastSensorUpdate = currentMillis;
  }

  controlLight();
  controlMoisture();
  runLedAnimation();

  if (Serial.available() > 0) {
    handleCommand(Serial.read());
  }
}

// ===== 光照自动控制 =====
void SmartAgricultureSystem::controlLight() {
  if (!state.autoControl || state.manualOverride) return;

  bool lowLight = (sensor.light < params.lightThreshold);
  if (lowLight != state.ledEnabled) {
    setLedState(lowLight);
    Serial.print("[Auto] LED ");
    Serial.println(lowLight ? "ON" : "OFF");
  }
}

// ===== 湿度自动控制 =====
void SmartAgricultureSystem::controlMoisture() {
  // 如果当前是手动控制模式，或者是手动触发的水泵操作，则跳过自动控制
  if (!state.autoControl || state.manualOverride || state.manualPumpActive) return;

  // 如果水泵已经在运行，且水泵的运行时间已超过设定的持续时间，则停止水泵
  if (state.pumpRunning) return;

  unsigned long currentMillis = millis();// 获取当前的时间（毫秒）

  // 检查是否已经到了湿度检测的时间间隔
  if (currentMillis - timers.lastMoistureCheck >= params.moistureInterval) {
    timers.lastMoistureCheck = currentMillis;// 更新最后一次检测的时间

    // 如果土壤湿度低于设定的阈值，则启动水泵
    if (sensor.soilMoisture < params.moistureThreshold) {
      startPump();// 启动水泵
    }
  }

  // 如果水泵正在运行，并且已超过设定的水泵运行时长，则停止水泵
  if (state.pumpRunning && (currentMillis - timers.pumpStartTime >= params.pumpDuration)) {
    stopPump();// 停止水泵
  }
}

// ===== 启动水泵 =====
void SmartAgricultureSystem::startPump() {
  digitalWrite(pins.relay, HIGH);	// 设置继电器引脚为高电平，启动水泵
  state.pumpRunning = true;			// 标记水泵正在运行
  timers.pumpStartTime = millis();	// 记录水泵启动的时间
  Serial.println("[Pump] ON"); 		// 打印水泵启动日志
}

// ===== 停止水泵 =====
void SmartAgricultureSystem::stopPump() {
  digitalWrite(pins.relay, LOW);	// 设置继电器引脚为低电平，停止水泵
  state.pumpRunning = false;		// 标记水泵已停止
  state.manualPumpActive = false;	// 重置手动控制标志
  Serial.println("[Pump] OFF");		// 打印水泵停止日志
}

// ===== 串口命令处理 =====
void SmartAgricultureSystem::handleCommand(char cmd) {
  switch(cmd) {
    case 'H': startPump(); state.manualOverride = true; break;
    case 'L': stopPump(); state.manualOverride = true; break;
    case 'M': manualMoistureCheck(); break;
    case 'P': printSoilMoisture(); break;
    case 'W': printWifiInfo(); break;
    case 'G': printLightData(); break;
    case 'T': printDHTData(); break;
    case 'I': setLedState(true); break;
    case 'O': setLedState(false); break;
    case 'A': toggleAutoControl(); break;
    case 'S': handleSettings(); break;
    default: if (cmd != '\n') printHelp(); break;
  }
}

// ===== 手动湿度检测与水泵启动 =====
void SmartAgricultureSystem::manualMoistureCheck() {
  unsigned long currentMillis = millis();// 获取当前时间

  // 检查手动湿度检测的冷却时间是否已经过去
  if (currentMillis - state.lastManualCheck < params.manualCooldown) {
  	// 如果冷却时间未结束，打印提示信息并返回
    Serial.println("[Manual] Cooldown, please wait");
    return;
  }
  
  state.lastManualCheck = currentMillis;// 更新最后一次手动检测的时间
  updateSensors();  // 重新更新一次传感器数据（读取最新的湿度值）

  // 判断土壤湿度是否低于设定的阈值
  if (sensor.soilMoisture < params.moistureThreshold) {
   	// 如果土壤湿度低于阈值，启动水泵
    Serial.println("[Manual] Soil dry, activating pump");
    startPump(); // 启动水泵
    state.manualPumpActive = true;// 标记手动水泵已激活
    timers.pumpStartTime = currentMillis;// 记录水泵启动时间
  } else {
  	// 如果土壤湿度足够，打印湿度状态
    Serial.println("[Manual] Soil moisture OK");
  }
}

// ===== 切换自动控制开关 =====
void SmartAgricultureSystem::toggleAutoControl() {
  state.autoControl = !state.autoControl;
  state.manualOverride = false; // 切换时重置手动标志
  Serial.print("[Auto] ");
  Serial.println(state.autoControl ? "Enabled" : "Disabled");
}

// ===== 控制LED灯开关 =====
void SmartAgricultureSystem::setLedState(bool enable) {
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

// ===== 串口参数设置处理 =====
void SmartAgricultureSystem::handleSettings() {
  delay(10);  // 等待数据到达
  if (Serial.available() < 2) return;  // 不足两个字符，无效

  char type = Serial.read();
  int value = Serial.parseInt();

  switch (type) {
    case 'L':  // 设置光照阈值
      if (value >= 0 && value <= MAX_LUX) {
        params.lightThreshold = value;
        Serial.print("[Set] Light threshold: ");
        Serial.println(value);
      }
      break;
    case 'M':  // 设置湿度阈值
      params.moistureThreshold = constrain(value, 0, 100);
      Serial.print("[Set] Moisture threshold: ");
      Serial.println(params.moistureThreshold);
      break;
    case 'D':  // 设置水泵持续时间（秒）
      params.pumpDuration = constrain(value, 1, 60) * 1000;
      Serial.print("[Set] Pump duration: ");
      Serial.println(value);
      break;
    case 'I':  // 设置湿度检测间隔（小时）
      params.moistureInterval = constrain(value, 1, 24) * 3600000;
      Serial.print("[Set] Moisture check interval: ");
      Serial.println(value);
      break;
    default:
      Serial.println("[Error] Invalid setting type");
  }
}

// ===== 输出土壤湿度到串口 =====
void SmartAgricultureSystem::printSoilMoisture() const {
  Serial.print("[Soil] Moisture: ");
  Serial.print(sensor.soilMoisture);
  Serial.println("%");
}

// ===== 输出WiFi连接信息到串口 =====
void SmartAgricultureSystem::printWifiInfo() const {
  Serial.println("[WiFi] Info:");
  Serial.print("SSID: ");
  Serial.println(wifiSSID);
  Serial.print("IP: ");
  Serial.println(ipAddress);
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// ===== 输出光照强度到串口 =====
void SmartAgricultureSystem::printLightData() const {
  Serial.print("[Light] Illuminance: ");
  Serial.print(sensor.light);
  Serial.println(" lx");
}

// ===== 输出温湿度数据到串口 =====
void SmartAgricultureSystem::printDHTData() const {
  Serial.print("[DHT11] Temperature: ");
  Serial.print(sensor.temperature, 1);
  Serial.print("°C | Humidity: ");
  Serial.print(sensor.humidity, 1);
  Serial.println("%");
}

// ===== 打印帮助命令列表 =====
void SmartAgricultureSystem::printHelp() const {
  Serial.println("\nAvailable Commands:");
  Serial.println("H - Start pump");
  Serial.println("L - Stop pump");
  Serial.println("M - Manual moisture check");
  Serial.println("P - Print soil moisture");
  Serial.println("W - Print WiFi info");
  Serial.println("G - Print light data");
  Serial.println("T - Print DHT data");
  Serial.println("I/O - Enable/Disable LED");
  Serial.println("A - Toggle Auto Control");
  Serial.println("S<type><value> - Set parameters (L=light, M=moisture, D=duration, I=interval)");
}

// ===== 读取温度 =====
float SmartAgricultureSystem::getTemperature() const {
  return sensor.temperature;
}

// ===== 读取湿度 =====
float SmartAgricultureSystem::getHumidity() const {
  return sensor.humidity;
}

// ===== 读取土壤湿度 =====
float SmartAgricultureSystem::getSoilMoisture() const {
  return sensor.soilMoisture;
}


