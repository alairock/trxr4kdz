#pragma once

#include "Widget.h"

class ProgressBarWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    int16_t w = 32, h = 1;
    uint8_t percent = 0;
    uint16_t fgColor = 0x07E0; // green
    uint16_t bgColor = 0x4208; // dark gray
    bool autoProgress = true;  // auto-fill based on time of day
};
