#pragma once

#include "Widget.h"

class CalendarWidget : public Widget {
public:
    void draw(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    uint16_t frameColor = 0xFFFF;  // white calendar outline
    uint16_t tabColor = 0xF800;    // red top tabs
    uint16_t textColor = 0xFFFF;   // white day number
};
