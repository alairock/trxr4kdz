#pragma once

#include "Screen.h"
#include "../widgets/TimeWidget.h"
#include "../widgets/DateWidget.h"
#include "../widgets/IconWidget.h"
#include "../widgets/ProgressBarWidget.h"
#include "../widgets/DayIndicatorWidget.h"

class ClockScreen : public Screen {
public:
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::CLOCK; }
    const char* typeName() const override { return "clock"; }

private:
    void layoutWidgets();
    void applyTimezone();

    TimeWidget _time;
    DateWidget _date;
    IconWidget _icon;
    ProgressBarWidget _progress;
    DayIndicatorWidget _dayIndicator;

    bool _showTime = true;
    bool _showDate = false;
    bool _showIcon = false;
    bool _showProgress = false;
    bool _showDayIndicator = true;

    bool _use24h = false;
    String _timezone = "MST7MDT,M3.2.0,M11.1.0"; // Mountain
};
