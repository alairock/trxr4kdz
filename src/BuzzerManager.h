#pragma once

#include <Arduino.h>
#include "pins.h"

class BuzzerManager {
public:
    void begin();
    void ready();
    void beep(uint16_t freq = 1000, uint16_t durationMs = 150);
    void update();

private:
    bool _ready = false;
    bool _playing = false;
    unsigned long _stopAt = 0;
    static const uint8_t LEDC_CHANNEL = 0;
};
