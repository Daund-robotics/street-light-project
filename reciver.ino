#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ================= DATA STRUCT =================
typedef struct {
  float V;
  float I;
  float P;
  bool  SL;
  bool  FAULT;
} NodeData;

typedef struct {
  NodeData node1;
  NodeData node2;
  NodeData node3;
} FinalData;

FinalData rx;

// ================= RECEIVE CALLBACK =================
void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {

  memcpy(&rx, data, sizeof(rx));
  updateLCD();
  sendUART();
}

// ================= LCD FUNCTIONS =================
void printNode(int row, const char* name, NodeData n) {
  lcd.setCursor(0, row);
  lcd.print(name);
  lcd.print(" V:");
  lcd.print((int)n.V);
  lcd.print(" I:");
  lcd.print((int)n.I);
  lcd.print(" P:");
  lcd.print((int)n.P);
  lcd.print(" ");

  if (n.FAULT) lcd.print("FA");
  else lcd.print(n.SL ? "ON" : "OF");
}

void updateLCD() {
  lcd.clear();
  printNode(0, "ESP1", rx.node1);
  printNode(1, "ESP2", rx.node2);
  printNode(2, "ESP3", rx.node3);
  lcd.setCursor(0,3);
  lcd.print("--------------------");
}

// ================= UART SEND =================
void sendUART() {
  Serial2.print("ESP1,");
  Serial2.print(rx.node1.V); Serial2.print(",");
  Serial2.print(rx.node1.I); Serial2.print(",");
  Serial2.print(rx.node1.P); Serial2.print(",");
  Serial2.print(rx.node1.SL); Serial2.print(",");
  Serial2.print(rx.node1.FAULT); Serial2.print(";");

  Serial2.print("ESP2,");
  Serial2.print(rx.node2.V); Serial2.print(",");
  Serial2.print(rx.node2.I); Serial2.print(",");
  Serial2.print(rx.node2.P); Serial2.print(",");
  Serial2.print(rx.node2.SL); Serial2.print(",");
  Serial2.print(rx.node2.FAULT); Serial2.print(";");

  Serial2.print("ESP3,");
  Serial2.print(rx.node3.V); Serial2.print(",");
  Serial2.print(rx.node3.I); Serial2.print(",");
  Serial2.print(rx.node3.P); Serial2.print(",");
  Serial2.print(rx.node3.SL); Serial2.print(",");
  Serial2.print(rx.node3.FAULT);
  Serial2.println();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // ✅ DO NOT declare Serial2, only begin it
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16 TX=17

  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  lcd.print("ESP-4 MONITOR");
  delay(1500);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    lcd.clear();
    lcd.print("ESP-NOW FAIL");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {}
