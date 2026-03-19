#pragma once

#include "Screen.h"

class BatteryScreen : public Screen {
public:
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;

    ScreenType type() const override { return ScreenType::BATTERY; }
    const char* typeName() const override { return "battery"; }

private:
    uint8_t readBatteryPercent() const;

    // Battery calibration defaults (TC001 baseline)
    uint16_t _minRaw = 475;
    uint16_t _maxRaw = 665;

    uint16_t _textColor = 0xFFFF;
    uint16_t _lowColor = 0xF800;
    uint8_t _lowThreshold = 20;
};
