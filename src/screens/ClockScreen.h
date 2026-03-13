#pragma once

#include "Screen.h"
#include "../widgets/TimeWidget.h"
#include "../widgets/DateWidget.h"
#include "../widgets/IconWidget.h"
#include "../widgets/ProgressBarWidget.h"
#include "../widgets/DayIndicatorWidget.h"
#include "../widgets/CalendarWidget.h"

class ClockScreen : public Screen {
public:
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::CLOCK; }
    const char* typeName() const override { return "clock"; }

    uint8_t getFontId() const { return _fontId; }
    void setFontId(uint8_t id) { _fontId = id; _time.fontId = id; layoutWidgets(); }

    bool getShowCalendar() const { return _showCalendar; }
    void setShowCalendar(bool v) { _showCalendar = v; layoutWidgets(); }

    bool getShowDayIndicator() const { return _showDayIndicator; }
    void setShowDayIndicator(bool v) { _showDayIndicator = v; layoutWidgets(); }

private:
    void layoutWidgets();
    void applyTimezone();

    TimeWidget _time;
    DateWidget _date;
    IconWidget _icon;
    ProgressBarWidget _progress;
    DayIndicatorWidget _dayIndicator;
    CalendarWidget _calendar;

    bool _showTime = true;
    bool _showDate = false;
    bool _showIcon = false;
    bool _showProgress = false;
    bool _showDayIndicator = true;
    bool _showCalendar = false;

    bool _use24h = false;
    String _timezone = "MST7MDT,M3.2.0,M11.1.0"; // Mountain
    uint8_t _fontId = 1; // 0=default(5x7), 1=TomThumb(3x5), 2=Picopixel(3x5), 3=Org_01(5x6)
};
