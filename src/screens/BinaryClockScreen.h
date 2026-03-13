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
    enum class IndicatorMode : uint8_t {
        WHITE,
        RED,
        GREEN,
        BLUE,
        PINK,
        PURPLE,
        RAINBOW_STATIC,
        RAINBOW_ANIMATED
    };

    uint16_t colorForMode(IndicatorMode mode, unsigned long now, uint8_t slot = 0) const;
    IndicatorMode modeFromString(const String& s) const;
    const char* modeToString(IndicatorMode m) const;

    bool _use24h = false;
    uint16_t _onColor = 0xD81F;      // bright magenta/purple
    uint16_t _offColor = 0x580B;     // slightly brighter dim purple (AM + unlit dots)
    uint16_t _pmColor = 0xF800;      // legacy PM color fallback
    IndicatorMode _indicatorMode = IndicatorMode::RED;
};
