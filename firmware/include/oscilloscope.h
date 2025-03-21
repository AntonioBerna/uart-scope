#pragma once

#include <Arduino.h>

class Oscilloscope {
private:
    static constexpr uint8_t channels = 4;
    const uint8_t scopeChannels[channels] = { 0, 1, 2, 3 }; // ? A0, ..., A3
    uint8_t voltageInputs[channels] = { 0 };

public:
    void initChannels(void);
    void acquireData(void);
};