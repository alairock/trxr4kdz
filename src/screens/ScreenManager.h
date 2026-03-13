#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Screen.h"

class DisplayManager;
class ConfigManager;
class CanvasScreen;
class WeatherService;

#define MAX_SCREENS 12

class ScreenManager {
public:
    void begin(DisplayManager& display, WeatherService* weather = nullptr);
    void update();

    // Screen lifecycle
    Screen* addScreen(ScreenType type, const String& id = "");
    bool removeScreen(const String& id);
    Screen* getScreen(const String& id);
    CanvasScreen* getCanvasScreen(const String& id);
    Screen* getActiveScreen();
    uint8_t getScreenCount() const { return _count; }

    // Navigation
    void nextScreen();
    void prevScreen();
    void goToScreen(const String& id);

    // Cycling
    void setCycling(bool on) { _cycling = on; _lastSwitch = millis(); }
    bool isCycling() const { return _cycling; }

    // Ordering
    void reorder(const String* ids, uint8_t count);

    // Persistence
    bool load();
    bool save();

    // Serialization for API
    void serializeAll(JsonDocument& doc) const;
    void serializeScreen(const String& id, JsonDocument& doc) const;

private:
    Screen* createScreenByType(ScreenType type);
    ScreenType typeFromString(const char* name);
    int findNextEnabled(int from, int dir = 1);
    void sortByOrder();
    String generateId(const char* typeName);

    Screen* _screens[MAX_SCREENS] = {};
    uint8_t _count = 0;
    int8_t _activeIndex = -1;
    bool _cycling = true;
    unsigned long _lastSwitch = 0;
    DisplayManager* _display = nullptr;
    WeatherService* _weatherService = nullptr;
    bool _started = false;
};
