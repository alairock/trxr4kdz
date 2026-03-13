#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include "ConfigManager.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "screens/ScreenManager.h"
#include "WeatherService.h"

#define FIRMWARE_VERSION "0.1.0"

class WebServerManager {
public:
    void begin(ConfigManager& config, WiFiManager& wifi, DisplayManager& display, ScreenManager& screens, WeatherService* weather = nullptr);
    void update();

private:
    void handleStatus();
    void handleGetConfig();
    void handlePostConfig();
    void handlePostWifi();
    void handleUpdate();
    void handleUpdateUpload();

    // Screen API handlers
    void handleGetScreens();
    void handleGetScreen();
    void handlePostScreen();
    void handlePutScreen();
    void handleDeleteScreen();
    void handleScreenEnable();
    void handleScreenDisable();
    void handleScreensReorder();
    void handleScreensNext();
    void handleScreensCycling();
    void handleGetScreenDefaults();
    void handlePutScreenDefaults();
    void handleCanvasPush();
    void handleCanvasDraw();
    void handleCaptivePortal();
    void handleWifiSave();

    // Route matching helper
    bool matchRoute(const String& uri, const String& pattern, String& param);

    WebServer _server{80};
    DNSServer _dns;
    ConfigManager* _config = nullptr;
    WiFiManager* _wifi = nullptr;
    DisplayManager* _display = nullptr;
    ScreenManager* _screens = nullptr;
    WeatherService* _weather = nullptr;
};
