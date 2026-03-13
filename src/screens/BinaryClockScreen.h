#pragma once

#include "Screen.h"

class BinaryClockScreen : public Screen {
public:
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::BINARY_CLOCK; }
    const char* typeName() const override { return "binary_clock"; }

private:
    bool _use24h = false;
    uint16_t _onColor = 0xD81F;      // bright magenta/purple
    uint16_t _offColor = 0x3006;     // dim purple (also AM indicator)
    uint16_t _pmColor = 0xF800;      // red
};
