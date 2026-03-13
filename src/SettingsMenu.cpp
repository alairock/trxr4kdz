#include "SettingsMenu.h"
#include "DisplayManager.h"
#include "screens/ScreenManager.h"
#include "screens/ClockScreen.h"

const char* SettingsMenu::SETTING_NAMES[] = { "Font", "Calendar", "Day Dots" };

void SettingsMenu::begin(DisplayManager& display, ScreenManager& screens) {
    _display = &display;
    _screens = &screens;
}

void SettingsMenu::snapshotState() {
    Screen* s = _screens->getActiveScreen();
    if (s && s->type() == ScreenType::CLOCK) {
        ClockScreen* cs = static_cast<ClockScreen*>(s);
        _fontIndex = cs->getFontId();
        _savedFont = _fontIndex;
        _savedCalendar = cs->getShowCalendar();
        _savedDayIndicator = cs->getShowDayIndicator();
    }
}

void SettingsMenu::applyToggle(uint8_t settingIndex, bool value) {
    Screen* s = _screens->getActiveScreen();
    if (!s || s->type() != ScreenType::CLOCK) return;
    ClockScreen* cs = static_cast<ClockScreen*>(s);
    switch (settingIndex) {
        case 1: cs->setShowCalendar(value); break;
        case 2: cs->setShowDayIndicator(value); break;
    }
}

bool SettingsMenu::getToggle(uint8_t settingIndex) {
    Screen* s = _screens->getActiveScreen();
    if (!s || s->type() != ScreenType::CLOCK) return false;
    ClockScreen* cs = static_cast<ClockScreen*>(s);
    switch (settingIndex) {
        case 1: return cs->getShowCalendar();
        case 2: return cs->getShowDayIndicator();
        default: return false;
    }
}

void SettingsMenu::enter() {
    _active = true;
    _menuIndex = 0;
    _inSubmenu = false;
    snapshotState();
    draw();
}

void SettingsMenu::exit() {
    _active = false;
    _inSubmenu = false;
    _screens->save();
}

void SettingsMenu::update(ButtonEvent left, ButtonEvent right, ButtonEvent middle) {
    if (!_active) return;

    bool needRedraw = false;

    if (_inSubmenu) {
        if (_menuIndex == 0) {
            // Font selector
            if (left == ButtonEvent::SHORT_PRESS) {
                _fontIndex = (_fontIndex == 0) ? 3 : _fontIndex - 1;
                needRedraw = true;
            }
            if (right == ButtonEvent::SHORT_PRESS) {
                _fontIndex = (_fontIndex + 1) % 4;
                needRedraw = true;
            }
            if (middle == ButtonEvent::SHORT_PRESS) {
                // Confirm font
                Screen* s = _screens->getActiveScreen();
                if (s && s->type() == ScreenType::CLOCK) {
                    static_cast<ClockScreen*>(s)->setFontId(_fontIndex);
                }
                _savedFont = _fontIndex;
                _screens->save();
                _inSubmenu = false;
                needRedraw = true;
            }
            if (middle == ButtonEvent::DOUBLE_PRESS) {
                // Cancel font
                Screen* s = _screens->getActiveScreen();
                if (s && s->type() == ScreenType::CLOCK) {
                    static_cast<ClockScreen*>(s)->setFontId(_savedFont);
                }
                _fontIndex = _savedFont;
                _inSubmenu = false;
                needRedraw = true;
            }
        } else {
            // Toggle setting (Calendar or Day Dots)
            if (left == ButtonEvent::SHORT_PRESS || right == ButtonEvent::SHORT_PRESS) {
                bool cur = getToggle(_menuIndex);
                applyToggle(_menuIndex, !cur);
                needRedraw = true;
            }
            if (middle == ButtonEvent::SHORT_PRESS) {
                // Save and go back to settings list
                _screens->save();
                _inSubmenu = false;
                needRedraw = true;
            }
            if (middle == ButtonEvent::DOUBLE_PRESS) {
                // Cancel — revert and go back
                if (_menuIndex == 1) applyToggle(1, _savedCalendar);
                if (_menuIndex == 2) applyToggle(2, _savedDayIndicator);
                _inSubmenu = false;
                needRedraw = true;
            }
        }
        if (middle == ButtonEvent::LONG_PRESS) {
            // Exit settings entirely, revert unsaved
            Screen* s = _screens->getActiveScreen();
            if (s && s->type() == ScreenType::CLOCK) {
                static_cast<ClockScreen*>(s)->setFontId(_savedFont);
            }
            applyToggle(1, _savedCalendar);
            applyToggle(2, _savedDayIndicator);
            exit();
            return;
        }
    } else {
        // Settings list navigation
        if (left == ButtonEvent::SHORT_PRESS) {
            _menuIndex = (_menuIndex == 0) ? NUM_SETTINGS - 1 : _menuIndex - 1;
            needRedraw = true;
        }
        if (right == ButtonEvent::SHORT_PRESS) {
            _menuIndex = (_menuIndex + 1) % NUM_SETTINGS;
            needRedraw = true;
        }
        if (middle == ButtonEvent::SHORT_PRESS) {
            _inSubmenu = true;
            snapshotState(); // snapshot for cancel
            needRedraw = true;
        }
        if (middle == ButtonEvent::DOUBLE_PRESS || middle == ButtonEvent::LONG_PRESS) {
            exit();
            return;
        }
    }

    if (needRedraw) draw();
}

void SettingsMenu::draw() {
    if (!_display) return;

    if (_inSubmenu) {
        if (_menuIndex == 0) {
            drawFontSelector();
        } else {
            drawToggleSetting(SETTING_NAMES[_menuIndex], getToggle(_menuIndex));
        }
    } else {
        drawSettingsList();
    }
    _display->show();
}

void SettingsMenu::drawDots(uint8_t count, uint8_t activeIndex) {
    int16_t totalW = count * 2 + (count - 1); // 2px per dot + 1px gap
    int16_t dotX = (MATRIX_WIDTH - totalW) / 2;
    for (uint8_t i = 0; i < count; i++) {
        uint16_t color = (i == activeIndex) ? 0x07E0 : 0x4208; // green vs dim gray
        _display->drawPixel(dotX + i * 3, 7, color);
        _display->drawPixel(dotX + i * 3 + 1, 7, color);
    }
}

void SettingsMenu::drawSettingsList() {
    _display->clear();

    const char* name = SETTING_NAMES[_menuIndex];
    uint8_t charW = DisplayManager::fontCharWidth(1);
    int16_t textW = strlen(name) * charW;
    int16_t textX = (MATRIX_WIDTH - textW) / 2;
    _display->drawFontText(textX, 0, name, 0xFFFF, 1);

    drawDots(NUM_SETTINGS, _menuIndex);
}

void SettingsMenu::drawFontSelector() {
    _display->clear();

    char buf[12];
    snprintf(buf, sizeof(buf), "12:34 PM");
    uint8_t charW = DisplayManager::fontCharWidth(_fontIndex);
    int16_t narrow = 0;
    for (int i = 0; buf[i]; i++) {
        if (buf[i] == ':' || buf[i] == ' ') narrow++;
    }
    int16_t textW = (strlen(buf) - narrow) * charW + narrow * (charW / 2);
    int16_t textX = (MATRIX_WIDTH - textW) / 2;
    _display->drawFontText(textX, 0, buf, 0xFFFF, _fontIndex);

    drawDots(4, _fontIndex);
}

void SettingsMenu::drawToggleSetting(const char* label, bool value) {
    _display->clear();
    _display->show();
    _display->clear();

    // Show ON or OFF centered vertically and horizontally
    const char* state = value ? "ON" : "OFF";
    uint16_t stateColor = value ? 0x07E0 : 0xF800;
    uint8_t charW = DisplayManager::fontCharWidth(1);
    int16_t stateW = strlen(state) * charW;
    int16_t stateX = (MATRIX_WIDTH - stateW) / 2;
    int16_t stateY = (8 - DisplayManager::fontHeight(1)) / 2;
    _display->drawFontText(stateX, stateY, state, stateColor, 1);
}
