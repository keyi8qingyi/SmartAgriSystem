#include <WiFi.h>
#include <PubSubClient.h>

// 配置WiFi和OneNet信息
const char* ssid = "aaa";
const char* password = "ABCabc888";
const char* mqtt_server = "mqtts.heclouds.com"; // OneNet MQTT服务器地址
const int mqtt_port = 1883; // 端口
const char* product_id = "Pnr52oo7kr";
const char* device_id = "plant_01";
const char* auth_info = "version=2018-10-31&res=products%2FPnr52oo7kr%2Fdevices%2Fplant_01&et=20000000000&method=md5&sign=t7gxqfJo3XPzHyja%2FjSwXA%3D%3D"; // 通常为设备名称或Token

WiFiClient espClient;
PubSubClient client(espClient);

// 定义消息ID计数器（自增）
long msg_id = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi已连接");
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

// 收到订阅消息的回调函数（可选）
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("收到消息：");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// MQTT重连逻辑
void reconnect() {
  while (!client.connected()) {
    Serial.println("尝试连接MQTT服务器...");
    String clientId = device_id;
    if (client.connect(clientId.c_str(), product_id, auth_info)) {
      Serial.println("MQTT连接成功");
      // 订阅回复主题（可选）
      client.subscribe("$sys/Pnr52oo7kr/plant_01/thing/property/post/reply");
    } else {
      Serial.print("连接失败，错误码：");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 生成消息ID（自增）
  msg_id++;
  
  // 构建符合OneNet要求的Payload
  String payload = "{";
  payload += "\"id\":\"" + String(msg_id) + "\",";  // 必须包含id字段
  payload += "\"version\":\"1.0\",";                // 协议版本固定为1.0
  payload += "\"method\":\"thing.property.post\","; // 必须指定方法
  payload += "\"params\":{";
  payload += "\"soil\":{\"value\":" + String(random(20, 30)) + "}"; // 正确的JSON格式
  payload += "}}";

  // 定义MQTT主题（格式固定）
  String topic = "$sys/" + String(product_id) + "/" + String(device_id) + "/thing/property/post";

  // 发布数据
  if (client.publish(topic.c_str(), payload.c_str())) {
    Serial.println("数据发送成功");
    Serial.println("Topic: " + topic);
    Serial.println("Payload: " + payload);
  } else {
    Serial.println("数据发送失败");
  }
  
  delay(5000); // 每5秒发送一次
}
