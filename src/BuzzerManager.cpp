#include "BuzzerManager.h"

void BuzzerManager::begin() {
    // Drive pin LOW immediately to silence buzzer ASAP.
    // GPIO 15 is a strapping pin that floats high during flash/reset,
    // causing the buzzer to whine. OUTPUT+LOW is the fastest software fix.
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    Serial.println("[Buzzer] GPIO driven LOW for boot");
}

void BuzzerManager::ready() {
    if (_ready) return;
    ledcSetup(LEDC_CHANNEL, 1000, 8);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    ledcWriteTone(LEDC_CHANNEL, 0);
    _ready = true;
    Serial.println("[Buzzer] Ready");
}

void BuzzerManager::beep(uint16_t freq, uint16_t durationMs, uint8_t volume) {
    if (!_ready) return;
    uint8_t duty = (uint8_t)((uint16_t)255 * volume / 100);
    ledcWriteTone(LEDC_CHANNEL, freq);
    ledcWrite(LEDC_CHANNEL, duty);
    _playing = true;
    _stopAt = millis() + durationMs;
}

void BuzzerManager::stop() {
    if (!_ready) return;
    ledcWriteTone(LEDC_CHANNEL, 0);
    ledcWrite(LEDC_CHANNEL, 0);
    _playing = false;
}

void BuzzerManager::update() {
    if (_playing && millis() >= _stopAt) {
        stop();
    }
}
