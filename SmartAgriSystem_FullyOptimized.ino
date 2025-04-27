/*
 * 主程序：SmartAgriSystem_Final.ino
 * 功能：管理多实例农业系统 + WiFi连接 + MQTT上传OneNet + 本地继电器控制
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "SmartAgricultureSystem.h"
#include <ArduinoJson.h>

/*** 配置参数 ***/
#define MAX_SYSTEMS 3       // 最大同时运行的实例数量
#define RELAY_PIN 13        // 水泵继电器控制引脚

/*** WiFi与MQTT配置（需要根据你的实际账号修改） ***/
const char* wifiSSID = "aaa";
const char* wifiPassword = "ABCabc888";
const char* mqttServer = "mqtt.heclouds.com";
const int mqttPort = 1883;
const char* mqttClientID = "esp32_plant01";    // 修改为你自己的OneNet Client ID
const char* mqttUsername = "Pnr52oo7kr";   // 修改为你自己的OneNet Product ID
const char* mqttPassword = "UGNVZjFSazVNZVNDMzF1empQTnBnNFVlUklPdTdjRGY=";      // 修改为你自己的OneNet APIKey

WiFiClient espClient;
PubSubClient mqttClient(espClient);

/*** 实例数组 ***/
SmartAgricultureSystem* systems[MAX_SYSTEMS] = {nullptr};
int currentSystemIndex = -1;

/*** 本地继电器控制相关变量 ***/
float soilThreshold = 30.0;  // 土壤湿度阈值
bool relayState = false;     // 当前继电器状态

// ===== MQTT连接函数 =====
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("[MQTT] Connecting...");
    if (mqttClient.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println("[MQTT] Connected!");
    } else {
      Serial.print("[MQTT] Failed. Error code: ");
      Serial.print(mqttClient.state());
      Serial.println(". Retry in 5s...");
      delay(5000);
    }
  }
}

// ===== 上传数据到OneNet函数 =====
void uploadToOneNet(float temperature, float humidity, float soilMoisture) {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  String payload = "{";
  payload += "\"datastreams\":[";
  payload += "{\"id\":\"temperature\",\"datapoints\":[{\"value\":" + String(temperature) + "}]},"; 
  payload += "{\"id\":\"humidity\",\"datapoints\":[{\"value\":" + String(humidity) + "}]},"; 
  payload += "{\"id\":\"soil\",\"datapoints\":[{\"value\":" + String(soilMoisture) + "}]}";
  payload += "]}";

  mqttClient.publish("$dp", payload.c_str());
}

// ===== MQTT消息回调函数 =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("[MQTT] Parse error");
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

// ===== 创建实例 =====
void createSystem() {
  for (int i = 0; i < MAX_SYSTEMS; i++) {
    if (!systems[i]) {
      systems[i] = new SmartAgricultureSystem(RELAY_PIN, 10, A2, 4, wifiSSID, wifiPassword);
      systems[i]->begin();
      currentSystemIndex = i;
      Serial.print("[Manager] Created instance ");
      Serial.println(i);
      return;
    }
  }
  Serial.println("[Manager] No available slots");
}

// ===== 删除实例 =====
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

// ===== 切换实例 =====
void switchSystem(int index) {
  if (index >= 0 && index < MAX_SYSTEMS && systems[index]) {
    currentSystemIndex = index;
    Serial.print("[Manager] Switched to instance ");
    Serial.println(index);
  } else {
    Serial.println("[Manager] Invalid index");
  }
}

// ===== Arduino标准setup()函数 =====
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.begin(wifiSSID, wifiPassword);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected!");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  Serial.println("[System] Ready. Commands: C=Create, D<n>=Delete, X<n>=Switch");
}

// ===== Arduino标准loop()函数 =====
unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 60000; // 1分钟上传间隔

void loop() {
  // 处理串口指令
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.print("[Serial] Received command: ");
    Serial.println(cmd);

    if (cmd == 'C') {
      createSystem();
    } else if (cmd == 'D') {
      while (!Serial.available());
      int idx = Serial.parseInt();
      deleteSystem(idx);
    } else if (cmd == 'X') {
      while (!Serial.available());
      int idx = Serial.parseInt();
      switchSystem(idx);
    } else if (currentSystemIndex >= 0 && systems[currentSystemIndex]) {
      systems[currentSystemIndex]->handleCommand(cmd);
    } else {
      Serial.println("[Warning] No active system to handle command!");
    }
  }

  // 更新当前实例（持续执行传感器采集、LED动画、自动控制等）
  if (currentSystemIndex >= 0 && systems[currentSystemIndex]) {
    systems[currentSystemIndex]->update();

    // 本地继电器控制
    float temp = systems[currentSystemIndex]->getTemperature();
    float hum = systems[currentSystemIndex]->getHumidity();
    float soil = systems[currentSystemIndex]->getSoilMoisture();

    if (soil < soilThreshold && !relayState) {
      relayState = true;
      digitalWrite(RELAY_PIN, HIGH);
    } else if (soil >= soilThreshold && relayState) {
      relayState = false;
      digitalWrite(RELAY_PIN, LOW);
    }

    // 按时间间隔上传数据
    if (millis() - lastUploadTime >= uploadInterval) {
      uploadToOneNet(temp, hum, soil);
      lastUploadTime = millis();
    }
  }
}
