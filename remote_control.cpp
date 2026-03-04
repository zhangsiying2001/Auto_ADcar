#include <WiFi.h>
#include <esp_now.h>

// 引脚定义
#define VRX_PIN 0
#define VRY_PIN 1
#define SW_PIN 10
#define LED1_PIN 12   // 板载灯 D4
#define LED2_PIN 13   // 板载灯 D5

// 中间区域阈值（调整灵敏度）
#define THRESHOLD_HIGH 3000
#define THRESHOLD_LOW  1000

// 发送的数据结构
typedef struct {
  int x;
  int y;
  char direction[10];
  bool buttonPressed;
} JoystickData;

JoystickData dataToSend;

// 接收端的 MAC 地址
uint8_t receiverMAC[] = {0xB4, 0x3A, 0x45, 0x54, 0xA9, 0x90};

// 用于LED闪烁的定时变量
unsigned long previousMillis = 0;
const long interval = 250;  // 250ms 闪一次
bool ledState = false;

void setup() {
  Serial.begin(115200);

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW 初始化失败");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("发送端初始化完成");
}

void loop() {
  // 1. LED 交替闪烁
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = !ledState;

    // 交替控制：一个亮一个灭
    digitalWrite(LED1_PIN, ledState);       // 比如 LED1 亮
    digitalWrite(LED2_PIN, !ledState);      // LED2 灭，下一次反过来
  }

  // 2. 摇杆数据读取和发送
  dataToSend.x = analogRead(VRX_PIN);
  dataToSend.y = analogRead(VRY_PIN);
  dataToSend.buttonPressed = (digitalRead(SW_PIN) == LOW);

  if (dataToSend.x < THRESHOLD_LOW) strcpy(dataToSend.direction, "left");
  else if (dataToSend.x > THRESHOLD_HIGH) strcpy(dataToSend.direction, "right");
  else if (dataToSend.y < THRESHOLD_LOW) strcpy(dataToSend.direction, "up");
  else if (dataToSend.y > THRESHOLD_HIGH) strcpy(dataToSend.direction, "down");
  else strcpy(dataToSend.direction, "center");

  esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));

  Serial.printf("发送 → X: %d | Y: %d | 方向: %s | 按钮: %s\n",
                dataToSend.x, dataToSend.y,
                dataToSend.direction,
                dataToSend.buttonPressed ? "按下" : "未按");

  delay(200);  // 发送频率 5Hz
}
