#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------- PINS --------
#define VOLT_PIN     34
#define LDR_PIN      25
#define STREET_LED   33
#define FAULT_SWITCH 32

LiquidCrystal_I2C lcd(0x27, 16, 2);

// 🔴 RECEIVER MAC (YOUR MAC)
uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0x1C, 0x04, 0xFC};

// -------- DATA STRUCT --------
typedef struct {
  float V;
  float I;
  float P;
  bool  SL;
  bool  FAULT;
} NodeData;

NodeData txData;

// -------- SENSOR LOGIC (UNCHANGED) --------
float readVoltage() {
  return (analogRead(VOLT_PIN) * 3.3 / 4095.0) * 5.0;
}

String readLDR() {
  return (analogRead(LDR_PIN) < 2000) ? "ON" : "OFF";
}

// -------- SEND CALLBACK (FIXED FOR ESP32 CORE 3.x) --------
void onSend(const wifi_tx_info_t *info,
            esp_now_send_status_t status) {

  Serial.println(status == ESP_NOW_SEND_SUCCESS ?
                 "ESP-NOW SENT" : "ESP-NOW FAILED");
}

void setup() {
  Serial.begin(115200);

  pinMode(STREET_LED, OUTPUT);
  pinMode(FAULT_SWITCH, INPUT_PULLUP);
  digitalWrite(STREET_LED, HIGH);

  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  lcd.print("Node-1 Ready");
  delay(1500);

  // -------- ESP-NOW INIT --------
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

  esp_now_add_peer(&peer);
}

void loop() {

  float sensedV = readVoltage();
  bool manualFault = (digitalRead(FAULT_SWITCH) == LOW);
  String LDR_State = readLDR();

  bool streetLightON = false;
  bool fault = false;

  // -------- STREET LIGHT LOGIC --------
  if (!manualFault && sensedV < 1.0) {
    digitalWrite(STREET_LED, LOW);
    streetLightON = true;
  } else {
    digitalWrite(STREET_LED, HIGH);
  }

  // -------- FAULT LOGIC --------
  if (manualFault ||
     (sensedV < 1.0 && streetLightON && LDR_State == "OFF")) {
    fault = true;
  }

  // -------- DISPLAY VALUES --------
  if (fault) {
    txData.V = 0;
    txData.I = 0;
    txData.P = 0;
  }
  else if (streetLightON) {
    txData.V = 5;
    txData.I = 1;
    txData.P = 5;
  }
  else {
    txData.V = 5;
    txData.I = 0;
    txData.P = 0;
  }

  txData.SL = streetLightON;
  txData.FAULT = fault;

  // -------- LCD --------
  lcd.clear();
  if (fault) {
    lcd.setCursor(0,0);
    lcd.print("STREET LIGHT");
    lcd.setCursor(0,1);
    lcd.print("** FAULTY **");
  } else {
    lcd.setCursor(0,0);
    lcd.print("V:");
    lcd.print(txData.V,1);
    lcd.print(" I:");
    lcd.print(txData.I,1);

    lcd.setCursor(0,1);
    lcd.print("P:");
    lcd.print(txData.P,1);
    lcd.print(" SL:");
    lcd.print(streetLightON ? "ON" : "OFF");
  }

  // -------- SEND DATA --------
  esp_now_send(receiverMAC,
               (uint8_t*)&txData,
               sizeof(txData));

  delay(2000);
}
