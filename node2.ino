#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== PINS (ESP-2 HARDWARE) ==================
#define VOLT_PIN     34
#define LDR_PIN      25
#define STREET_LED   33
#define FAULT_SWITCH 32

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== ESP-3 MAC ==================
uint8_t esp3MAC[] = {0x00, 0x70, 0x07, 0x17, 0xC1, 0xF4};

// ================== DATA STRUCT ==================
typedef struct {
  float V;
  float I;
  float P;
  bool  SL;
  bool  FAULT;
} NodeData;

// ESP-1 received data
NodeData esp1;

// ESP-2 own data
NodeData esp2;

// Combined packet to ESP-3
typedef struct {
  NodeData node1;
  NodeData node2;
} CombinedData;

CombinedData txData;

// ================== SENSOR LOGIC (SAME AS ESP-1) ==================
float readVoltage() {
  return (analogRead(VOLT_PIN) * 3.3 / 4095.0) * 5.0;
}

String readLDR() {
  return (analogRead(LDR_PIN) < 2000) ? "ON" : "OFF";
}

// ================== RECEIVE FROM ESP-1 ==================
void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {

  memcpy(&esp1, data, sizeof(esp1));

  Serial.println("📥 ESP-1 DATA RECEIVED");
}

// ================== SEND CALLBACK (ESP32 core 3.x) ==================
void onSend(const wifi_tx_info_t *info,
            esp_now_send_status_t status) {

  Serial.println(status == ESP_NOW_SEND_SUCCESS ?
                 "📤 SENT TO ESP-3" : "❌ SEND FAILED");
}

void setup() {
  Serial.begin(115200);

  pinMode(STREET_LED, OUTPUT);
  pinMode(FAULT_SWITCH, INPUT_PULLUP);
  digitalWrite(STREET_LED, HIGH);

  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  lcd.print("ESP-2 Ready");
  delay(1500);

  // ================== ESP-NOW INIT ==================
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("ESP-2 MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, esp3MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {

  // ================== ESP-2 OWN LOGIC ==================
  float sensedV = readVoltage();
  bool manualFault = (digitalRead(FAULT_SWITCH) == LOW);
  String ldrState = readLDR();

  bool streetLightON = false;
  bool fault = false;

  if (!manualFault && sensedV < 1.0) {
    digitalWrite(STREET_LED, LOW);
    streetLightON = true;
  } else {
    digitalWrite(STREET_LED, HIGH);
  }

  if (manualFault ||
     (sensedV < 1.0 && streetLightON && ldrState == "OFF")) {
    fault = true;
  }

  if (fault) {
    esp2.V = 0; esp2.I = 0; esp2.P = 0;
  }
  else if (streetLightON) {
    esp2.V = 5; esp2.I = 1; esp2.P = 5;
  }
  else {
    esp2.V = 5; esp2.I = 0; esp2.P = 0;
  }

  esp2.SL = streetLightON;
  esp2.FAULT = fault;

  // ================== LCD DISPLAY (ESP-2 OWN DATA) ==================
  lcd.clear();
  if (fault) {
    lcd.setCursor(0,0);
    lcd.print("ESP-2 STREET");
    lcd.setCursor(0,1);
    lcd.print("** FAULTY **");
  } else {
    lcd.setCursor(0,0);
    lcd.print("V:");
    lcd.print(esp2.V,1);
    lcd.print(" I:");
    lcd.print(esp2.I,1);

    lcd.setCursor(0,1);
    lcd.print("P:");
    lcd.print(esp2.P,1);
    lcd.print(" SL:");
    lcd.print(streetLightON ? "ON" : "OFF");
  }

  // ================== SEND COMBINED DATA TO ESP-3 ==================
  txData.node1 = esp1;
  txData.node2 = esp2;

  esp_now_send(esp3MAC,
               (uint8_t*)&txData,
               sizeof(txData));

  delay(2000);
}
