#include "ScreenManager.h"
#include "../DisplayManager.h"
#include "ClockScreen.h"
#include "TextTickerScreen.h"
#include "CanvasScreen.h"
#include "WeatherScreen.h"
#include "../WeatherService.h"
#include <LittleFS.h>

void ScreenManager::begin(DisplayManager& display, WeatherService* weather) {
    _display = &display;
    _weatherService = weather;

    if (!load()) {
        Serial.println("[Screens] No config found, creating default clock");
        Screen* clock = addScreen(ScreenType::CLOCK, "clock-1");
        if (clock) {
            // Default config
            JsonDocument doc;
            JsonObject settings = doc.to<JsonObject>();
            settings["use24h"] = false;
            settings["timezone"] = "MST7MDT,M3.2.0,M11.1.0";
            JsonArray comps = settings["complications"].to<JsonArray>();
            comps.add("time");
            comps.add("day_indicator");
            clock->configure(static_cast<JsonObjectConst>(settings));
            save();
        }
    }

    // Activate first enabled screen
    _activeIndex = findNextEnabled(-1);
    if (_activeIndex >= 0) {
        _screens[_activeIndex]->activate();
        _lastSwitch = millis();
    }

    _started = true;
    Serial.printf("[Screens] Loaded %d screens, active: %d\n", _count, _activeIndex);
}

void ScreenManager::update() {
    if (!_display || _activeIndex < 0) return;

    unsigned long now = millis();
    Screen* active = _screens[_activeIndex];

    // Check if it's time to cycle
    if (_cycling && active->durationSec > 0) {
        if (now - _lastSwitch >= (unsigned long)active->durationSec * 1000UL) {
            nextScreen();
            active = _screens[_activeIndex];
        }
    }

    // Update active screen
    active->update(*_display, now);
}

Screen* ScreenManager::addScreen(ScreenType type, const String& id) {
    if (_count >= MAX_SCREENS) return nullptr;

    Screen* s = createScreenByType(type);
    if (!s) return nullptr;

    s->id = id.length() > 0 ? id : generateId(s->typeName());
    s->order = _count;
    _screens[_count++] = s;
    return s;
}

bool ScreenManager::removeScreen(const String& id) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_screens[i]->id == id) {
            // Deactivate if active
            if (i == _activeIndex) {
                _screens[i]->deactivate();
            }
            delete _screens[i];
            // Shift remaining
            for (uint8_t j = i; j < _count - 1; j++) {
                _screens[j] = _screens[j + 1];
            }
            _screens[--_count] = nullptr;

            // Fix active index
            if (_activeIndex >= _count) {
                _activeIndex = findNextEnabled(-1);
                if (_activeIndex >= 0) _screens[_activeIndex]->activate();
            } else if (_activeIndex > i) {
                _activeIndex--;
            }
            return true;
        }
    }
    return false;
}

Screen* ScreenManager::getScreen(const String& id) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_screens[i]->id == id) return _screens[i];
    }
    return nullptr;
}

CanvasScreen* ScreenManager::getCanvasScreen(const String& id) {
    Screen* s = getScreen(id);
    if (s && s->type() == ScreenType::CANVAS) {
        return static_cast<CanvasScreen*>(s);
    }
    return nullptr;
}

Screen* ScreenManager::getActiveScreen() {
    if (_activeIndex >= 0 && _activeIndex < _count) return _screens[_activeIndex];
    return nullptr;
}

void ScreenManager::nextScreen() {
    if (_count == 0) return;
    int next = findNextEnabled(_activeIndex, 1);
    if (next >= 0 && next != _activeIndex) {
        if (_activeIndex >= 0) _screens[_activeIndex]->deactivate();
        _activeIndex = next;
        _screens[_activeIndex]->activate();
        _lastSwitch = millis();
    }
}

void ScreenManager::prevScreen() {
    if (_count == 0) return;
    int prev = findNextEnabled(_activeIndex, -1);
    if (prev >= 0 && prev != _activeIndex) {
        if (_activeIndex >= 0) _screens[_activeIndex]->deactivate();
        _activeIndex = prev;
        _screens[_activeIndex]->activate();
        _lastSwitch = millis();
    }
}

void ScreenManager::goToScreen(const String& id) {
    for (uint8_t i = 0; i < _count; i++) {
        if (_screens[i]->id == id && _screens[i]->enabled) {
            if (_activeIndex >= 0) _screens[_activeIndex]->deactivate();
            _activeIndex = i;
            _screens[_activeIndex]->activate();
            _lastSwitch = millis();
            return;
        }
    }
}

void ScreenManager::reorder(const String* ids, uint8_t count) {
    for (uint8_t i = 0; i < count && i < _count; i++) {
        Screen* s = getScreen(ids[i]);
        if (s) s->order = i;
    }
    sortByOrder();
    // Re-find active screen index after reorder
    Screen* active = getActiveScreen();
    if (active) {
        for (uint8_t i = 0; i < _count; i++) {
            if (_screens[i] == active) { _activeIndex = i; break; }
        }
    }
}

int ScreenManager::findNextEnabled(int from, int dir) {
    if (_count == 0) return -1;
    int start = (from + dir + _count) % _count;
    for (uint8_t i = 0; i < _count; i++) {
        int idx = (start + i * dir + _count) % _count;
        if (_screens[idx]->enabled) return idx;
    }
    return -1;
}

void ScreenManager::sortByOrder() {
    // Simple insertion sort (max 12 elements)
    for (uint8_t i = 1; i < _count; i++) {
        Screen* key = _screens[i];
        int j = i - 1;
        while (j >= 0 && _screens[j]->order > key->order) {
            _screens[j + 1] = _screens[j];
            j--;
        }
        _screens[j + 1] = key;
    }
}

String ScreenManager::generateId(const char* typeName) {
    // Generate id like "clock-1", "ticker-2"
    for (int n = 1; n < 100; n++) {
        String candidate = String(typeName) + "-" + String(n);
        bool taken = false;
        for (uint8_t i = 0; i < _count; i++) {
            if (_screens[i]->id == candidate) { taken = true; break; }
        }
        if (!taken) return candidate;
    }
    return String(typeName) + "-" + String(millis());
}

Screen* ScreenManager::createScreenByType(ScreenType type) {
    switch (type) {
        case ScreenType::CLOCK: return new ClockScreen();
        case ScreenType::TEXT_TICKER: return new TextTickerScreen();
        case ScreenType::CANVAS: return new CanvasScreen();
        case ScreenType::WEATHER: {
            WeatherScreen* ws = new WeatherScreen();
            ws->setWeatherService(_weatherService);
            return ws;
        }
        default: return nullptr;
    }
}

ScreenType ScreenManager::typeFromString(const char* name) {
    if (strcmp(name, "clock") == 0) return ScreenType::CLOCK;
    if (strcmp(name, "text_ticker") == 0) return ScreenType::TEXT_TICKER;
    if (strcmp(name, "canvas") == 0) return ScreenType::CANVAS;
    if (strcmp(name, "weather") == 0) return ScreenType::WEATHER;
    return ScreenType::CLOCK; // default
}

// Persistence
bool ScreenManager::load() {
    File f = LittleFS.open("/screens.json", "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[Screens] Parse error: %s\n", err.c_str());
        return false;
    }

    _cycling = doc["cycling"] | true;

    JsonArray screens = doc["screens"].as<JsonArray>();
    if (!screens) return false;

    for (JsonObject s : screens) {
        const char* typeStr = s["type"] | "clock";
        ScreenType type = typeFromString(typeStr);
        Screen* screen = addScreen(type, s["id"] | "");
        if (!screen) continue;

        screen->enabled = s["enabled"] | true;
        screen->durationSec = s["duration"] | 10;
        screen->order = s["order"] | screen->order;

        if (s["settings"].is<JsonObject>()) {
            screen->configure(s["settings"].as<JsonObjectConst>());
        }
    }

    sortByOrder();
    Serial.printf("[Screens] Loaded %d screens from file\n", _count);
    return _count > 0;
}

bool ScreenManager::save() {
    JsonDocument doc;
    doc["cycling"] = _cycling;

    JsonArray screens = doc["screens"].to<JsonArray>();
    for (uint8_t i = 0; i < _count; i++) {
        JsonObject s = screens.add<JsonObject>();
        s["id"] = _screens[i]->id;
        s["type"] = _screens[i]->typeName();
        s["enabled"] = _screens[i]->enabled;
        s["duration"] = _screens[i]->durationSec;
        s["order"] = _screens[i]->order;

        JsonObject settings = s["settings"].to<JsonObject>();
        _screens[i]->serialize(settings);
    }

    File f = LittleFS.open("/screens.json", "w");
    if (!f) {
        Serial.println("[Screens] Failed to open screens.json for writing");
        return false;
    }
    serializeJson(doc, f);
    f.close();
    return true;
}

void ScreenManager::serializeAll(JsonDocument& doc) const {
    doc["cycling"] = _cycling;
    if (_activeIndex >= 0) {
        doc["activeScreen"] = _screens[_activeIndex]->id;
    }
    JsonArray arr = doc["screens"].to<JsonArray>();
    for (uint8_t i = 0; i < _count; i++) {
        JsonObject s = arr.add<JsonObject>();
        s["id"] = _screens[i]->id;
        s["type"] = _screens[i]->typeName();
        s["enabled"] = _screens[i]->enabled;
        s["duration"] = _screens[i]->durationSec;
        s["order"] = _screens[i]->order;
        s["active"] = (i == _activeIndex);
    }
}

void ScreenManager::serializeScreen(const String& id, JsonDocument& doc) const {
    for (uint8_t i = 0; i < _count; i++) {
        if (_screens[i]->id == id) {
            doc["id"] = _screens[i]->id;
            doc["type"] = _screens[i]->typeName();
            doc["enabled"] = _screens[i]->enabled;
            doc["duration"] = _screens[i]->durationSec;
            doc["order"] = _screens[i]->order;
            doc["active"] = (i == _activeIndex);
            JsonObject settings = doc["settings"].to<JsonObject>();
            _screens[i]->serialize(settings);
            return;
        }
    }
}
