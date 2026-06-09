# Model 103 (M103) — Display Demo
**Sketch:** 103_Demo &nbsp;|&nbsp; **Based on:** M103_board_20260504_beta (05/13/26) &nbsp;|&nbsp; **MCU:** Adafruit HUZZAH32 (ESP32 Feather)

---

## Demo Overview

This is a display-only demo build of the M103 PID Analyzer firmware, intended for trade-show or bench demonstration units. It runs on an **Adafruit HUZZAH32** (ESP32 Feather) driving the same 3.5-inch 320×480 ILI9488 TFT panel as the production M103.

The code is a direct copy of `M103_board_20260504_beta` with three small changes:
- `getMiliVolts()` in `AnalogSourceInput.h` returns a sine-wave animated value (0–300 ppm, 60-second period) instead of reading the ADS1115.
- `getMiliVolts_battery()` returns a fixed 3900 mV (78%, green) — the HUZZAH32 does not have the M103 INA219 / IO-34 battery circuit.
- `GasManager` default calibration set to intercept = 1.040 V, slope = 1.0 with an EEPROM-blank fallback, so the reading displays correctly on a fresh board.

All menus, button logic, splash screen, system check, and screen layouts are **identical** to production firmware.

---

## Setup

1. Copy `User_Setup.h` (in this folder) to your TFT_eSPI library folder, replacing the existing one.
   - Example path: `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`
2. Select board **Adafruit ESP32 Feather** (or *ESP32 Dev Module*) in Arduino IDE.
3. Wire the display per the table below and upload.

---

## Hardware

### HUZZAH32 TFT Wiring

| TFT pin | GPIO | HUZZAH32 label |
|---------|------|----------------|
| MISO | 19 | 19 (MISO) |
| MOSI | 18 | 18 (MOSI) |
| SCLK | 5 | 5 (SCK) |
| CS | 26 | 26 (A0) |
| DC | 27 | 27 (A10) |
| RST | 4 | 4 (A5) |
| VCC | 3.3 V | 3V |
| GND | GND | GND |

### Other Pin Map (unchanged from M103)

| Net | GPIO | Direction | Notes |
|---|---|---|---|
| Button DOWN | IO 13 | INPUT_PULLUP | Advance menu |
| Button S | IO 21 | INPUT_PULLUP | Save / confirm |
| Button RIGHT (UP) | IO 14 | INPUT_PULLUP | Cycle value / enter cal |
| Power Switch | IO 12 | INPUT_PULLUP / EXT0 | Deep-sleep wake |
| ALARM | IO 15 | OUTPUT | Relay / buzzer, idle low |
| LAMPOUT | IO 16 | OUTPUT | UV lamp enable, active-high |
| PID_GAIN | IO 17 | — | Reserved (gain select unused; firmware does not drive it) |
| HVON_STATUS | IO 36 (SENSOR_VP) | INPUT only | HV stage feedback |
| POWER HOLD | IO 25 | OUTPUT | Keep power on, active-high |
| I2C SDA | IO 23 | Bidirectional | Same as HUZZAH32 default |
| I2C SCL | IO 22 | Bidirectional | Same as HUZZAH32 default |

> **Note:** Buttons, alarm relay, and deep-sleep are wired and functional if the corresponding hardware is attached. For a display-only demo unit they can be left unconnected.

### System Check on Startup

All five items are hardcoded to **[ OK ]** in the demo build. The screen shows "ALL SYSTEMS OK" and clears after 2 s.

---

## 2. Hardware

### 2.1 Pin Map (M103 Production Reference)

| Net | GPIO | Direction | Notes |
|---|---|---|---|
| Button DOWN | IO 13 | INPUT_PULLUP | Advance menu |
| Button S | IO 21 | INPUT_PULLUP | Save / confirm |
| Button RIGHT (UP) | IO 14 | INPUT_PULLUP | Cycle value / enter cal |
| Power Switch | IO 12 | INPUT_PULLUP / EXT0 | Deep-sleep wake |
| ALARM | IO 15 | OUTPUT | Relay / buzzer, idle low |
| LAMPOUT | IO 16 | OUTPUT | UV lamp enable, active-high |
| PID_GAIN | IO 17 | — | Reserved (gain select unused; firmware does not drive it) |
| HVON_STATUS | IO 36 (SENSOR_VP) | INPUT only | HV stage feedback |
| VBATT_ADC | IO 34 | ADC input | 100K/100K divider from VBATT |
| POWER HOLD | IO 25 | OUTPUT | Keep power on, active-high |
| I2C SDA | IO 23 | Bidirectional | RTC, ADS1115, INA219, SHT4x |
| I2C SCL | IO 22 | Bidirectional | Same bus |

### 2.2 I2C Devices (M103 Production Reference)

| Device | Address | Library | Function | Demo |
|---|---|---|---|---|
| PCF8563 RTC | 0x51 | RTClib | Real-time clock, cal timestamps | Optional |
| ADS1115 ADC | 0x48 | Adafruit_ADS1015 | PID signal — channel A0, 16-bit | **Not used** — replaced by sine wave |
| INA219 | 0x40 | Adafruit_INA219 | Battery bus voltage (primary) | **Not used** — fixed 3900 mV |
| SHT4x | 0x44 | Adafruit_SHT4x | Temperature / humidity | Optional |

### 2.3 Power & Battery
- **Demo:** Fixed 3900 mV → displays 78%, green
- **Production thresholds:** < 3.4 V empty, ≥ 4.05 V full; status bar color-coded (red / yellow / green)

---

## 3. Software Architecture

### 3.1 Key Modules

| Module | File(s) | Responsibility |
|---|---|---|
| **GasManager** | `inc/GasManager.h` | Up to 4 gas slots; intercept/slope calibration; EEPROM persistence |
| **AnalogSourceInput** | `inc/AnalogSourceInput.h` | **Demo:** `getMiliVolts()` returns animated sine wave; battery fixed at 3900 mV |
| **TimeSync** | `inc/TimeSync.h`, `TimeSync.cpp` | PCF8563 → ESP32 system clock; EEPROM time backup |
| **UnitSet** | `inc/UnitSet.h` | Display unit: ppm / ppb / 1117 Lamp; persisted at EEPROM 100 |
| **Menu / CompositeMenu** | `inc/Menu.h` | Composite menu tree with `setNavLimit()` for cal zone isolation |
| **MenuRenderer** | `inc/MenuRenderer.h`, `MenuRenderer.cpp` | Per-screen TFT renderers; shared footer helper |
| **DataLogger** | `inc/DataLogger.h` | SPIFFS `/log.txt` auto/manual session logging |
| **HourSet / MinuteSet** | `inc/HourSet.h`, `inc/MinuteSet.h` | RTC time adjust + EEPROM time backup on save |

### 3.2 Libraries

| Library | Purpose |
|---|---|
| `TFT_eSPI` | 3.5" SPI display driver |
| `RTClib` | PCF8563 real-time clock |
| `Adafruit_ADS1X15` | ADS1115 16-bit ADC |
| `Adafruit_INA219` | Battery current/voltage monitor |
| `Adafruit_SHT4x` | Temperature / humidity |
| `BluetoothSerial` | BT Classic SPP log retrieval |
| `SPIFFS` | Flash file system for log |
| `EEPROM` | Settings persistence |
| `sys/time.h` | `settimeofday()` for ESP32 system clock |

---

## 4. EEPROM Address Map

| Address | Bytes | Content |
|---|---|---|
| 0 | 4 | Timer interval |
| 4 | 4 | Gas index |
| 8 | 8 | Gas 1 intercept (double) |
| 16 | 8 | Gas 1 slope (double) |
| 24 | 8 | Gas 2 intercept (double) |
| 32 | 8 | Gas 2 slope (double) |
| 40 | 8 | Gas 3 intercept (double) |
| 48 | 8 | Gas 3 slope (double) |
| 56 | 8 | Gas 4 intercept (double) |
| 64 | 8 | Gas 4 slope (double) |
| 72 | 4 | Range index |
| 76 | 4 | High alarm index |
| 80 | 4 | Cal gas value index |
| 88 | 4 | Output port index |
| 92 | 4 | Low alarm index |
| 96 | 4 | Logging mode index |
| 100 | 1 | Display unit (0=ppm, 1=ppb, 2=1117) |
| 120 | ~20 | Cal timestamp string "YYYY-MM-DD HH:MM" |
| 200 | 4 | Logging interval index |
| 220 | 4 | Hour setting |
| 230 | 4 | Minute setting |
| 240 | 5 | Time backup: yr−2000, month, day, hour, minute |

---

## 5. Menu Structure

```
Idx  Screen            Button Access       Notes
──────────────────────────────────────────────────────
 0   Run (live)        —                   Always visible
 1   Gas Library       DOWN
 2   High Alarm        DOWN
 3   PEL Low Alarm     DOWN
 4   Logging Mode      DOWN
 5   Logging Interval  DOWN
 6   Set Hour          DOWN
 7   Set Minute        DOWN
 8   Display Unit      DOWN                ppm / ppb / 1117
 9   Cal Info          DOWN                View only: factors + date
── navLimit = 10 : DOWN wraps 9 → 0 ────────────────────
10   Cal Zero          RIGHT from run      Step 1/2
11   Cal Gas Value     auto-advance        Set span ppm
12   Cal Span          auto-advance        Step 2/2
13   Cal Result        auto-advance        Pass / Fail → run
```

---

## 6. Button Behaviour

| Button | Run (0) | Settings (1–9) | Cal zone (10–13) |
|---|---|---|---|
| **DOWN** | No action | Advance; wraps 9→0 | Abort → run (0) |
| **S** | Log trigger only | Save + banner + advance | Save + cal banner + advance |
| **RIGHT** | Enter cal → screen 10 | Cycle value in submenu | Cycle value in submenu |

**S-button banners:**

| Screen | Banner |
|---|---|
| 0 (run) | *(none)* |
| 9 (cal info) | *(none — view only)* |
| 1–8, 10–11 (settings / cal steps) | `VALUE SAVED` |
| 12 (span capture) | `CAL COMPLETE` |
| 13 (cal result) | `CAL FINISHED` |

**Standard footer (all settings screens):** `MENU  |  SET  |  OPTIONS`
**Cal gas value footer:** `ABORT  |  SET  |  OPTIONS`

---

## 7. Screen Layouts

### Run Screen
```
┌──────────────────────┐
│ M103   HH:MM    🔋%  │  header (38 px): model, time, battery
├──────────────────────┤
│                      │
│        1234          │  large reading (red)
│         ppm          │  unit label (ppm / ppb / 1117)
│                      │
├──────────────────────┤
│ ALARM  200ppm  [ON]  │  alarm panel
│ PEL     50ppm  [ON]  │
├──────────────────────┤
│ LOG: Auto  HH:MM:SS  │  status panel
├──────────────────────┤
│ M103    S:LOG  VALUE │  footer
└──────────────────────┘
```

### Settings Screens (idx 1–9)
```
┌──────────────────────┐
│      [TITLE]         │  y=5, font 4
│ ──────────────────── │  y=35
│                      │
│       VALUE          │  centered, font 4 size 2
│                      │
│ ──────────────────── │
│  MENU    SET OPTIONS │  standard footer
└──────────────────────┘
```

### Cal Info Screen (idx 9)
```
┌──────────────────────┐
│     CAL FACTORS      │
│ ──────────────────── │
│  Last Calibration:   │
│  2026-05-13 11:30    │
│ ──────────────────── │
│  ZERO                │
│     1045.2 mV        │  intercept × 1000, font 4 size 2
│ ──────────────────── │
│  SLOPE               │
│    0.000842          │  6 decimal places, font 4 size 2
│  Gas: PID            │
│ ──────────────────── │
│  MENU    SET OPTIONS │
└──────────────────────┘
```

### Cal Zero (idx 10)
```
┌──────────────────────┐
│  STEP 1/2 — ZERO     │
│ ──────────────────── │
│   Apply Zero Gas     │
│    (clean air)       │
│                      │
│      1038 mV         │  live, font 4 size 3
│                      │
│  [ STABILIZING... ]  │  or [ STABLE ] green
│                      │
│  Press S to Capture  │
└──────────────────────┘
```

### Cal Gas Value (idx 11)
```
┌──────────────────────┐
│    Cal Gas Value     │
│ ──────────────────── │
│                      │
│       100 ppm        │  font 4 size 2
│                      │
│ ──────────────────── │
│ ABORT    SET OPTIONS │
└──────────────────────┘
```

### Cal Span (idx 12)
```
┌──────────────────────┐
│  STEP 2/2 — SPAN     │
│ ──────────────────── │
│   Apply Cal Gas      │
│      100 ppm         │  font 4 size 2
│                      │
│      1245 mV         │  live, font 4 size 3
│                      │
│  [ STABLE ]          │
│  Press S to Capture  │
└──────────────────────┘
```

### Cal Result (idx 13)
```
┌──────────────────────┐
│      Cal Result      │
│ ──────────────────── │
│                      │
│       Success        │  green, font 4 size 3
│         or           │
│        Failed        │  red, font 4 size 3
│                      │
│           OK         │
└──────────────────────┘
```

### System Check (startup, after splash)
```
┌──────────────────────┐
│     SYSTEM CHECK     │
│ ──────────────────── │
│  RTC  PCF8563  [ OK ]│  I2C probe 0x51
│  ADC  ADS1115  [ OK ]│  I2C probe 0x48
│  BAT  INA219   [FAIL]│  ina219.begin()
│  RHT  SHT4x    [ OK ]│  sht4x.begin()
│  FLASH SPIFFS  [ OK ]│  SPIFFS.begin()
│ ──────────────────── │
│   FAULT DETECTED     │  red; or ALL SYSTEMS OK green
└──────────────────────┘
Holds 4 s on fault, 2 s on pass.
```

---

## 8. Calibration Flow

```
RIGHT from run screen
        │
        ▼
  [10] Zero ──S──► VALUE SAVED ──► [11] Cal Gas Value
                                           │ S
                                           ▼
                                     VALUE SAVED
                                           │
                                           ▼
                                   [12] Span Capture
                                           │ S
                                           ▼
                                     CAL COMPLETE
                                           │
                                           ▼
                                    [13] Cal Result
                                           │ S
                                           ▼
                                      CAL FINISHED
                                           │
                                           ▼
                                    [0] Run screen

DOWN at any cal screen ──► abort ──► [0] Run screen
```

**Slope validation:** `slope = (V_span − V_zero) / (ppm / 1000)`. If slope ≤ 0.00001 or span gas concentration is zero, calibration is rejected (result: Failed) and EEPROM is unchanged.

---

## 9. Time & RTC

- **Primary:** PCF8563 at I2C 0x51 (SDA=23, SCL=22)
- **Startup sequence:** `Wire.begin(23,22)` → re-assert after sensor lib inits → `initTimeFromRTC()`
- **Valid time:** year ≥ 2024 → sync ESP32 system clock + write EEPROM backup (addr 240–244)
- **Invalid time (no battery):** restore from EEPROM backup; Serial prints `"RTC invalid — restored from EEPROM"`
- **On hour/minute save:** EEPROM backup updated immediately alongside RTC adjust
- **Hardware fix:** connect CR2032 or 0.1 F supercap to PCF8563 VBAT pin for true battery-backed timekeeping

---

## 10. Data Logging & Bluetooth

- **Modes:** Off / Auto (interval-based) / Manual / Clear Log
- **Intervals:** 1 s, 10 s, 1 min, 10 min, 1 hour
- **Storage:** SPIFFS `/log.txt`
- **Bluetooth:** Classic SPP, device name `ESP32 PID M103`
  - `PID73*` → dumps `/log.txt` over Serial + BT
  - `CLEARLOG` → erases log file
