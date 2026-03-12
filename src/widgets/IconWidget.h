#pragma once

#include "Widget.h"

class IconWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    uint16_t iconId = 0;
    uint16_t color = 0xFFFF;

    // Built-in 8x8 icons (stored as 8 bytes, 1 bit per pixel)
    static const uint8_t ICON_CLOCK[];
    static const uint8_t ICON_TEXT[];
    static const uint8_t ICON_SMILE[];
};
