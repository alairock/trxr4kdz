#pragma once

#include <Arduino.h>

enum class ButtonEvent : uint8_t {
    NONE,
    SHORT_PRESS,
    LONG_PRESS,
    DOUBLE_PRESS
};

class ButtonManager {
public:
    void begin();
    void update();

    ButtonEvent getLeftEvent();
    ButtonEvent getRightEvent();
    ButtonEvent getMiddleEvent();

private:
    struct ButtonState {
        uint8_t pin;
        bool lastReading = true;        // INPUT_PULLUP: high = not pressed
        bool pressed = false;
        unsigned long lastDebounce = 0;
        unsigned long pressStart = 0;
        bool longFired = false;
        uint8_t clickCount = 0;
        unsigned long lastClick = 0;
        ButtonEvent pending = ButtonEvent::NONE;
    };

    void updateButton(ButtonState& btn);

    ButtonState _left;
    ButtonState _right;
    ButtonState _middle;

    static const unsigned long DEBOUNCE_MS = 30;
    static const unsigned long LONG_PRESS_MS = 600;
    static const unsigned long DOUBLE_CLICK_MS = 300;
};
