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
#include "AlarmManager.h"
#include "OvertakeManager.h"
#include "NotificationManager.h"

#define FIRMWARE_VERSION "0.1.0"

class WebServerManager {
public:
    enum class RemoteButtonAction : uint8_t {
        NONE,
        LEFT_SHORT,
        RIGHT_SHORT,
        MIDDLE_SHORT,
        MIDDLE_DOUBLE,
        MIDDLE_LONG
    };

    void begin(ConfigManager& config, WiFiManager& wifi, DisplayManager& display, ScreenManager& screens, WeatherService* weather = nullptr, AlarmManager* alarm = nullptr, OvertakeManager* overtake = nullptr, NotificationManager* notifications = nullptr);
    void update();
    RemoteButtonAction takeRemoteButtonAction();

private:
    void handleStatus();
    void handleGetConfig();
    void handlePostConfig();
    void handlePostWifi();
    void handleWifiScan();
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
    void handleCanvasFrame();
    void handlePreview();
    void handleLametricSearch();
    void handleMqttStatus();
    void handleGetAlarm();
    void handlePutAlarm();
    void handleAlarmPreview();
    void handleAlarmTrigger();
    void handleOvertakeMute();
    void handleOvertakeClear();
    void handleLametricIcon();
    void handleGetNotifications();
    void handlePutNotificationsConfig();
    void handlePutNotificationsState();
    void handleNotificationsPreview();
    void handleButtonAction();
    void handleCaptivePortal();
    void handleUI();
    void handleWifiSave();
    void scheduleRestart(unsigned long delayMs);

    bool isAuthorizedRequest();
    bool requireAuth();

    // Route matching helper
    bool matchRoute(const String& uri, const String& pattern, String& param);

    WebServer _server{80};
    DNSServer _dns;
    ConfigManager* _config = nullptr;
    WiFiManager* _wifi = nullptr;
    DisplayManager* _display = nullptr;
    ScreenManager* _screens = nullptr;
    WeatherService* _weather = nullptr;
    AlarmManager* _alarm = nullptr;
    OvertakeManager* _overtake = nullptr;
    NotificationManager* _notifications = nullptr;

    bool _restartPending = false;
    unsigned long _restartAt = 0;

    uint8_t _otaErrorCode = 0;
    String _otaErrorMessage;

    volatile RemoteButtonAction _pendingButtonAction = RemoteButtonAction::NONE;
};
