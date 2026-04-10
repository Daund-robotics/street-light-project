#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================== PINS (ESP-3 HARDWARE) ==================
#define VOLT_PIN     34
#define LDR_PIN      25
#define STREET_LED   33
#define FAULT_SWITCH 32

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================== ESP-4 MAC ==================
uint8_t esp4MAC[] = {0xC0, 0xCD, 0xD6, 0x8E, 0xD7, 0xD0};

// ================== DATA STRUCT ==================
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

// Incoming data from ESP-2
typedef struct {
  NodeData node1;
  NodeData node2;
} CombinedData;

CombinedData rx;     // from ESP-2
FinalData    tx;     // to ESP-4
NodeData     esp3;   // ESP-3 own data

// ================== SENSOR LOGIC ==================
float readVoltage() {
  return (analogRead(VOLT_PIN) * 3.3 / 4095.0) * 5.0;
}

String readLDR() {
  return (analogRead(LDR_PIN) < 2000) ? "ON" : "OFF";
}

// ================== RECEIVE FROM ESP-2 ==================
void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {

  memcpy(&rx, data, sizeof(rx));
  Serial.println("📥 Data received from ESP-2");
}

// ================== SEND CALLBACK ==================
void onSend(const wifi_tx_info_t *info,
            esp_now_send_status_t status) {

  Serial.println(status == ESP_NOW_SEND_SUCCESS ?
                 "📤 Sent to ESP-4" : "❌ Send Failed");
}

void setup() {
  Serial.begin(115200);

  pinMode(STREET_LED, OUTPUT);
  pinMode(FAULT_SWITCH, INPUT_PULLUP);
  digitalWrite(STREET_LED, HIGH);

  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  lcd.print("ESP-3 Ready");
  delay(1500);

  // ================== ESP-NOW INIT ==================
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("ESP-3 MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, esp4MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {

  // ================== ESP-3 OWN STREETLIGHT LOGIC ==================
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
    esp3.V = 0; esp3.I = 0; esp3.P = 0;
  }
  else if (streetLightON) {
    esp3.V = 5; esp3.I = 1; esp3.P = 5;
  }
  else {
    esp3.V = 5; esp3.I = 0; esp3.P = 0;
  }

  esp3.SL = streetLightON;
  esp3.FAULT = fault;

  // ================== LCD (ESP-3 OWN DATA ONLY) ==================
  lcd.clear();
  if (fault) {
    lcd.setCursor(0,0);
    lcd.print("ESP-3 STREET");
    lcd.setCursor(0,1);
    lcd.print("** FAULTY **");
  } else {
    lcd.setCursor(0,0);
    lcd.print("V:");
    lcd.print(esp3.V,1);
    lcd.print(" I:");
    lcd.print(esp3.I,1);

    lcd.setCursor(0,1);
    lcd.print("P:");
    lcd.print(esp3.P,1);
    lcd.print(" SL:");
    lcd.print(streetLightON ? "ON" : "OFF");
  }

  // ================== SEND ALL DATA TO ESP-4 ==================
  tx.node1 = rx.node1;
  tx.node2 = rx.node2;
  tx.node3 = esp3;

  esp_now_send(esp4MAC,
               (uint8_t*)&tx,
               sizeof(tx));

  delay(2000);
}
