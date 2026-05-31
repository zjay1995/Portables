# M10-TCD-H2 VerK — Design Document

**Version:** VerK  
**Platform:** ESP32 · Arduino framework  
**Sensor:** Thermal Conductivity Detector (TCD), single gas H2

---

## 1. Purpose

Portable handheld H2 gas analyzer. Measures hydrogen concentration in ppm using a
thermal conductivity detector bridge, displays value on a 128×64 OLED, triggers
audible/visual alarms, logs data to SPIFFS, and allows field calibration through a
4-button menu system.

---

## 2. Hardware Summary

| Component | Part | Role |
|-----------|------|------|
| MCU | ESP32 | Main controller |
| ADC | ADS1115 | TCD bridge voltage, 0.125 mV/count |
| Power monitor | INA219 | Battery voltage → % |
| RH/T sensor | SHT4x | Ambient conditions |
| RTC | PCF8563 | Timestamps for logs and cal records |
| Display | SSD1306 128×64 | OLED menu/run screen |
| Storage | SPIFFS (internal flash) | CSV log file |
| Buzzer | GPIO (ALARM_PIN) | Audible alarm |

**I2C bus:** SDA = GPIO 21, SCL = GPIO 22  
**Buttons:** DOWN = GPIO 14, S = GPIO 19, RIGHT = GPIO 13, ON = GPIO 12

---

## 3. Software Architecture

```
M10_TCD_H2_VerK.ino
├── setup()
│   ├── System check (I2C probe all peripherals, display result)
│   ├── Build menu tree (CompositeMenu hierarchy)
│   ├── Bind button callbacks
│   ├── Load EEPROM parameters
│   └── Auto-sync RTC from compile timestamp (if new firmware)
└── loop()
    └── g_mainMenu->render()  (called every ~100 ms)

Key modules
├── GasManager          Calibration math, ppm calculation
├── DataSource          Reads ADS1115 → calls GasManager → ppm
├── MenuRenderer        One renderer class per screen; draws OLED
├── CompositeMenu       Hierarchical menu tree + navigation guards
└── ConfigurationManager EEPROM serialization for all parameters
```

---

## 4. Menu System

### Structure

The main menu is a flat horizontal `CompositeMenu` of 13 sub-menus.
Each sub-menu is itself a `CompositeMenu` of selectable items (except
cal pages which are single-action leaf menus).

```
Index  Screen           Access
  0    Run              Normal operation (default)
  1    Cal Zero         DOWN from Run
  2    Cal Value        RIGHT from Cal Zero
  3    Cal Gas          RIGHT from Cal Value
  4    Cal Result       Skipped — result shown as banner
  5    Alarm High       RIGHT from Run
  6    Low Alarm        RIGHT from Alarm
  7    Logging Set      RIGHT from Low Alarm
  8    Logging Time     Skipped in RIGHT sequence
  9    Set Hour         RIGHT from Logging Set
 10    Set Minute       RIGHT from Set Hour
 11    Range            RIGHT from Set Minute
 12    Device Info      RIGHT from Range
```

### Button Actions

| Button | On Run Screen | In Settings Sub-menu | In Cal Sequence |
|--------|--------------|---------------------|-----------------|
| DOWN | Enter Cal Zero (idx 1) | Cycle options | Cycle cal-value options |
| RIGHT | Enter Alarm (idx 5) | Next setting page | Next cal page |
| S | Manual log (if mode=Manual) | Save + advance | Execute cal step |
| ON | Power off | Power off | Power off |

### Navigation Guards (`moveToNext` in Menu.h)

| Lands on | Redirected to | Reason |
|----------|---------------|--------|
| 1 | 5 | RIGHT from Run skips cal, goes to Alarm |
| 4 | 5 | Cal Result removed; result shown as banner |
| 8 | 9 | Logging Time skipped in RIGHT sequence |

### Sub-menu Sync

When RIGHT navigates to a new settings page, `syncSubMenuToCurrent()` reads
the saved EEPROM index and positions the cursor on the active value so the
user sees what is currently set. Hour and minute always start at 0.

---

## 5. Screens

### Run Screen

```
 TCD              14:35   B 85%
 ─────────────────────────────
 H2
         1250
                          ppm
 ─────────────────────────────
 Alarm ON           1055mV
```

- Large centered ppm value (ArialMT_Plain_24)
- Ceiling: "Above Maximum Concentration" at ≥ 11 000 ppm
- Over-range: "OVR" if value > selected range
- Alarm blink: alternates "ALARM" / value every 500 ms for up to 30 s
- Battery: 16-sample rolling average, rounded to nearest 5%

### Cal Zero (idx 1)

```
     Cal: Zero  [H2]
 ────────────────────
 Insert zero gas
 Det: 1055 mV
 Press S when stable
```

No footer. S press saves intercept and shows "ZERO SET" banner.

### Cal Value / Ref H2 ppm (idx 2)

```
      Ref H2 (ppm)
 ────────────────────
      1000 ppm

 ────────────────────
 MENU      SET   OPTION
```

Opens at currently saved cal value. DOWN cycles options, S saves.

### Cal Gas / Span (idx 3)

```
  Calibration - Span
 ────────────────────
 Cal gas: 1000 ppm
 Det: 1072 mV
 Apply cal gas
```

No footer. S press runs span cal, shows "CAL SUCCESS" or "CAL FAILED"
banner (800 ms), then returns directly to Run screen.

### Settings Pages (Alarm, Low Alarm, Range, Logging, Hour, Minute)

```
     Alarm : High
 ────────────────────
      1000 ppm

 ────────────────────
 MENU      SET   OPTION
```

Opens at active saved value. DOWN cycles, S saves and advances to next page.

### Device Info (idx 12)

```
      DEVICE INFO
 ───────────────────
  M10-TCD-H2 VerK
    05/15/2026
 Cal: 2026-05-15 09:30
 Int: 1.05500
 Slp: 0.00002
```

---

## 6. Calibration Design

### Two-point field calibration

| Step | Action | Result |
|------|--------|--------|
| Zero | Apply zero gas; press S | Saves intercept = V_zero |
| Span | Apply known H2 ppm; press S | Saves slope = (V_cal − V_zero) / ppm |

**Failure condition:** If |V_cal − V_zero| < 20 mV the span is rejected
(`calSuccess = false`) — too small a signal to produce a reliable slope.

### ppm Calculation

```
ppm = (V_sensor − intercept) / slope
```

- Returns 0 if slope = 0 (before first calibration)
- Returns 0 if result is negative (sensor below zero baseline)
- No GCF correction — single gas, GCF = 1.0

### Calibration persistence

- Intercept stored as `double` at EEPROM address 8
- Slope stored as `double` at EEPROM address 16
- Timestamp (RTC) stored as ASCII string at EEPROM address 120
- Loaded on boot via `ConfigurationManager::loadFromEEPROM()` using
  `String(value, 10)` to preserve precision of small slopes (e.g. 0.00002)

---

## 7. Data Logging

| Mode | Trigger | File |
|------|---------|------|
| Off | — | — |
| Auto | Timer interval (1 s / 10 s / 1 min / 10 min / 1 hr) | /log.txt |
| Manual | S button on Run screen | /log.txt |
| Clear | Select "Clear Log" in Logging Set menu | Truncates /log.txt |

Log entry format:
```
May 15 2026 09:30:45 , H2, 1250 ppm, 1072mV, Range: 5000 ppm, userID: 0
```

File capped at 200 entries; oldest entries dropped on overflow.

---

## 8. Time / RTC

- **Display:** `getLocalTime()` from ESP32 system clock (UTC, no timezone)
- **Sync path:** `syncTimeFromRTC()` → `settimeofday()` → `getLocalTime()`
- **Setting time:** Hour and minute menus save index to EEPROM; S handler
  in `.ino` calls `rtc.adjust()` directly, then `syncTimeFromRTC()`
- **Auto-set on upload:** `autoSyncRTCFromBuild()` compares stored
  FW_BUILD string (EEPROM addr 152) to `__DATE__+" "+__TIME__`; sets
  RTC to compile time only when the string has changed (new firmware upload)

---

## 9. EEPROM Map

| Address | Bytes | Parameter |
|---------|-------|-----------|
| 4 | 4 | Gas index |
| 8 | 8 | Intercept (double) |
| 16 | 8 | Slope (double) |
| 72 | 1 | Range index |
| 76 | 1 | High alarm index |
| 80 | 1 | Cal value index |
| 92 | 1 | Low alarm index |
| 96 | 1 | Logging mode index |
| 120 | 32 | Cal timestamp (ASCII) |
| 152 | 28 | FW_BUILD string |
| 200 | 1 | Logging interval index |
| 210 | 2 | Manual log count |
| 220 | 4 | Hour index |
| 230 | 4 | Minute index |
