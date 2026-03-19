#include "AlarmManager.h"
#include <time.h>
#include <FastLED.h>

void AlarmManager::begin(ConfigManager& config, BuzzerManager& buzzer) {
    _config = &config;
    _buzzer = &buzzer;
}

void AlarmManager::startAlarm() {
    _isAlarming = true;
    _alarmStartedAt = millis();
    _nextToneAt = 0;
    _toneStep = 0;
}

void AlarmManager::stopAlarm() {
    _isAlarming = false;
    _alarmStartedAt = 0;
    _nextToneAt = 0;
}

void AlarmManager::snooze() {
    if (!_isAlarming || !_config) return;
    stopAlarm();
    _snoozeUntil = millis() + (unsigned long)_config->getAlarmSnoozeMinutes() * 60UL * 1000UL;
}

void AlarmManager::dismissUntilNextSchedule() {
    if (!_isAlarming) return;
    stopAlarm();
    time_t nowT = time(nullptr);
    struct tm lt{};
    localtime_r(&nowT, &lt);
    _dismissedYDay = lt.tm_yday;
    _snoozeUntil = 0;
}

uint16_t AlarmManager::toneFreq(const String& tone, uint8_t step) const {
    if (tone == "chime") {
        static const uint16_t seq[] = {784, 988, 1175, 988};
        return seq[step % 4];
    }
    if (tone == "pulse") {
        static const uint16_t seq[] = {880, 660};
        return seq[step % 2];
    }
    // default beep
    return 1000;
}

void AlarmManager::previewTone(const String& tone, uint8_t volume) {
    if (!_buzzer) return;
    _buzzer->beep(toneFreq(tone, 0), 180, volume);
}

void AlarmManager::triggerNow() {
    _snoozeUntil = 0;
    _dismissedYDay = -1;
    startAlarm();
}

void AlarmManager::tickTone(unsigned long now) {
    if (!_buzzer || !_config) return;
    if (now < _nextToneAt) return;

    String tone = _config->getAlarmTone();
    uint16_t f = toneFreq(tone, _toneStep++);
    _buzzer->beep(f, 170, _config->getAlarmVolume());
    _nextToneAt = now + 260;
}

void AlarmManager::update() {
    if (!_config) return;

    unsigned long nowMs = millis();

    if (_isAlarming) {
        tickTone(nowMs);
        if (_config->getAlarmTimeoutMinutes() > 0 &&
            (nowMs - _alarmStartedAt) >= (unsigned long)_config->getAlarmTimeoutMinutes() * 60UL * 1000UL) {
            dismissUntilNextSchedule();
        }
        return;
    }

    if (!_config->getAlarmEnabled()) return;
    if (_snoozeUntil > 0 && nowMs < _snoozeUntil) return;

    time_t nowT = time(nullptr);
    if (nowT < 100000) return; // clock not synced yet

    struct tm lt{};
    localtime_r(&nowT, &lt);

    if (_dismissedYDay == lt.tm_yday) return;

    uint8_t wday = (uint8_t)lt.tm_wday; // 0=Sun
    uint8_t mask = _config->getAlarmDaysMask();
    if ((mask & (1 << wday)) == 0) return;

    if ((uint8_t)lt.tm_hour != _config->getAlarmHour() || (uint8_t)lt.tm_min != _config->getAlarmMinute()) return;

    int minuteKey = lt.tm_yday * 1440 + lt.tm_hour * 60 + lt.tm_min;
    if (minuteKey == _lastTriggeredMinute) return;
    _lastTriggeredMinute = minuteKey;

    startAlarm();
}

void AlarmManager::render(DisplayManager& display, unsigned long now) {
    if (!_isAlarming || !_config) return;

    bool on = ((now / 300) % 2) == 0;
    display.clear();
    if (!on) return;

    if (_config->getAlarmFlashMode() == "rainbow") {
        uint8_t hue = (uint8_t)((now / 20) & 0xFF);
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            for (int x = 0; x < MATRIX_WIDTH; x++) {
                CRGB c = CHSV((uint8_t)(hue + x * 4 + y * 8), 255, 255);
                display.drawPixel(x, y, DisplayManager::rgb565(c.r, c.g, c.b));
            }
        }
    } else {
        uint16_t c = DisplayManager::hexToColor(_config->getAlarmColor());
        display.fillRect(0, 0, MATRIX_WIDTH, MATRIX_HEIGHT, c);
    }
}
