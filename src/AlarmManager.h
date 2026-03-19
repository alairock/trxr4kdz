#pragma once

#include <Arduino.h>
#include "ConfigManager.h"
#include "DisplayManager.h"
#include "BuzzerManager.h"

class AlarmManager {
public:
    void begin(ConfigManager& config, BuzzerManager& buzzer);
    void update();

    bool isAlarming() const { return _isAlarming; }
    bool isSnoozed() const { return _snoozeUntil > 0 && millis() < _snoozeUntil; }
    unsigned long snoozeUntilMs() const { return _snoozeUntil; }

    void snooze();
    void dismissUntilNextSchedule();
    void render(DisplayManager& display, unsigned long now);

    void previewTone(const String& tone, uint8_t volume);
    void triggerNow();

private:
    void startAlarm();
    void stopAlarm();
    void tickTone(unsigned long now);
    uint16_t toneFreq(const String& tone, uint8_t step) const;

    ConfigManager* _config = nullptr;
    BuzzerManager* _buzzer = nullptr;

    bool _isAlarming = false;
    unsigned long _alarmStartedAt = 0;
    unsigned long _snoozeUntil = 0;
    int _dismissedYDay = -1;
    int _lastTriggeredMinute = -1;

    // tone sequencer
    unsigned long _nextToneAt = 0;
    uint8_t _toneStep = 0;
};
