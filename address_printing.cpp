#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // ESP-NOW 必须处于 STA 模式
  Serial.print("ESP32-C3 的 MAC 地址是：");
  Serial.println(WiFi.macAddress());  // 打印 MAC 地址
}

void loop() {
  // 无操作
//B4:3A:45:54:A9:90
}
