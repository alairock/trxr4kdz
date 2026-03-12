# TRXR4KDZ — Custom Firmware for Ulanzi TC001

Custom ESP32 firmware for the **Ulanzi TC001** desktop LED matrix (32x8 WS2812B pixels). Features a modular screen system with auto-cycling, REST API control, OTA updates, and NTP time sync.

## Hardware

- **MCU**: ESP32-D0WD (240MHz dual-core, 320KB RAM, 4MB flash)
- **Display**: 32x8 WS2812B LED matrix (zigzag row layout)
- **Buzzer**: Piezo on GPIO 15 (strapping pin — whines during serial flash, hardware limitation)
- **Buttons**: Up (GPIO 26), Down (GPIO 14), Select (GPIO 27)
- **Sensors**: LDR (GPIO 35), Battery (GPIO 34)
- **I2C**: SCL (GPIO 22), SDA (GPIO 21)

## Building & Flashing

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Flash firmware via serial
pio run -t upload

# Flash filesystem (LittleFS) via serial
pio run -t uploadfs

# OTA firmware update (device must be on network)
curl -X POST http://<device-ip>/api/update -F "firmware=@.pio/build/ulanzi/firmware.bin"

# Serial monitor
pio device monitor
```

## Architecture

```
src/
  main.cpp                  Entry point, boot sequence, NTP setup
  DisplayManager.h/.cpp     LED matrix abstraction (drawing primitives)
  ConfigManager.h/.cpp      WiFi/device config persistence (LittleFS)
  WiFiManager.h/.cpp        AP + STA WiFi management
  WebServerManager.h/.cpp   HTTP server, REST API routes
  BuzzerManager.h/.cpp      Piezo buzzer control via LEDC
  screens/
    Screen.h                Base class + ScreenType enum
    ScreenManager.h/.cpp    Lifecycle, cycling, ordering, persistence
    ClockScreen.h/.cpp      Time display with configurable widgets
    TextTickerScreen.h/.cpp Horizontal scrolling text
    CanvasScreen.h/.cpp     Pixel framebuffer for external push
  widgets/
    Widget.h                Base widget class
    TimeWidget.h/.cpp       HH:MM with blinking colon, 12/24h
    DateWidget.h/.cpp       "Mar 11" or "3/11" format
    IconWidget.h/.cpp       8x8 bitmap icons
    ProgressBarWidget.h/.cpp  Horizontal bar (auto day-progress)
    DayIndicatorWidget.h/.cpp 7 dots for day of week
include/
  pins.h                    GPIO pin definitions
data/
  config.json               Device config (WiFi, brightness, hostname)
  screens.json              Screen definitions and settings
```

## Boot Sequence

1. **0ms** — GPIO 15 forced LOW (silence buzzer), serial init
2. **0ms** — Config loaded from LittleFS, display initialized, "BOOT" shown
3. **0ms** — WiFi started (AP always, STA if configured), NTP sync started
4. **0ms** — Web server started
5. **2000ms** — Buzzer beep, IP address displayed
6. **4000ms** — Screen manager initialized, screen cycling begins

## Screen System

### Screen Types

| Type | Description |
|------|-------------|
| `clock` | Time display with optional complications (day indicator, date, icon, progress bar) |
| `text_ticker` | Horizontal scrolling text with configurable speed, color, and optional icon |
| `canvas` | Raw pixel framebuffer — external apps push content via API |

### Configuration

Screens are persisted in `/screens.json` on LittleFS. Each screen has:

- **id** — unique identifier (e.g., `"clock-1"`)
- **type** — screen type string
- **enabled** — whether it participates in cycling
- **duration** — seconds to display before advancing (0 = no auto-advance)
- **order** — sort order for cycling
- **settings** — type-specific configuration object

### Clock Screen Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `use24h` | bool | `false` | 24-hour time format |
| `timezone` | string | `"MST7MDT,M3.2.0,M11.1.0"` | POSIX TZ string |
| `complications` | array | `["time", "day_indicator"]` | Active widgets |
| `colors.time` | string | `"#FFFFFF"` | Time text color (hex) |
| `colors.date` | string | `"#00AAFF"` | Date text color (hex) |
| `colors.icon` | string | `"#FFFFFF"` | Icon color (hex) |

Available complications: `time`, `date`, `day_indicator`, `progress_bar`, `icon`

Note: `date` and `time` will overlap on the 32px display — use one or the other.

### Text Ticker Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `text` | string | `"Hello World!"` | Text to scroll |
| `color` | string | `"#FFFFFF"` | Text color (hex) |
| `speed` | float | `30` | Scroll speed in pixels/second |
| `iconId` | int | `0` | Icon ID (0=clock, 1=text, 2=smile) |
| `hasIcon` | bool | `false` | Show pinned icon on left |

### Canvas Screen Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `lifetime` | int | `60` | Auto-disable after N seconds with no push (0 = indefinite) |

Canvas screens accept external content via `/api/canvas/{id}` and `/api/canvas/{id}/draw`.

---

## REST API

Base URL: `http://<device-ip>`

### Device

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/status` | Device info (version, IPs, RSSI, heap, uptime) |
| GET | `/api/config` | Get device config |
| POST | `/api/config` | Update device config (brightness, hostname, ap_password) |
| POST | `/api/wifi` | Set WiFi credentials and connect |
| POST | `/api/update` | OTA firmware upload (multipart form, field: `firmware`) |

### Screens

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/screens` | List all screens with summary info |
| GET | `/api/screens/{id}` | Get screen with full settings |
| POST | `/api/screens` | Create new screen (`type` required) |
| PUT | `/api/screens/{id}` | Update screen settings |
| DELETE | `/api/screens/{id}` | Remove a screen |
| POST | `/api/screens/{id}/enable` | Enable screen |
| POST | `/api/screens/{id}/disable` | Disable screen (remove from cycle) |
| POST | `/api/screens/reorder` | Reorder screens (`{"order":["id1","id2"]}`) |
| POST | `/api/screens/next` | Force advance to next screen |
| PUT | `/api/screens/cycling` | Enable/disable auto-cycling (`{"enabled":true}`) |

### Canvas

| Method | Path | Description |
|--------|------|-------------|
| POST | `/api/canvas/{id}` | Push pixel data (`{"rgb":"<hex>"}`) |
| POST | `/api/canvas/{id}/draw` | Push draw commands (AWTRIX3-compatible) |

#### Draw Commands

```json
{
  "dp": [[x, y, "#RRGGBB"], ...],
  "dl": [[x0, y0, x1, y1, "#RRGGBB"], ...],
  "df": [[x, y, w, h, "#RRGGBB"], ...],
  "cl": true
}
```

- `dp` — draw pixels
- `dl` — draw lines
- `df` — fill rectangles
- `cl` — clear framebuffer

### API Examples

```bash
# Get device status
curl http://192.168.0.48/api/status

# List screens
curl http://192.168.0.48/api/screens

# Create a text ticker
curl -X POST http://192.168.0.48/api/screens \
  -H "Content-Type: application/json" \
  -d '{"type":"text_ticker"}'

# Update clock to 24h mode
curl -X PUT http://192.168.0.48/api/screens/clock-1 \
  -H "Content-Type: application/json" \
  -d '{"settings":{"use24h":true}}'

# Change timezone to Eastern
curl -X PUT http://192.168.0.48/api/screens/clock-1 \
  -H "Content-Type: application/json" \
  -d '{"settings":{"timezone":"EST5EDT,M3.2.0,M11.1.0"}}'

# Draw a red pixel on canvas
curl -X POST http://192.168.0.48/api/canvas/canvas-1/draw \
  -H "Content-Type: application/json" \
  -d '{"dp":[[16,4,"#FF0000"]]}'

# Advance to next screen
curl -X POST http://192.168.0.48/api/screens/next

# Disable a screen
curl -X POST http://192.168.0.48/api/screens/text_ticker-1/disable

# OTA update
curl -X POST http://192.168.0.48/api/update \
  -F "firmware=@.pio/build/ulanzi/firmware.bin"
```

### Common Timezone Strings

| Timezone | POSIX TZ String |
|----------|----------------|
| US Pacific | `PST8PDT,M3.2.0,M11.1.0` |
| US Mountain | `MST7MDT,M3.2.0,M11.1.0` |
| US Central | `CST6CDT,M3.2.0,M11.1.0` |
| US Eastern | `EST5EDT,M3.2.0,M11.1.0` |
| UTC | `UTC0` |
| Europe/London | `GMT0BST,M3.5.0/1,M10.5.0` |
| Europe/Berlin | `CET-1CEST,M3.5.0,M10.5.0/3` |
| Asia/Tokyo | `JST-9` |

---

## Roadmap

### Phase 2 — Additional Screens & Interaction
- **BinaryClockScreen** — Binary-encoded time display
- **CustomScreen** — User-defined widget placement (place any combination of widgets at arbitrary positions)
- **Button handling** — Physical buttons (Up/Down/Select) for manual screen cycling and menu navigation

### Phase 3 — Connected Data Screens
- **WeatherScreen** — Current conditions and forecast via OpenWeatherMap API
- **CryptoScreen** — Cryptocurrency price ticker via CoinGecko API
- **Async HTTP fetch utility** — Non-blocking HTTP client for data screens to pull external APIs without stalling the display loop

### Phase 4 — Polish & Ecosystem
- **Transition effects** — Slide, fade, and other animations between screen changes
- **LaMetric icon storage** — Download and cache LaMetric-compatible 8x8 icons on LittleFS for richer screen content
- **MQTT transport** — Subscribe to MQTT topics for real-time data push (Home Assistant integration, custom dashboards)

---

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| FastLED | ^3.7 | WS2812B LED driver |
| FastLED NeoMatrix | ^1.2 | Matrix layout mapping |
| Adafruit GFX | ^1.11 | Graphics primitives and font rendering |
| ArduinoJson | ^7.0 | JSON parsing/serialization |
| LittleFS | built-in | Flash filesystem for config persistence |
| WebServer | built-in | HTTP server |
| WiFi | built-in | WiFi connectivity |
| Update | built-in | OTA firmware updates |
