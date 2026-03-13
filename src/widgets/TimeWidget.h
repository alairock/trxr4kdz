#pragma once

#include "Widget.h"

class TimeWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    bool use24h = true;
    uint16_t color = 0xFFFF;
    bool showSeconds = false;
    bool autoCenter = false;
    int16_t displayWidth = 32;
    uint8_t fontId = 1; // 0=default, 1=TomThumb, 2=Picopixel, 3=Org_01
};
