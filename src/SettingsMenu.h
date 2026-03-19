#pragma once

#include <Arduino.h>
#include "ButtonManager.h"

class DisplayManager;
class ScreenManager;
class WiFiManager;

class SettingsMenu {
public:
    void begin(DisplayManager& display, ScreenManager& screens, WiFiManager& wifi);
    void update(ButtonEvent left, ButtonEvent right, ButtonEvent middle);
    bool isActive() const { return _active; }
    void enter();
    void exit();

private:
    void draw();
    void drawSettingsList();
    void drawFontSelector();
    void drawToggleSetting(const char* label, bool value);
    void drawConfigUrl();
    void drawDots(uint8_t count, uint8_t activeIndex);

    // Read/apply current screen state
    void snapshotState();
    void applyToggle(uint8_t settingIndex, bool value);
    bool getToggle(uint8_t settingIndex);

    DisplayManager* _display = nullptr;
    ScreenManager* _screens = nullptr;
    WiFiManager* _wifi = nullptr;
    bool _active = false;

    // Menu navigation
    uint8_t _menuIndex = 0;
    bool _inSubmenu = false;

    // Font selector state
    uint8_t _fontIndex = 0;
    uint8_t _savedFont = 1;

    // Toggle saved states (for cancel)
    bool _savedCalendar = false;
    bool _savedDayIndicator = true;

    // Config URL scroller state
    int16_t _urlScrollX = 32;
    unsigned long _urlLastStep = 0;

    // Settings: 0=Font, 1=Calendar, 2=Day Dots, 3=Config URL
    static const uint8_t NUM_SETTINGS = 4;
    static const char* SETTING_NAMES[];
};
