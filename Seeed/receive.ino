#include <esp_now.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// ====== 引脚定义 ======
#define LIGHT_PIN    A0
#define LED_PIN      D4
#define BUZZER_PIN   D5
#define KEY_PIN      D7
#define NEOPIXEL_PIN A0
#define NUMPIXELS    60

// ====== 阈值配置 ======
#define GAS_ALARM      2000
#define TEMP_ALARM     38
#define HUMI_ALARM     70
#define LIGHT_LIMIT    1600

// ====== WiFi 配置 ======
const char* ssid     = "X60 Pro";
const char* password = "12345678";

// ====== 对象初始化 ======
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
HardwareSerial SerialWio(1);

// ====== 数据结构 ======
typedef struct {
  uint16_t seq_num;
  float temperature;
  float humidity;
  int gasVal;
  uint8_t alarmStatus;
} SensorMessage;

// ====== 全局状态 ======
SensorMessage rxData = {0};
String lastDetectType = "null";
bool lastMaskStatus = false;
bool muteFlag = false;

// ====== Web 接口处理 ======
void handleDetect() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, body);
    
    if (!err) {
      if (doc["data"]["labels"].size() > 0) {
        lastDetectType = doc["data"]["labels"][0].as<String>();
      } else {
        lastDetectType = "null";
      }
      Serial.print("识别结果：");
      Serial.println(lastDetectType);
    }
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ====== ESP-NOW 接收回调 ======
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  if (len == sizeof(rxData)) {
    memcpy(&rxData, data, sizeof(rxData));
  }
}

// ====== 更新 NeoPixel ======
void updatePixels(bool hasMask, bool noPerson) {
  pixels.clear();
  if (noPerson) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 50));  // 蓝色 = 无人
    }
  } else if (hasMask) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 50, 0));  // 绿色 = 戴口罩
    }
  } else {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(50, 0, 0));  // 红色 = 没戴
    }
  }
  pixels.show();
}

void setup() {
  Serial.begin(115200);
  SerialWio.begin(9600, SERIAL_8N1, -1, D6);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT_PULLUP);
  pinMode(LIGHT_PIN, INPUT);

  pixels.begin();
  pixels.clear();
  pixels.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接");
  // 在 WiFi 连接成功后添加
  Serial.print("WiFi 信道: ");
  Serial.println(WiFi.channel());

  server.on("/detect", HTTP_POST, handleDetect);
  server.begin();
  
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void loop() {
  int light = analogRead(LIGHT_PIN);
  float t = rxData.temperature;
  float h = rxData.humidity;
  int g = rxData.gasVal;
  bool key = digitalRead(KEY_PIN);

  if (key == LOW) {
    muteFlag = true;
    delay(200);
  }
  if (light > LIGHT_LIMIT) {
    muteFlag = false;
  }

  bool isDanger = (g > GAS_ALARM) || (t > TEMP_ALARM) || (h > HUMI_ALARM);
  bool isRemind = false;

  // ====== 🆕 口罩状态处理 ======
  bool hasValidDetection = (lastDetectType != "null" && lastDetectType != "无");
  int maskStatus = 0;
  bool noPerson = !hasValidDetection;

  

 if (hasValidDetection) {
  // 先判断是否是no mask
  bool isNoMask = lastDetectType.indexOf("no mask") >= 0;
  bool isMask = lastDetectType.indexOf("mask") >= 0 && !isNoMask;

  if(isMask){
    maskStatus = 1;
  }else if(isNoMask){
    maskStatus = 0;
    
  }
}else{
  maskStatus = 2;
}

  if (isDanger) {
    digitalWrite(LED_PIN, HIGH);
    if (!muteFlag) tone(BUZZER_PIN, 2500);
  } else {
    digitalWrite(LED_PIN, LOW);
    if (light < LIGHT_LIMIT && !muteFlag) {
      isRemind = true;
      tone(BUZZER_PIN, 1200);
      delay(200);
      noTone(BUZZER_PIN);
      delay(200);
    } else {
      noTone(BUZZER_PIN);
    }
  }

  updatePixels(lastMaskStatus, noPerson);

  SerialWio.printf("%.1f,%.1f,%d,%d,%d,%d,%d\n",
    t, h, g, light, isDanger, isRemind, maskStatus);

  Serial.printf("T:%.1f H:%.1f G:%d L:%d | 检测:%s | %s\n",
    t, h, g, light, lastDetectType.c_str(), 
    isDanger ? "⚠ 报警" : "✅ 正常");

  server.handleClient();
  delay(100);
}