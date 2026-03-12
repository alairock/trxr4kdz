#pragma once

#include "Widget.h"

class DateWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    uint16_t color = 0x07FF; // cyan
    bool shortMonth = true;  // "Mar 11" vs "3/11"
};
