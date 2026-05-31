# M30 VerK — Design Document

**Version:** VerK (`FW_VERSION = "M30 VerK 05/30/2026"`)
**Platform:** ESP32 · Arduino framework
**Sensor:** Configurable at compile time via sensor profiles (default: EC NH3 0–100 ppm)

> Ported from the M10-TCD-H2 VerK design. The menu/navigation/RTC/EEPROM
> architecture is shared; M30 adds a compile-time **sensor-profile system**,
> **multi-gas** calibration (up to 4 slots), and a **Bump Test** screen.

---

## 1. Purpose

Portable handheld single-gas analyzer. Reads a sensor (electrochemical, PID,
NDIR, or TCD depending on the selected profile), converts to engineering units,
displays the value on a 128×64 OLED, triggers audible/visual alarms, logs data,
and allows field calibration through a 4-button menu system. The target gas,
unit label, range/alarm/cal ladders, ADS gain, and Bluetooth name are all
selected at compile time by choosing one sensor profile — no `.ino` edits.

---

## 2. Hardware Summary

| Component | Part | Role |
|-----------|------|------|
| MCU | ESP32 | Main controller |
| ADC | ADS1115 | Sensor front-end voltage |
| Power monitor | INA219 | Battery voltage → % |
| RH/T sensor | SHT4x | Ambient conditions |
| RTC | PCF8563 | Timestamps for logs and cal records |
| Display | SSD1306 128×64 OLED (`USE_SSD1306_DISPLAY`) | Menu/run screen |
| Comms | Bluetooth Serial | Name from `SENSOR_BT_NAME` |
| Buzzer | GPIO (`ALARM_PIN`, see `inc/Globals.h`) | Audible alarm |

**I2C bus:** SDA = GPIO 21, SCL = GPIO 22
**Buttons:** DOWN = GPIO 14, S = GPIO 19, RIGHT = GPIO 13, ON = GPIO 12

---

## 3. Sensor Profile System

The active sensor is chosen by uncommenting exactly one `#include` in
[`inc/SensorConfig.h`](inc/SensorConfig.h). Each profile header under
[`inc/sensors/`](inc/sensors/) defines a fixed set of `SENSOR_*` macros:

| Macro | Purpose |
|-------|---------|
| `SENSOR_BT_NAME` | Bluetooth device name |
| `SENSOR_GAS_NAME` | Gas label on the run screen / GasManager gas |
| `SENSOR_UNIT_LABEL` | Unit string (e.g. `"ppm"`, `"%"`, `"% LEL"`) |
| `SENSOR_RES_DIGITS` | Display decimal places |
| `SENSOR_RANGE_FS` / `SENSOR_OVERRANGE` | Full-scale + over-range ("OVR") threshold |
| `SENSOR_ADS_GAIN` | ADS1115 PGA gain |
| `SENSOR_INTERCEPT[2-4]` / `SENSOR_SLOPE[2-4]` | Factory cal defaults per gas slot |
| `SENSOR_RANGE_VALUES` | Range menu ladder |
| `SENSOR_CALVALUE_VALUES` | Cal-value menu ladder |
| `SENSOR_ALARM_VALUES` | High-alarm ladder (first entry `0` = Off) |
| `SENSOR_PEL_VALUES` | PEL/low-alarm ladder (first entry `0` = Off) |

`SensorConfig.h` enforces the contract with `#error` guards (missing
`SENSOR_BT_NAME` / `SENSOR_ADS_GAIN` / `SENSOR_SLOPE` fails the build) and
supplies fallback defaults for the optional display/ladder macros.

The menu ladders are materialized into C arrays in the `*Set.h` headers, e.g.
`k_rangeArr[]`/`k_rangeCount` in [`inc/RangeSet.h`](inc/RangeSet.h), and the
`.ino` builds the Range/Alarm/PEL/Calvalue `CompositeMenu`s by iterating those
arrays. Catalog of supported sensors lives in
[`SENSOR_CONFIGS.md`](SENSOR_CONFIGS.md).

To add a sensor: copy [`inc/sensors/EC_TEMPLATE.h`](inc/sensors/EC_TEMPLATE.h),
fill in the macros, add an `#include` line to `SensorConfig.h`.

---

## 4. Software Architecture

```
M30_VerK_20260514.ino
├── setup()
│   ├── System check (I2C probe peripherals, display result)
│   ├── Populate GasManager gas list from SENSOR_GAS_NAME
│   ├── Build menu tree (CompositeMenu hierarchy from sensor-profile ladders)
│   ├── Bind button callbacks
│   ├── First-boot EEPROM seeding (sentinel 0xA8 @ addr 250)
│   ├── Load EEPROM parameters
│   └── Auto-sync RTC from compile timestamp (if new firmware)
└── loop()
    └── g_mainMenu->render()

Key modules
├── GasManager          Calibration math, ppm calculation, up to 4 gas slots
├── DataSource          Reads ADS1115 → GasManager → value
├── MenuRenderer        One renderer class per screen; draws OLED
├── CompositeMenu       Hierarchical menu tree + navigation guards
└── ConfigurationManager EEPROM serialization for all parameters
```

---

## 5. Menu System

### Structure

A flat horizontal `CompositeMenu` of **14** sub-menus. (M10 had 13; M30 inserts
**Bump Test** at index 12 and pushes Device Info to 13. The Library and
stand-alone Zero-only screens were removed — a single compile-time gas needs no
library, and DOWN from Run enters the full cal sequence directly.)

```
Index  Screen           Access
  0    Run              Normal operation (default)
  1    Cal Zero         DOWN from Run
  2    Cal Value        RIGHT from Cal Zero
  3    Cal Gas          RIGHT from Cal Value → S returns to Run
  4    Cal Result       Skipped — result shown as banner
  5    Alarm High       RIGHT from Run
  6    Low Alarm (PEL)  RIGHT from Alarm
  7    Logging Set      RIGHT from Low Alarm
  8    Logging Time     Skipped in RIGHT sequence
  9    Set Hour         RIGHT from Logging Set
 10    Set Minute       RIGHT from Set Hour
 11    Range            RIGHT from Set Minute
 12    Bump Test        RIGHT from Range → S returns to Run
 13    Device Info      RIGHT from Bump Test
```

### Button Actions

| Button | On Run Screen | In Settings Sub-menu | In Cal / Bump Sequence |
|--------|--------------|----------------------|------------------------|
| DOWN | Enter Cal Zero (idx 1) | Cycle options | Cycle cal-value options |
| RIGHT | Cycle main menu (lands on Alarm, idx 5) | Next setting page | Next cal page |
| S | Manual log (if mode=Manual) | Save + advance | Execute step + banner |
| ON | Power off | Power off | Power off |

RIGHT calls `g_mainMenu->moveToNext()` then `syncSubMenuToCurrent()`.

### Navigation Guards (`moveToNext` in [`inc/Menu.h`](inc/Menu.h))

| Lands on | Redirected to | Reason |
|----------|---------------|--------|
| 1 | 5 | RIGHT from Run skips cal section, goes to Alarm |
| 4 | 0 | Cal Result removed; exit cal sequence back to Run |
| 8 | 9 | Logging Time skipped in RIGHT sequence |

### Sub-menu Sync

`syncSubMenuToCurrent()` reads the saved EEPROM index for the landed page and
positions the cursor on the active value. Hour and Minute always start at 0.
Index→EEPROM map: 2→80 (calvalue), 5→76 (alarm), 6→92 (low alarm),
7→96 (logging set), 8→200 (logging time), 11→72 (range).

---

## 6. Calibration Design

### Two-point field calibration

| Step | Action | Result |
|------|--------|--------|
| Zero | Apply zero gas; press S | Saves intercept = V_zero |
| Span | Apply known cal-gas value; press S | Saves slope = (V_cal − V_zero) / refV |

**Span-rejection guard:** if `|V_cal − V_zero| < 20 mV`, the span is rejected
(`calSuccess = false`, no slope or cal-date written) — too small a signal for a
reliable slope (disconnected/dead sensor or no gas applied). Implemented in
`GasManager::calibrate2()`.

### ppm Calculation (`GasManager::calculateSLM`)

```
value = ((V_sensor − intercept) / slope) × 1000
```

- Returns 0 if the selected gas slot's slope == 0 (before first calibration)
- Returns 0 if the result is negative (sensor below zero baseline)
- Per-gas slope/intercept selected by `m_selectedGas` (slots 0–3)

### Multi-gas slots

Up to 4 gas slots (intercept/intercept2–4, slope/slope2–4) support composite
sensors such as the 4-in-1 profile. Single-gas profiles use slot 0 only.

### Persistence

- Intercept/slope stored as `double`; slot _n_ at EEPROM `8 + n·16` /
  `16 + n·16`
- Cal timestamp stored as ASCII string at EEPROM address 120
- Loaded on boot via `ConfigurationManager::loadFromEEPROM()`

---

## 7. First-Boot EEPROM Seeding

A blank ESP32 flash reads `0xFF` everywhere; `readDouble` of `0xFF…` returns a
signaling NaN that overflows `dtostrf` → boot loop. On boot the `.ino` checks a
sentinel byte at **addr 250**; if it is not `0xA8`, it writes safe defaults
(intercepts/slopes from the sensor profile, all menu indices = 0,
`"not calibrated"` timestamp) and sets the sentinel — *before*
`loadFromEEPROM()` runs.

---

## 8. Data Logging

| Mode | Trigger |
|------|---------|
| Off | — |
| Auto | Timer interval (1 s / 10 s / 1 min / 10 min / 1 hr) |
| Manual | S button on Run screen |
| Clear | "Clear Log" in Logging Set menu |

File capped at 200 entries; oldest dropped on overflow.

---

## 9. Time / RTC

- **Display:** `getLocalTime()` from ESP32 system clock
- **Sync path:** `syncTimeFromRTC()` → `settimeofday()` → `getLocalTime()`
- **Setting time:** Hour/Minute menus; S handler calls `rtc.adjust()` then
  `syncTimeFromRTC()`
- **Auto-set on upload:** `autoSyncRTCFromBuild()` compares stored `FW_BUILD`
  string (EEPROM addr 152) to `__DATE__ " " __TIME__`; sets RTC to compile
  time only when the string changed (new firmware upload). Normal reboots leave
  a manually-set time untouched.

---

## 10. EEPROM Map

| Address | Bytes | Parameter |
|---------|-------|-----------|
| 4 | 4 | Gas index |
| 8 | 8 | Intercept slot 0 (double) |
| 16 | 8 | Slope slot 0 (double) |
| 24 | 8 | Intercept slot 1 |
| 32 | 8 | Slope slot 1 |
| 40 | 8 | Intercept slot 2 |
| 48 | 8 | Slope slot 2 |
| 56 | 8 | Intercept slot 3 |
| 64 | 8 | Slope slot 3 |
| 72 | 1 | Range index |
| 76 | 1 | High alarm index |
| 80 | 1 | Cal value index |
| 92 | 1 | Low alarm (PEL) index |
| 96 | 1 | Logging mode index |
| 120 | 32 | Cal timestamp (ASCII) |
| 152 | 28 | FW_BUILD string |
| 200 | 1 | Logging interval index |
| 250 | 1 | First-boot seed sentinel (0xA8) |

> Slots 1–3 (addr 24–64) and the seed sentinel (250) are M30 additions over the
> M10 map.

---

## 11. Differences vs. M10-TCD-H2 Design

- **Sensor-profile system** — gas/unit/ranges/gain selected at compile time (§3)
- **Multi-gas calibration** — up to 4 slots; composite-sensor support (§6)
- **Bump Test screen** (idx 12) — quick pass/fail vs. current cal: passes if
  `|live − ref| / ref ≤ 20 %`; writes no EEPROM (`BumpTestMenuItem` in `Menu.h`)
- **Library / stand-alone Zero screens removed** — index map shifted accordingly
- **First-boot EEPROM seeding** (§7)
