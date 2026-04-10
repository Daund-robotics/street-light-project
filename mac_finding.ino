#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);   // IMPORTANT
  delay(1000);

  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
