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
};
