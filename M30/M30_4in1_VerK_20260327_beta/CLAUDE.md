# CLAUDE.md — M30 4Gas Firmware Project Context

This file gives Claude Code instant context when opening this project on any machine.
Read this before touching any source file.

---

## Project Identity

| Item | Value |
|------|-------|
| Product | M30 4Gas PID Analyzer |
| MCU | ESP32 (Adafruit Huzzah32) |
| FW Version | `VerG-20260514` (defined as `#define FW_VERSION` in the `.ino`) |
| IDE | Arduino IDE 2.x |
| Display | 128×64 SSD1306 OLED via ThingPulse `SSD1306Wire` |
| ADC | ADS1115 (4-channel) |
| RTC | PCF8563 via RTClib |
| Power monitor | INA219 |
| Sensor input | UART serial from sensor board |
| Persistence | Arduino EEPROM library |

Display constructor (already set in `.ino`):
```cpp
SSD1306Wire display(0x3c, 23, 22);
```

---

## Repo Layout

```
M30_4in1_VerK_20260327_beta/
├── M30_4in1_VerK_20260327_beta.ino   # Main sketch — entry point, button handlers, menu wiring
├── MenuRenderer.cpp                   # All OLED renderer implementations
├── ButtonPressDetector.cpp
├── ConfigurationManager.cpp
├── DataLogger.cpp
├── Globals.cpp
├── TimeSync.cpp
├── inc/
│   ├── GasManager.h      # Gas selection, calibration (zero + span), EEPROM cal coefficients
│   ├── Menu.h            # Menu class hierarchy + CompositeMenu
│   ├── MenuRenderer.h    # Renderer class declarations
│   ├── AlarmSet.h        # High alarm thresholds per gas
│   ├── LowAlarmSet.h     # Low alarm thresholds per gas
│   ├── RangeSet.h        # Measurement range per gas
│   ├── CalvalueSet.h     # Span cal target concentration options
│   ├── ZerovalueSet.h    # Zero cal target value options
│   ├── HourSet.h         # RTC hour setting (writes to PCF8563 + EEPROM 220)
│   ├── MinuteSet.h       # RTC minute setting (writes to PCF8563 + EEPROM 230)
│   ├── OutputSet.h       # Analog output type (4-20mA / 0-10V)
│   ├── LoggingSet.h      # Data logging interval setting
│   ├── SleepTimer.h      # Auto-sleep timer
│   ├── DataSource.h      # Abstraction over ADS1115 readings → calibrated values
│   ├── DataLogger.h      # Flash data logging
│   ├── ConfigurationManager.h
│   ├── Globals.h         # Shared constants, param name strings
│   ├── TimeSync.h
│   ├── Button.h
│   ├── pins.h            # Hardware pin assignments
│   ├── image.h           # Logo XBM bitmap
│   └── CalManager.h
├── README.md             # Full design document (screens, EEPROM map, change log)
└── CLAUDE.md             # This file
```

---

## Gases and Channel Mapping

| Gas | Channel | EEPROM Intercept addr | EEPROM Slope addr |
|-----|---------|-----------------------|-------------------|
| CO  | 0       | 8                     | 16                |
| H2S | 1       | 24                    | 32                |
| O2  | 2       | 40                    | 48                |
| CH4 | 3       | 56                    | 64                |

Calibration formula:
- **Zero cal** — `newIntercept = rawReading - (targetValue × currentSlope)`
- **Span cal** — `newSlope = (rawReading - currentIntercept) / targetValue`

---

## EEPROM Address Map (do not overlap these)

| Address | Size | Content |
|---------|------|---------|
| 4 | 4 B | Selected gas index |
| 8–15 | 8 B | Intercept CO |
| 16–23 | 8 B | Slope CO |
| 24–31 | 8 B | Intercept H2S |
| 32–39 | 8 B | Slope H2S |
| 40–47 | 8 B | Intercept O2 |
| 48–55 | 8 B | Slope O2 |
| 56–63 | 8 B | Intercept CH4 |
| 64–71 | 8 B | Slope CH4 |
| 72–87 | 16 B | Range settings |
| **88–119** | **32 B** | **Cal timestamp string `"YYYY-MM-DD HH:MM"`** |
| 120–135 | 16 B | Low Alarm settings |
| 220–223 | 4 B | RTC Hour |
| 230–233 | 4 B | RTC Minute |

`EEPROM_CAL_TIMESTAMP_OFFSET` is defined as `88` in `inc/GasManager.h`.

---

## Horizontal Menu Index

| Idx | Screen | Key behaviour |
|-----|--------|---------------|
| 0 | Run / Main | Live readings, battery, alarm bar |
| 1 | Library (Gas Select) | LEFT cycles gases; S selects active gas |
| 2 | Zero Cal | S zeros selected gas; banner ZERO SET / CAL FAILED |
| 3 | Cal Gas Select | S picks gas for span cal |
| 4 | Span Cal | S spans all 4 gases; banner CAL COMPLETE / CAL FAILED |
| 5 | Cal Result | Read-only: slopes + last cal timestamp |
| 6 | Range | LEFT cycles range; S sets for selected gas |
| 7 | Alarm (High) | LEFT cycles levels; S sets high alarm |
| 8 | Low Alarm | LEFT cycles levels; S sets low alarm |
| 9 | Cal Value | Sets span cal target concentration |
| 10 | Set Hour | S writes hour to PCF8563 + EEPROM 220 |
| 11 | Set Minute | S writes minute to PCF8563 + EEPROM 230; banner TIME SAVED |
| 12 | Output Port | 4-20mA / 0-10V selection |
| 13 | Cal Info | Read-only: all 4 slopes, cal timestamp, FW version |

Mode/RIGHT button wraps: 0 → 1 → … → 13 → 0.
Index 1 (Library) is also reachable via a dedicated button shortcut.

---

## Calibration Guard (±50% rule)

Both `calibrate()` and `calibrate2()` in `GasManager` reject cals where:

```
ratio = currentConc / targetValue
ratio < 0.5  OR  ratio > 1.5  →  FAIL (no EEPROM write, m_lastCalFailed = true)
```

- Zero cal guard is **skipped** when `targetValue == 0`.
- Span cal is a batch of 4 gases. `resetCalFlag()` is called once before the batch in `CalMenuItemCalGas::action()`. Any single gas failure keeps `m_lastCalFailed = true` through the whole batch.
- The S-button handler reads `wasLastCalFailed()` after the action and shows the appropriate banner.

---

## S-Button Banner Pattern

```cpp
static void showSavedBanner(const char* msg) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 24, msg);
    display.display();
    delay(700);
    display.clear();
    display.display();
    display.setFont(ArialMT_Plain_10);
}
```

Banners by screen index:
- idx 2 (Zero Cal): `"ZERO SET"` or `"CAL FAILED"`
- idx 4 (Span Cal): `"CAL COMPLETE"` or `"CAL FAILED"`
- idx 10 (Set Hour): `"TIME SAVED"`
- idx 11 (Set Minute): `"TIME SAVED"`

---

## Footer Pattern (all settings screens)

Every settings screen footer uses:
```cpp
m_display->drawLine(0, 49, 128, 49);
m_display->setTextAlignment(TEXT_ALIGN_LEFT);
m_display->drawString(0, 51, "OPTION");
m_display->setTextAlignment(TEXT_ALIGN_CENTER);
m_display->drawString(64, 51, "SET");
m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
m_display->drawString(128, 51, "MENU");
m_display->setTextAlignment(TEXT_ALIGN_LEFT);
```

---

## Startup Sequence

```
setup() {
  logo bitmap  →  delay(2000)
  "M30 4Gas" + "Ver. VerG-20260514"  →  delay(1500)
  load EEPROM
  sync RTC from PCF8563
  enter loop()
}
```

---

## What Has Been Removed

WiFi and MQTT are **fully removed**. Do not re-add:
- `WiFi.h`, `WebServer.h`, any MQTT library includes
- `setupWiFi()`, `WebServer server`, MQTT listener callbacks
- `WiFiDumpMenuItem`, `WiFiRealTimeDumpMenuItem`, `NTPSyncMenuItem`
- `SSD1306WiFiDumpMenuRenderer`, `SSD1306WiFiRealTimeDumpMenuRenderer`, `SSD1306NTPSyncMenuRenderer`

---

## Known Stale Files (do not delete, but treat as inactive)

- `inc/MQTTFlashPublisher.h` — MQTT remnant, no longer instantiated
- `inc/MQTTRealTimePublisher.h` — same
- `inc/WebServer.h` — legacy web server header, not included in `.ino`
- `inc/test.h` / `inc/test.cpp` — scratch test files, not compiled into sketch

---

## Build & Upload

1. Open `M30_4in1_VerK_20260327_beta.ino` in Arduino IDE 2.x
2. Board: **Adafruit ESP32 Feather** (or generic ESP32 Dev Module)
3. Upload speed: 921600
4. Required libraries (install via Library Manager):
   - ESP8266 and ESP32 OLED Driver for SSD1306 (ThingPulse)
   - RTClib (Adafruit)
   - Adafruit ADS1X15
   - Adafruit INA219
   - EEPROM (built-in ESP32 core)
