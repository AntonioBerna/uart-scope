#include <Arduino.h>

#include "oscilloscope.h"

#define BAUD_RATE 115200 // ? Customizable baud rate for serial communication

const uint32_t interval = 20; // ? Delay between two measurements (in ms)
uint32_t lastUpdate = 0;

Oscilloscope scope = Oscilloscope();

void setup() {
  Serial.begin(BAUD_RATE);
  scope.initChannels();
  lastUpdate = millis();
}

void loop() {
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    scope.acquireData();
  }
}