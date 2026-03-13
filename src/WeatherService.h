#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class WeatherService {
public:
    void begin();
    void update();  // non-blocking; call from loop()

    // Configure
    void setZipCode(const String& zip);
    String getZipCode() const;
    bool hasZipCode() const;

    // Data
    float getTemperatureF() const;
    float getHumidity() const;
    bool hasData() const;
    bool isOnline() const;
    bool hasGeocode() const;
    bool needsGeocode() const;
    float getLat() const;
    float getLon() const;
    String getLastError() const;

    // Diagnostics
    String getNetworkState() const;
    unsigned long getLastSuccessMs() const;
    unsigned long getLastFailureMs() const;
    unsigned long getConsecutiveFailures() const;
    unsigned long getBackoffMs() const;
    unsigned long getInFlightAgeMs() const;
    unsigned long getLastOperationLatencyMs() const;
    unsigned long getMaxLoopUpdateUs() const;
    unsigned long getLastLoopUpdateUs() const;
    bool isRequestInFlight() const;

private:
    enum class NetworkState : uint8_t {
        IDLE,
        WAITING_BACKOFF,
        WAITING_INTERVAL,
        GEOCODING,
        FETCHING
    };

    enum class JobType : uint8_t {
        NONE,
        GEOCODE,
        FETCH
    };

    struct JobResult {
        bool ok = false;
        JobType type = JobType::NONE;
        uint32_t opId = 0;
        float lat = 0;
        float lon = 0;
        float tempF = 0;
        float humidity = 0;
        String error;
        unsigned long latencyMs = 0;
    };

    void readSensorNonBlocking(unsigned long now);
    void scheduleJob(JobType type, unsigned long now);
    void applyJobResult(unsigned long now);
    void updateStateMachine(unsigned long now);

    bool doGeocode(String& errOut, float& latOut, float& lonOut);
    bool doFetch(String& errOut, float& tempOut, float& humidityOut);

    static void workerTaskEntry(void* arg);
    void workerTaskLoop();

    String _zipCode;
    float _lat = 0, _lon = 0;
    bool _needGeocode = false;
    bool _hasGeocode = false;

    float _tempF = 0;
    float _humidity = 0;
    bool _hasData = false;
    bool _online = false;

    bool _sensorAvailable = false;
    String _lastError;

    // Sensor read state (non-blocking two-stage SHT30 read)
    bool _sensorReadPending = false;
    unsigned long _sensorReadyAt = 0;
    unsigned long _lastSensorRead = 0;

    // Network state machine
    NetworkState _state = NetworkState::IDLE;
    unsigned long _lastFetch = 0;
    unsigned long _nextAttemptAt = 0;
    unsigned long _lastSuccessMs = 0;
    unsigned long _lastFailureMs = 0;
    unsigned long _consecutiveFailures = 0;
    unsigned long _backoffMs = 0;

    // Worker task communication
    SemaphoreHandle_t _mutex = nullptr;
    SemaphoreHandle_t _notifySem = nullptr;
    JobType _queuedJob = JobType::NONE;
    bool _jobInFlight = false;
    uint32_t _activeOpId = 0;
    uint32_t _nextOpId = 1;
    unsigned long _inFlightStartMs = 0;
    bool _resultReady = false;
    JobResult _result;

    // Diagnostics
    unsigned long _lastOperationLatencyMs = 0;
    unsigned long _lastLoopUpdateUs = 0;
    unsigned long _maxLoopUpdateUs = 0;

    static const unsigned long FETCH_INTERVAL = 300000;         // 5 min
    static const unsigned long SENSOR_INTERVAL = 10000;         // 10 sec
    static const unsigned long NET_OP_TIMEOUT = 5000;           // hard cap per op
    static const unsigned long NET_BASE_BACKOFF = 15000;        // 15s
    static const unsigned long NET_MAX_BACKOFF = 300000;        // 5 min
    static const unsigned long GEOCODE_RETRY_MIN = 30000;       // 30s
    static const unsigned long SENSOR_MEASURE_WAIT_MS = 20;     // SHT30 conversion time
};