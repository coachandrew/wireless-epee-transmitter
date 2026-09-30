#include <Arduino.h>

namespace {
constexpr uint32_t kSerialBaud = 115200;
}

void setup() {
  Serial.begin(kSerialBaud);
  delay(250);
  Serial.println("Wireless epee transmitter prototype");
  Serial.println("Hardware input and ESP-NOW protocol are not configured yet.");
}

void loop() {
  delay(1000);
}
