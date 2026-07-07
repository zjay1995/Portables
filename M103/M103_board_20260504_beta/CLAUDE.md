# M103 PID Analyzer — Claude Code Context

## Project Type
Arduino firmware for ESP32 (Arduino IDE 1.8.x).  
**Do NOT use `cmake`, `make`, or `idf.py`** — compile via Arduino IDE only.

## Board Settings (Arduino IDE)
- Board: ESP32 Dev Module
- Flash: 4MB — No OTA (2MB APP / 2MB SPIFFS)
- CPU: 240 MHz (WiFi/BT)
- Upload Speed: 921600
- Partition Scheme: No OTA (2MB APP/2MB SPIFFS)

## Key File Map

| File | Role |
|---|---|
| `M103_board_20260504_beta.ino` | Main sketch — setup(), loop(), menu build, button handlers |
| `MenuRenderer.cpp` | All TFT screen renderers |
| `TimeSync.cpp` | RTC read + EEPROM time backup |
| `ConfigurationManager.cpp` | EEPROM load/save orchestration |
| `DataLogger.cpp` | SPIFFS log session management |
| `Globals.cpp` | Pin constants, EEPROM param name strings |
| `inc/GasManager.h` | Calibration math, EEPROM persistence, up to 4 gas slots |
| `inc/Menu.h` | CompositeMenu tree, NavLimit, all MenuItem classes |
| `inc/MenuRenderer.h` | Renderer class declarations |
| `inc/AnalogSourceInput.h` | ADS1115 A0 averaging, INA219/ADC34 battery fallback |
| `inc/UnitSet.h` | ppm / ppb / 1117 Lamp unit selection |
| `inc/TimeSync.h` | TimeSync class + EEPROM_TIME_BACKUP_OFFSET = 240 |
| `inc/HourSet.h` | RTC hour adjust + EEPROM time backup |
| `inc/MinuteSet.h` | RTC minute adjust + EEPROM time backup |
| `inc/pins.h` | GPIO defines |

## I2C Pins (non-default — critical)
- SDA = **IO 23**, SCL = **IO 22**
- Always use `Wire.begin(23, 22)` — default ESP32 pins (21/22) will miss the RTC and sensors
- Re-assert `Wire.begin(23, 22)` before any RTC call because sensor lib `begin()` calls may reset Wire

## Menu Index Reference
```
 0  Run screen         (navLimit boundary)
 1  Gas Library
 2  Range              (5000 / 10000 ppm over-range threshold)
 3  High Alarm
 4  PEL Low Alarm
 5  Logging Mode
 6  Logging Interval
 7  Set Hour
 8  Set Minute
 9  Display Unit
10  Auto Power-Off     (sleep timer: 5m/1h/2h/6h/continuous)
11  Cal Info           ← last normal menu page (view only)
── navLimit = 12 ──────────────────────────────────────
12  Cal Zero           ← RIGHT from run enters here
13  Cal Gas Value
14  Cal Span Capture
15  Cal Result
```

## Button Logic Summary
- **DOWN**: advance settings (0–11); if idx ≥ 12 → abort cal → run
- **S**: save + `showSavedBanner()` + advance; idx 0 and 11 skip banner
- **RIGHT**: from run → `setCurrentMenu(12)`; elsewhere → cycle submenu values

## EEPROM Key Addresses
- 0: Auto power-off index (0=5m, 1=1h, 2=2h, 3=6h, 4=continuous)
- 8–71: Gas intercepts/slopes (doubles, 8 bytes each, 4 gases × 2 × 8)
- 100: Display unit
- 120: Cal timestamp string
- 210: Manual log entry counter (ushort)
- 240–244: Time backup (yr-2000, month, day, hour, minute)
- `EEPROM_SIZE` = 512 (ConfigurationManager.h) — must exceed the highest address or writes are silently dropped

## Display
- TFT_eSPI, 320×480 portrait, ILI9488
- `drawCenteredString(display, text, y, font, size=1)` — local helper in MenuRenderer.cpp
- Static/dynamic split in `TFTRunMenuRenderer` — call `g_runRenderer->invalidate()` after `fillScreen()`

## What Was Removed
- WiFi, NTP, MQTT (all deleted — no `#include <WiFi.h>` anywhere)
- `MQTTFlashPublisher.h`, `MQTTRealTimePublisher.h`, `WebServer.h` — deleted
- DataLogger flash menu page + flash-session methods — deleted (logging is
  now the Auto/Manual modes writing to `/log.txt`)
- `inc/CalManager.h`, `inc/Gas.h`, `inc/image.h` — dead files, deleted
  (`Gas.h` held a conflicting duplicate `Gas` class; the real one is in GasManager.h)
- ConfigurationManager WiFi/DeviceID/TC/timer legacy methods — deleted
- User ID / DateTime menu pages were never reachable (built but not added
  to the menu tree) — leftover constructions deleted from setup().
  Range got a real settings page at index 2 instead (it sets the
  over-range threshold only; the gain pin is not driven).

## Common Pitfalls
- `struct timeval` / `settimeofday` require `#include <sys/time.h>` (WiFi.h used to pull it in)
- `Calvalue().getSelectedCalvalue()` reads EEPROM directly — temporary object is safe
- RTC backup battery: if PCF8563 VBAT is unconnected, time resets on power-off; EEPROM backup at addr 240 provides fallback
- ADS1115 `begin()` in old Adafruit lib calls `Wire.begin()` with no args — resets pins to 21/22
