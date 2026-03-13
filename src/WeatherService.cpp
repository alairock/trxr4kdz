#include "WeatherService.h"

#include <Wire.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "pins.h"

#define SHT30_ADDR 0x44

void WeatherService::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    Wire.beginTransmission(SHT30_ADDR);
    _sensorAvailable = (Wire.endTransmission() == 0);
    Serial.printf("[Weather] SHT30 sensor %s\n", _sensorAvailable ? "found" : "not found");

    _mutex = xSemaphoreCreateMutex();
    _notifySem = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(workerTaskEntry, "weather-net", 8192, this, 1, nullptr, 1);
}

void WeatherService::setZipCode(const String& zip) {
    if (_zipCode == zip) return;
    _zipCode = zip;
    _needGeocode = _zipCode.length() > 0;
    if (_needGeocode) {
        _hasGeocode = false;
        _lat = 0;
        _lon = 0;
        _nextAttemptAt = 0;
        _consecutiveFailures = 0;
        _backoffMs = 0;
    }
}

String WeatherService::getZipCode() const { return _zipCode; }
bool WeatherService::hasZipCode() const { return _zipCode.length() > 0; }
float WeatherService::getTemperatureF() const { return _tempF; }
float WeatherService::getHumidity() const { return _humidity; }
bool WeatherService::hasData() const { return _hasData; }
bool WeatherService::isOnline() const { return _online; }
bool WeatherService::hasGeocode() const { return _hasGeocode; }
bool WeatherService::needsGeocode() const { return _needGeocode; }
float WeatherService::getLat() const { return _lat; }
float WeatherService::getLon() const { return _lon; }
String WeatherService::getLastError() const { return _lastError; }
unsigned long WeatherService::getLastSuccessMs() const { return _lastSuccessMs; }
unsigned long WeatherService::getLastFailureMs() const { return _lastFailureMs; }
unsigned long WeatherService::getConsecutiveFailures() const { return _consecutiveFailures; }
unsigned long WeatherService::getBackoffMs() const { return _backoffMs; }
unsigned long WeatherService::getLastOperationLatencyMs() const { return _lastOperationLatencyMs; }
unsigned long WeatherService::getMaxLoopUpdateUs() const { return _maxLoopUpdateUs; }
unsigned long WeatherService::getLastLoopUpdateUs() const { return _lastLoopUpdateUs; }
bool WeatherService::isRequestInFlight() const { return _jobInFlight; }

unsigned long WeatherService::getInFlightAgeMs() const {
    if (!_jobInFlight) return 0;
    return millis() - _inFlightStartMs;
}

String WeatherService::getNetworkState() const {
    switch (_state) {
        case NetworkState::IDLE: return "idle";
        case NetworkState::WAITING_BACKOFF: return "backoff";
        case NetworkState::WAITING_INTERVAL: return "waiting_interval";
        case NetworkState::GEOCODING: return "geocoding";
        case NetworkState::FETCHING: return "fetching";
    }
    return "unknown";
}

void WeatherService::update() {
    unsigned long t0 = micros();
    unsigned long now = millis();

    applyJobResult(now);
    updateStateMachine(now);

    if (!_online && _sensorAvailable) {
        readSensorNonBlocking(now);
    }

    _lastLoopUpdateUs = micros() - t0;
    if (_lastLoopUpdateUs > _maxLoopUpdateUs) {
        _maxLoopUpdateUs = _lastLoopUpdateUs;
    }
}

void WeatherService::readSensorNonBlocking(unsigned long now) {
    if (!_sensorReadPending) {
        if (_lastSensorRead != 0 && (now - _lastSensorRead < SENSOR_INTERVAL)) {
            return;
        }

        Wire.beginTransmission(SHT30_ADDR);
        Wire.write(0x2C);
        Wire.write(0x06);
        if (Wire.endTransmission() != 0) {
            return;
        }

        _sensorReadPending = true;
        _sensorReadyAt = now + SENSOR_MEASURE_WAIT_MS;
        return;
    }

    if ((long)(now - _sensorReadyAt) < 0) {
        return;
    }

    Wire.requestFrom((uint8_t)SHT30_ADDR, (uint8_t)6);
    if (Wire.available() < 6) {
        _sensorReadPending = false;
        _lastSensorRead = now;
        return;
    }

    uint8_t data[6];
    for (int i = 0; i < 6; i++) data[i] = Wire.read();

    float tempC = ((((data[0] * 256) + data[1]) * 175.0f) / 65535.0f) - 45.0f;
    _tempF = tempC * 9.0f / 5.0f + 32.0f;
    _humidity = ((data[3] * 256) + data[4]) * 100.0f / 65535.0f;

    _hasData = true;
    _online = false;
    _sensorReadPending = false;
    _lastSensorRead = now;
}

void WeatherService::scheduleJob(JobType type, unsigned long now) {
    if (_jobInFlight || !_notifySem) return;

    xSemaphoreTake(_mutex, portMAX_DELAY);
    _queuedJob = type;
    _activeOpId = _nextOpId++;
    _jobInFlight = true;
    _inFlightStartMs = now;
    xSemaphoreGive(_mutex);

    _state = (type == JobType::GEOCODE) ? NetworkState::GEOCODING : NetworkState::FETCHING;
    xSemaphoreGive(_notifySem);
}

void WeatherService::applyJobResult(unsigned long now) {
    bool hasResult = false;
    JobResult local;

    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_resultReady) {
        hasResult = true;
        local = _result;
        _resultReady = false;

        if (_jobInFlight && local.opId == _activeOpId) {
            _jobInFlight = false;
        }
    }
    xSemaphoreGive(_mutex);

    if (!hasResult) {
        if (_jobInFlight && (now - _inFlightStartMs > NET_OP_TIMEOUT)) {
            _lastError = "timeout";
            _lastFailureMs = now;
            _consecutiveFailures++;
            _backoffMs = min(NET_MAX_BACKOFF, (unsigned long)(NET_BASE_BACKOFF << min((unsigned long)4, _consecutiveFailures - 1)));
            _nextAttemptAt = now + _backoffMs;
            _state = NetworkState::WAITING_BACKOFF;
            _jobInFlight = false;  // stale worker result will be ignored via opId mismatch
            _activeOpId = 0;
        }
        return;
    }

    _lastOperationLatencyMs = local.latencyMs;

    if (!local.ok) {
        _online = false;
        _lastError = local.error;
        _lastFailureMs = now;
        _consecutiveFailures++;
        _backoffMs = min(NET_MAX_BACKOFF, (unsigned long)(NET_BASE_BACKOFF << min((unsigned long)4, _consecutiveFailures - 1)));
        _nextAttemptAt = now + _backoffMs;
        _state = NetworkState::WAITING_BACKOFF;
        return;
    }

    _lastError = "";
    _lastSuccessMs = now;
    _consecutiveFailures = 0;
    _backoffMs = 0;

    if (local.type == JobType::GEOCODE) {
        _lat = local.lat;
        _lon = local.lon;
        _needGeocode = false;
        _hasGeocode = true;
        _lastFetch = 0;
        _nextAttemptAt = now;
        _state = NetworkState::IDLE;
        return;
    }

    if (local.type == JobType::FETCH) {
        _tempF = local.tempF;
        _humidity = local.humidity;
        _hasData = true;
        _online = true;
        _lastFetch = now;
        _nextAttemptAt = now + FETCH_INTERVAL;
        _state = NetworkState::IDLE;
        return;
    }
}

void WeatherService::updateStateMachine(unsigned long now) {
    if (_jobInFlight) return;

    if (_nextAttemptAt != 0 && now < _nextAttemptAt) {
        _state = (_consecutiveFailures > 0 || _backoffMs > 0)
            ? NetworkState::WAITING_BACKOFF
            : NetworkState::WAITING_INTERVAL;
        return;
    }

    if (_needGeocode && _zipCode.length() > 0) {
        scheduleJob(JobType::GEOCODE, now);
        return;
    }

    if (_hasGeocode && (_lastFetch == 0 || now - _lastFetch >= FETCH_INTERVAL)) {
        scheduleJob(JobType::FETCH, now);
        return;
    }

    _state = NetworkState::IDLE;
}

bool WeatherService::doGeocode(String& errOut, float& latOut, float& lonOut) {
    HTTPClient http;
    String url = "http://api.zippopotam.us/us/" + _zipCode;
    http.begin(url);
    http.setTimeout(NET_OP_TIMEOUT);

    int code = http.GET();
    if (code != 200) {
        errOut = "geocode HTTP " + String(code);
        http.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        errOut = "geocode parse " + String(err.c_str());
        return false;
    }

    JsonArray places = doc["places"].as<JsonArray>();
    if (!places || places.size() == 0) {
        errOut = "geocode no places";
        return false;
    }

    const char* latStr = places[0]["latitude"];
    const char* lonStr = places[0]["longitude"];
    if (!latStr || !lonStr) {
        errOut = "geocode missing lat/lon";
        return false;
    }

    latOut = atof(latStr);
    lonOut = atof(lonStr);
    return true;
}

bool WeatherService::doFetch(String& errOut, float& tempOut, float& humidityOut) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" +
                 String(_lat, 4) + "&longitude=" + String(_lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m&temperature_unit=fahrenheit";

    http.begin(client, url);
    http.setTimeout(NET_OP_TIMEOUT);

    int code = http.GET();
    if (code != 200) {
        errOut = "fetch HTTP " + String(code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        errOut = "fetch parse " + String(err.c_str());
        return false;
    }

    JsonObject current = doc["current"];
    if (!current) {
        errOut = "fetch no current";
        return false;
    }

    tempOut = current["temperature_2m"] | 0.0f;
    humidityOut = current["relative_humidity_2m"] | 0.0f;
    return true;
}

void WeatherService::workerTaskEntry(void* arg) {
    static_cast<WeatherService*>(arg)->workerTaskLoop();
}

void WeatherService::workerTaskLoop() {
    while (true) {
        xSemaphoreTake(_notifySem, portMAX_DELAY);

        JobType job = JobType::NONE;
        uint32_t opId = 0;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        job = _queuedJob;
        opId = _activeOpId;
        _queuedJob = JobType::NONE;
        xSemaphoreGive(_mutex);

        if (job == JobType::NONE) continue;

        unsigned long t0 = millis();
        JobResult out;
        out.type = job;
        out.opId = opId;

        if (job == JobType::GEOCODE) {
            out.ok = doGeocode(out.error, out.lat, out.lon);
        } else if (job == JobType::FETCH) {
            out.ok = doFetch(out.error, out.tempF, out.humidity);
        }

        out.latencyMs = millis() - t0;

        xSemaphoreTake(_mutex, portMAX_DELAY);
        if (opId == _activeOpId) {
            _result = out;
            _resultReady = true;
        }
        xSemaphoreGive(_mutex);
    }
}
