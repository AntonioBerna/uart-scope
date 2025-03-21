#include "oscilloscope.h"

void Oscilloscope::initChannels(void) {
  for (uint8_t i = 0; i < channels; ++i) {
    pinMode(scopeChannels[i], INPUT);
  }
}

void Oscilloscope::acquireData(void) {
  for (uint8_t i = 0; i < channels; ++i) {
    voltageInputs[i] = analogRead(scopeChannels[i]) * (5.0 / 1023.0);
  }

  for (uint8_t i = 0; i < channels; ++i) {
    Serial.print(voltageInputs[i]);
    Serial.print('\t');
  }
  
  Serial.println();
}