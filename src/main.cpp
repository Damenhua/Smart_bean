#include <Arduino.h>
#include "esp_camera.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include <ESP32Servo.h>

// 相機腳位設定
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32  // 電源控制腳
#define RESET_GPIO_NUM -1 // 重置腳（未使用）
#define XCLK_GPIO_NUM 0   // XCLK 時脈
#define SIOD_GPIO_NUM 26  // SDA (資料線)
#define SIOC_GPIO_NUM 27  // SCL (時脈線)
#define Y9_GPIO_NUM 35

// VSYNC, HREF, PCLK 是相機時序控制腳位
// Y2-Y9 是 8 位元影像資料腳位
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// 伺服馬達設定
#define SERVO_ENTRANCE_PIN 15 // 入口閘門伺服馬達
#define SERVO_PLASTIC_PIN 13  // 塑膠垃圾伺服馬達
#define SERVO_PAPER_PIN 12    // 紙類垃圾伺服馬達
#define SERVO_CAN_PIN 14      // 鋁箔罐垃圾伺服馬達

// 定義伺服馬達角度 - 針對SG90調整
#define GATE_CLOSE 0    // SG90的關閉位置
#define GATE_OPEN 90    // SG90的開啟位置
#define SERVO_DELAY 500 // 伺服馬達運動延遲時間

// 建立伺服馬達物件
Servo servoEntrance; // 新增入口閘門馬達
Servo servoPlastic;
Servo servoPaper;
Servo servoCan;

// 新增影像傳送相關變數
#define FRAME_BUFFER_SIZE 8192
uint8_t frameBuffer[FRAME_BUFFER_SIZE];
bool isCapturing = false;

// 函數宣告
void initCamera();
void initServos();
void controlGate(String category);
void closeAllGates();
void captureAndSendImage();
void processSerialCommand();

void setup() {
  // 關閉褐色電壓警告
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // 初始化串口通訊（提高波特率以支援影像傳輸）
  Serial.begin(115200);

  // 初始化相機
  initCamera();

  // 初始化伺服馬達
  initServos();

  // 初始狀態：關閉所有閘門
  closeAllGates();

  Serial.println("System ready!");
}

void loop() { processSerialCommand(); }

void processSerialCommand() {
  // 檢查是否有新命令
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "capture") {
      captureAndSendImage(); // 拍照命令
    } else if (command == "plastic" || command == "paper" || command == "can") {
      controlGate(command); // 控制閘門命令
      Serial.println("Gate:" + command);
    }
  }
}

void captureAndSendImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // 傳送影像大小
  Serial.print("SIZE:");
  Serial.println(fb->len);

  // 傳送影像資料
  Serial.print("DATA:");
  Serial.write(fb->buf, fb->len);
  Serial.println(); // 結束標記

  // 釋放緩衝區
  esp_camera_fb_return(fb);
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 提高解析度
  config.jpeg_quality = 12;          // 提高影像品質
  config.fb_count = 2;               // 使用兩個緩衝區以提高性能

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // 調整相機設定以獲得更好的影像品質
  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 1);
  s->set_whitebal(s, 1);
}

void initServos() {
  // 設定入口閘門
  servoEntrance.setPeriodHertz(50);
  servoEntrance.attach(SERVO_ENTRANCE_PIN, 500, 2400);

  // 設定其他閘門
  servoPlastic.setPeriodHertz(50);
  servoPlastic.attach(SERVO_PLASTIC_PIN, 500, 2400);

  servoPaper.setPeriodHertz(50);
  servoPaper.attach(SERVO_PAPER_PIN, 500, 2400);

  servoCan.setPeriodHertz(50);
  servoCan.attach(SERVO_CAN_PIN, 500, 2400);
}

void controlGate(String category) {
  // 先確保所有分類閘門關閉
  closeAllGates();
  delay(SERVO_DELAY);

  // 開啟入口閘門和對應的分類閘門
  if (category == "plastic") {
    servoEntrance.write(GATE_OPEN);
    servoPlastic.write(GATE_OPEN);
    delay(3000);
    servoEntrance.write(GATE_CLOSE);
    servoPlastic.write(GATE_CLOSE);
  } else if (category == "paper") {
    servoEntrance.write(GATE_OPEN);
    servoPaper.write(GATE_OPEN);
    delay(3000);
    servoEntrance.write(GATE_CLOSE);
    servoPaper.write(GATE_CLOSE);
  } else if (category == "Aluminium") {
    servoEntrance.write(GATE_OPEN);
    servoCan.write(GATE_OPEN);
    delay(3000);
    servoEntrance.write(GATE_CLOSE);
    servoCan.write(GATE_CLOSE);
  }
  delay(SERVO_DELAY); // 確保閘門完全關閉
}

void closeAllGates() {
  servoEntrance.write(GATE_CLOSE); // 關閉入口閘門
  servoPlastic.write(GATE_CLOSE);
  servoPaper.write(GATE_CLOSE);
  servoCan.write(GATE_CLOSE);
  delay(SERVO_DELAY);
}