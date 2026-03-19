#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "BuzzerManager.h"
#include "screens/ScreenManager.h"

class OvertakeManager {
public:
    void begin(BuzzerManager& buzzer, ScreenManager& screens);
    void update();

    // Activate using profile settings from a canvas config (no persistence of alert framebuffer)
    void trigger(int32_t durationSec, const String& soundMode, const String& tone, uint8_t volume);

    // Virtual (non-persistent) framebuffer writes for adhoc alerts
    void pushPixels(const uint8_t* rgb, size_t len);
    void pushDrawCommands(const JsonObjectConst& cmd);

    void clear();
    void stopSound();

    bool isActive() const;
    void render(DisplayManager& display);

private:
    void setPixel(int16_t x, int16_t y, uint16_t color);
    uint16_t toneFreq(const String& tone, uint8_t step) const;

    BuzzerManager* _buzzer = nullptr;
    ScreenManager* _screens = nullptr;

    uint16_t _framebuffer[32 * 8]{}; // virtual adhoc framebuffer (not saved)

    bool _active = false;
    bool _infinite = false;
    unsigned long _endAt = 0;

    String _soundMode = "off"; // off|single|repeat
    String _tone = "beep";
    uint8_t _volume = 80;
    bool _singlePlayed = false;
    bool _soundStopped = false;
    unsigned long _nextToneAt = 0;
    uint8_t _toneStep = 0;
};
