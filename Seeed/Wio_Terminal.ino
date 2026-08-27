#include <TFT_eSPI.h>
TFT_eSPI tft;
#define BL_PIN 15

String buf = "";
float temp, humi;
int gas, light, danger, remind, mask;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 5秒刷新

// 阈值定义
const float TEMP_THRESHOLD = 38.0;
const float HUMI_THRESHOLD = 70.0;
const int GAS_THRESHOLD = 2000;
const int LIGHT_THRESHOLD = 1600;

void setup() {
  tft.begin();
  tft.setRotation(3);
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, HIGH);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Waiting data...", 20, 40);

  Serial1.begin(9600);
}

void parse(String s) {
  int p1 = s.indexOf(',');
  int p2 = s.indexOf(',', p1 + 1);
  int p3 = s.indexOf(',', p2 + 1);
  int p4 = s.indexOf(',', p3 + 1);
  int p5 = s.indexOf(',', p4 + 1);
  int p6 = s.indexOf(',', p5 + 1);

  temp = s.substring(0, p1).toFloat();
  humi = s.substring(p1 + 1, p2).toFloat();
  gas = s.substring(p2 + 1, p3).toInt();
  light = s.substring(p3 + 1, p4).toInt();
  danger = s.substring(p4 + 1, p5).toInt();
  remind = s.substring(p5 + 1, p6).toInt();
  mask = s.substring(p6 + 1).toInt();
}

void show() {
  tft.fillScreen(TFT_BLACK);

  // 标题
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 5);
  tft.print("Safety Monitor");

  // 温度
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 35);
  tft.print("Temp: ");
  if (temp > TEMP_THRESHOLD) tft.setTextColor(TFT_RED);
  else                         tft.setTextColor(TFT_CYAN);
  tft.print(temp, 1); tft.print(" C");

  // 湿度
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 65);
  tft.print("Humi: ");
  if (humi > HUMI_THRESHOLD) tft.setTextColor(TFT_RED);
  else                         tft.setTextColor(TFT_CYAN);
  tft.print(humi, 1); tft.print(" %");

  // 煤气
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 95);
  tft.print("Gas:  ");
  if (gas > GAS_THRESHOLD) tft.setTextColor(TFT_RED);
  else                       tft.setTextColor(TFT_CYAN);
  tft.print(gas);

  // 光照
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 125);
  tft.print("Light:");
  if (light < LIGHT_THRESHOLD) tft.setTextColor(TFT_RED);
  else                           tft.setTextColor(TFT_CYAN);
  tft.print(light);

  // 状态
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 160);
  if (danger) {
    tft.setTextColor(TFT_RED);
    tft.print("Status: DANGER ALERT");
  } else if (remind) {
    tft.setTextColor(TFT_YELLOW);
    tft.print("Status: Turn off light");
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.print("Status: Normal");
  }

 tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 190);
  tft.print("Mask: ");
  
  if (mask == 2) {
    tft.setTextColor(TFT_YELLOW);
    tft.print("No Person");
  } else if (mask == 1) {
    tft.setTextColor(TFT_GREEN);
    tft.print("Worn");
  } else {
    tft.setTextColor(TFT_RED);
    tft.print("Not Worn");
  }
  
  // 报警提示信息
  if (remind) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 220);
    tft.print("check gas,water,electronic");
  }
}


void loop() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      parse(buf);
      buf = "";
    } else {
      buf += c;
    }
  }

  if (millis() - lastUpdate >= updateInterval) {
    show();
    lastUpdate = millis();
  }
}