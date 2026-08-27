/* 1. 头文件包含 */
#include <esp_now.h>  // ESP-NOW核心库
#include <WiFi.h>     // WiFi功能支持
#include <DHT.h>      // DHT11传感器库

/* 引脚定义 */
#define DHT_PIN     D2    // DHT11数据引脚
#define MQ2_PIN     A0    // MQ2煤气传感器引脚
#define DHT_TYPE    DHT11 // 传感器类型

/* 2. 接收端MAC地址定义 */
uint8_t receiverMac[] = {0xE8, 0xF6, 0x0A, 0x8B, 0x2E, 0x8C};

/* 3. 数据结构定义（保持你原来的格式！） */
typedef struct message {
  uint16_t seq_num;       // 序列号
  float temperature;      // 温度
  float humidity;         // 湿度
  int gasVal;             // 煤气浓度（替换掉光照）
  uint8_t alarmStatus;    // 报警状态
} message;

/* 初始化DHT */
DHT dht(DHT_PIN, DHT_TYPE);

/* 4. 回调函数声明 */ 
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) { 
  (void)info;  // 忽略 info 参数（如果不需要的话）
  Serial.printf("发送状态: %s\n", 
               status == ESP_NOW_SEND_SUCCESS ? "成功" : "失败"); 
}

/* 5. 初始化设置 */
void setup() {
  Serial.begin(115200);
  dht.begin(); // 启动传感器

  WiFi.mode(WIFI_STA);
  WiFi.begin("X60 Pro", "12345678");  // 连接到同一个WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接");
  Serial.print("发送端信道: ");
  Serial.println(WiFi.channel());  // 打印确认信道

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW初始化失败");
    ESP.restart();
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 6;  // 改为11，与接收端一致
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("添加对端设备失败");
    return;
  }

  Serial.println("===== ESP-NOW 温湿度 + 煤气监测 启动 =====");
}

/* 6. 主循环 */
void loop() {
  static uint16_t sequence = 0;

  // 读取真实传感器数据
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  int gasVal = analogRead(MQ2_PIN);

  // 数据校验
  if (isnan(temp) || isnan(humi)) {
    Serial.println("DHT11 读取失败！");
    delay(1000);
    return;
  }

  // 组装数据包
  message myData;
  myData.seq_num = sequence++;
  myData.temperature = temp;
  myData.humidity = humi;
  myData.gasVal = gasVal;
  myData.alarmStatus = 0;

  // 发送
  esp_now_send(receiverMac, (uint8_t *)&myData, sizeof(myData));

  // 串口打印本地数据
  Serial.print("温度："); Serial.print(temp); Serial.print(" ℃ | ");
  Serial.print("湿度："); Serial.print(humi); Serial.print(" % | ");
  Serial.print("煤气："); Serial.print(gasVal);
  Serial.println();

  delay(1000);
}