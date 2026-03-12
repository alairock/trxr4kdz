#include <Arduino.h>
#include <time.h>
#include "pins.h"
#include "ConfigManager.h"
#include "BuzzerManager.h"
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "screens/ScreenManager.h"

ConfigManager configManager;
BuzzerManager buzzerManager;
DisplayManager displayManager;
WiFiManager wifiManager;
WebServerManager webServerManager;
ScreenManager screenManager;

bool buzzerInitDone = false;
bool screensStarted = false;
unsigned long bootTime;

void setup() {
    // Silence buzzer ASAP — GPIO 15 floats high during flash/reset
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.begin(115200);
    Serial.println("\n=== Ulanzi TC001 Boot ===");

    bootTime = millis();

    buzzerManager.begin();

    // Load config from LittleFS
    configManager.begin();

    // Init display and show boot message
    displayManager.begin(configManager.getBrightness());
    displayManager.showText("BOOT", displayManager.getMatrix()->Color(0, 100, 255));

    // Start WiFi (AP always, STA if configured)
    wifiManager.begin(configManager);

    // Configure NTP time sync with Pacific timezone default
    setenv("TZ", "MST7MDT,M3.2.0,M11.1.0", 1);
    tzset();
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Start HTTP server (with screen manager)
    webServerManager.begin(configManager, wifiManager, displayManager, screenManager);

    Serial.println("=== Boot complete ===");
}

void loop() {
    // After 2s, enable buzzer, beep, show IP, then start screens
    if (!buzzerInitDone && millis() - bootTime >= 2000) {
        buzzerManager.ready();
        buzzerManager.beep(1000, 150);
        buzzerInitDone = true;

        // Show IP address briefly
        if (wifiManager.isSTAConnected()) {
            displayManager.showText(wifiManager.getSTAIP(),
                displayManager.getMatrix()->Color(0, 255, 0));
        } else {
            displayManager.showText(wifiManager.getAPIP(),
                displayManager.getMatrix()->Color(255, 200, 0));
        }
    }

    // After 4s, initialize and start screen cycling
    if (!screensStarted && millis() - bootTime >= 4000) {
        screenManager.begin(displayManager);
        screensStarted = true;
    }

    buzzerManager.update();
    wifiManager.update();
    webServerManager.update();

    if (screensStarted) {
        screenManager.update();
    }
    displayManager.update();
}
