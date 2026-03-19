#pragma once

#include "Widget.h"

class DayIndicatorWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    uint16_t activeColor = 0x07E0;  // green
    uint16_t inactiveColor = 0x4208; // dark gray
    bool weekStartsMonday = true;    // true: Mon..Sun, false: Sun..Sat
};
