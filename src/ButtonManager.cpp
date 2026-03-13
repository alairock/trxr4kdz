#include "ButtonManager.h"
#include "pins.h"

void ButtonManager::begin() {
    _left.pin = PIN_BTN_UP;
    _right.pin = PIN_BTN_DOWN;
    _middle.pin = PIN_BTN_SELECT;

    pinMode(_left.pin, INPUT_PULLUP);
    pinMode(_right.pin, INPUT_PULLUP);
    pinMode(_middle.pin, INPUT_PULLUP);
}

void ButtonManager::update() {
    updateButton(_left);
    updateButton(_right);
    updateButton(_middle);
}

void ButtonManager::updateButton(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);
    unsigned long now = millis();

    // Debounce
    if (reading != btn.lastReading) {
        btn.lastDebounce = now;
    }
    btn.lastReading = reading;

    if (now - btn.lastDebounce < DEBOUNCE_MS) return;

    bool isPressed = !reading; // active low

    // Press started
    if (isPressed && !btn.pressed) {
        btn.pressed = true;
        btn.pressStart = now;
        btn.longFired = false;
    }

    // Long press detection (fire while still held)
    if (isPressed && btn.pressed && !btn.longFired) {
        if (now - btn.pressStart >= LONG_PRESS_MS) {
            btn.pending = ButtonEvent::LONG_PRESS;
            btn.longFired = true;
            btn.clickCount = 0; // cancel any double-click tracking
        }
    }

    // Release
    if (!isPressed && btn.pressed) {
        btn.pressed = false;
        if (!btn.longFired) {
            btn.clickCount++;
            btn.lastClick = now;
        }
    }

    // Resolve click count after double-click window
    if (btn.clickCount > 0 && !btn.pressed && now - btn.lastClick >= DOUBLE_CLICK_MS) {
        if (btn.clickCount >= 2) {
            btn.pending = ButtonEvent::DOUBLE_PRESS;
        } else {
            btn.pending = ButtonEvent::SHORT_PRESS;
        }
        btn.clickCount = 0;
    }
}

ButtonEvent ButtonManager::getLeftEvent() {
    ButtonEvent e = _left.pending;
    _left.pending = ButtonEvent::NONE;
    return e;
}

ButtonEvent ButtonManager::getRightEvent() {
    ButtonEvent e = _right.pending;
    _right.pending = ButtonEvent::NONE;
    return e;
}

ButtonEvent ButtonManager::getMiddleEvent() {
    ButtonEvent e = _middle.pending;
    _middle.pending = ButtonEvent::NONE;
    return e;
}
