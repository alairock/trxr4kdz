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

    enum class IndicatorSize : uint8_t {
        PIXEL_1X1 = 1,
        PIXEL_2X2 = 2,
    };

    uint16_t colorForMode(IndicatorMode mode, unsigned long now, uint8_t slot = 0) const;
    IndicatorMode modeFromString(const String& s) const;
    const char* modeToString(IndicatorMode m) const;
    IndicatorSize sizeFromValue(JsonVariantConst value) const;
    const char* sizeToString(IndicatorSize s) const;

    bool _use24h = false;
    uint16_t _onColor = 0xD81F;      // bright magenta/purple
    uint16_t _offColor = 0x580B;     // slightly brighter dim purple (AM + unlit dots)
    IndicatorMode _indicatorMode = IndicatorMode::RED;
    bool _useOnMode = false;
    IndicatorMode _onMode = IndicatorMode::PURPLE;
    IndicatorSize _indicatorSize = IndicatorSize::PIXEL_2X2;
};
