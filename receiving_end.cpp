#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>


// 接收数据结构
typedef struct {
  int x;
  int y;
  char direction[10];
  bool buttonPressed;
} JoystickData;

// 电机引脚定义
#define LEFT_PWM   3
#define LEFT_DIR   2
#define RIGHT_PWM  6
#define RIGHT_DIR  10

// PWM 通道定义
#define PWM_LEFT_CHANNEL  0
#define PWM_RIGHT_CHANNEL 1

// 死区范围
#define DEADZONE_X_MIN 2100
#define DEADZONE_X_MAX 2130
#define DEADZONE_Y_MIN 2180
#define DEADZONE_Y_MAX 2200
#include "driver/ledc.h"

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // 初始化电机引脚
  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
  ledcSetup(PWM_LEFT_CHANNEL, 1000, 8);
  ledcSetup(PWM_RIGHT_CHANNEL, 1000, 8);
  ledcAttachPin(LEFT_PWM, PWM_LEFT_CHANNEL);
  ledcAttachPin(RIGHT_PWM, PWM_RIGHT_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW 初始化失败");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("接收端就绪，等待遥控数据...");
}

// 映射函数：4095~中心 → 0~255
int mapSpeed(int value, int center, bool reverse = false) {
  int distance = abs(value - center);
  int maxDist = max(center, 4095 - center);
  int speed = map(distance, 0, maxDist, 0, 255);
  return reverse ? 255 - speed : speed;
}

void stopMotors() {
  ledcWrite(PWM_LEFT_CHANNEL, 0);
  ledcWrite(PWM_RIGHT_CHANNEL, 0);
  Serial.println("🛑 停止电机");
}

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  JoystickData data;
  memcpy(&data, incomingData, sizeof(data));

  Serial.printf("X: %d | Y: %d | 按钮: %s\n", data.x, data.y, data.buttonPressed ? "按下" : "未按");

  // 如果在中间死区，停止
  if (data.x >= DEADZONE_X_MIN && data.x <= DEADZONE_X_MAX &&
      data.y >= DEADZONE_Y_MIN && data.y <= DEADZONE_Y_MAX) {
    stopMotors();
    return;
  }

  // === 前进或后退（两轮同向） ===
  if (data.x >= DEADZONE_X_MIN && data.x <= DEADZONE_X_MAX) {
    if (data.y <= 5) {
      // 前进
      int speed = 255;
      digitalWrite(LEFT_DIR, HIGH);
      digitalWrite(RIGHT_DIR, HIGH);
      ledcWrite(PWM_LEFT_CHANNEL, speed);
      ledcWrite(PWM_RIGHT_CHANNEL, speed);
      Serial.printf("🚗 前进 速度=%d\n", speed);
    } else if (data.y >= 4090) {
      // 后退
      int speed = 255;
      digitalWrite(LEFT_DIR, LOW);
      digitalWrite(RIGHT_DIR, LOW);
      ledcWrite(PWM_LEFT_CHANNEL, speed);
      ledcWrite(PWM_RIGHT_CHANNEL, speed);
      Serial.printf("↩️ 后退 速度=%d\n", speed);
    } else {
      // 线性控制前进/后退
      if (data.y < DEADZONE_Y_MIN) {
        int speed = mapSpeed(data.y, DEADZONE_Y_MIN);
        digitalWrite(LEFT_DIR, HIGH);
        digitalWrite(RIGHT_DIR, HIGH);
        ledcWrite(PWM_LEFT_CHANNEL, speed);
        ledcWrite(PWM_RIGHT_CHANNEL, speed);
        Serial.printf("🚗 前进 速度=%d\n", speed);
      } else if (data.y > DEADZONE_Y_MAX) {
        int speed = mapSpeed(data.y, DEADZONE_Y_MAX, true);
        digitalWrite(LEFT_DIR, LOW);
        digitalWrite(RIGHT_DIR, LOW);
        ledcWrite(PWM_LEFT_CHANNEL, speed);
        ledcWrite(PWM_RIGHT_CHANNEL, speed);
        Serial.printf("↩️ 后退 速度=%d\n", speed);
      }
    }
    return;
  }

  // === 左转（右前左后）===
  if (data.x <= 5) {
    int speed = 255;
    digitalWrite(LEFT_DIR, LOW);   // 左电机后退
    digitalWrite(RIGHT_DIR, HIGH); // 右电机前进
    ledcWrite(PWM_LEFT_CHANNEL, speed);
    ledcWrite(PWM_RIGHT_CHANNEL, speed);
    Serial.printf("↪️ 左转 速度=%d\n", speed);
    return;
  }

  // === 右转（左前右后）===
  if (data.x >= 4090) {
    int speed = 255;
    digitalWrite(LEFT_DIR, HIGH);  // 左电机前进
    digitalWrite(RIGHT_DIR, LOW);  // 右电机后退
    ledcWrite(PWM_LEFT_CHANNEL, speed);
    ledcWrite(PWM_RIGHT_CHANNEL, speed);
    Serial.printf("↩️ 右转 速度=%d\n", speed);
    return;
  }

  // 其余情况安全停
  stopMotors();
}
