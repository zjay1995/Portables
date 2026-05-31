# M10-TCD-H2 VerK — Firmware Reference

Single-gas H2 TCD (Thermal Conductivity Detector) portable analyzer built on ESP32.  
Display: SSD1306 128x64 OLED · Sensors: ADS1115, INA219, SHT4x, PCF8563 RTC

---

## Hardware

| Item | Detail |
|------|--------|
| MCU | ESP32 |
| Gas sensor input | ADS1115 ADC (0.125 mV/count, +/-4.096 V range) |
| Battery monitor | INA219 |
| Temp / Humidity | SHT4x |
| Real-time clock | PCF8563 (I2C address 0x51) |
| I2C pins | SDA = GPIO 21, SCL = GPIO 22 |

### Buttons

| Button | GPIO | Run screen | Settings menus | Cal sequence |
|--------|------|-----------|----------------|--------------|
| DOWN | 14 | Enter Cal Zero | Cycle options within sub-menu | Cycle cal-value options |
| S | 19 | Manual log (if enabled) | Confirm / save | Execute cal step |
| UP | 13 | Enter Settings (Alarm first) | Advance to next setting | Advance to next cal step |
| ON | 12 | Power off | Power off | Power off |

---

## EEPROM Map

| Address | Size | Parameter |
|---------|------|-----------|
| 4 | 4 B | Gas index (always 0 for H2) |
| 8 | 8 B | Intercept / zero voltage (V, double) |
| 16 | 8 B | Slope (V/ppm, double) |
| 72 | 1 B | Range index |
| 76 | 1 B | High alarm index |
| 80 | 1 B | Cal reference ppm index |
| 92 | 1 B | Low alarm index |
| 96 | 1 B | Logging mode index |
| 120 | 32 B | Calibration timestamp string (ASCII) |
| 152 | 28 B | FW_BUILD string — used for RTC auto-set detection |
| 200 | 1 B | Logging interval index |
| 210 | 2 B | Manual log count (uint16) |
| 220 | 4 B | Hour index |
| 230 | 4 B | Minute index |

All indices saved with `EEPROM.writeInt` / read back with `EEPROM.read` (low byte).  
Slope and intercept loaded via `String(value, 10)` to preserve small values (e.g. 0.00002).

---

## Main Menu Index Map

| Index | Screen | Notes |
|-------|--------|-------|
| 0 | Run screen | Normal measurement |
| 1 | Cal Zero | Set zero intercept |
| 2 | Cal Value | Select reference H2 ppm |
| 3 | Cal Gas | Span calibration |
| 4 | Cal Result | Skipped — result shown as banner on index 3 |
| 5 | Alarm High | ppm threshold, 0 = Off |
| 6 | Low Alarm | ppm threshold, 0 = Off |
| 7 | Logging Set | Off / Auto / Manual / Clear |
| 8 | Logging Time | Interval — skipped in UP navigation |
| 9 | Set Hour | Always opens at 0 |
| 10 | Set Minute | Always opens at 0 |
| 11 | Range | Full-scale ppm range |
| 12 | Device Info | FW version, cal date, intercept, slope |

**Navigation guards in `moveToNext()`:**
- `0 -> 1` skips to `5` (DOWN from run goes to Alarm, not Cal)
- `4` skips to `5` (Cal Result removed from normal flow)
- `8` skips to `9` (Logging Time skipped in UP sequence)

**Sub-menu sync:** When UP advances the main menu, `syncSubMenuToCurrent()` reads the
saved EEPROM index and positions the sub-menu cursor on the active value.
Hour and minute always start at 0.

---

## Display Screens

### Run Screen (index 0)

```
TCD                    14:35   B 85%
-------------------------------------
H2
          1250
                              ppm
-------------------------------------
Alarm ON              1055mV
```

- Value centered with `ArialMT_Plain_24` at y = 18
- "OVR" if value > range; "Above Maximum / Concentration" if >= 11000 ppm
- Battery: 16-sample rolling average, rounded to nearest 5%
- Alarm blink: alternates "ALARM" / value at 500 ms; buzzer active for first 30 s

### Cal Zero (index 1)

- Header: `Cal: Zero  [H2]`
- y=20: "Insert zero gas" / y=30: live mV / y=40: "Press S when stable"
- No footer

### Cal Value — Ref H2 ppm (index 2)

- Header: `Ref H2 (ppm)` · selected value in ArialMT_Plain_16
- Footer: `MENU | SET | OPTION`
- Opens at currently-saved cal value

### Cal Gas / Span (index 3)

- Header: `Calibration - Span`
- y=20: cal gas ppm / y=30: live mV / y=40: "Apply cal gas"
- No footer
- **S press:** shows `CAL SUCCESS` or `CAL FAILED` banner (800 ms) then returns to run screen

### Settings Menus (Alarm, Low Alarm, Range, Loggingset, Hour, Minute)

- Header: setting name · selected option in ArialMT_Plain_16
- Footer: `MENU | SET | OPTION`
- All open at currently-saved value (hour and minute always start at 0)

### Device Info (index 12)

- FW version split across two centered lines (y=13, y=23)
- y=34: cal timestamp / y=44: intercept (5 dp) / y=54: slope (5 dp)

---

## Calibration Logic (`inc/GasManager.h`)

### Zero cal — `calibrate(double zero_mV)`

1. Resets `calSuccess = false`
2. `intercept = zero_mV / 1000.0`
3. Saves intercept to EEPROM address 8

### Span cal — `calibrate2(double cal_mV)`

1. `calgas = cal_mV / 1000.0`
2. Reads reference ppm from CalvalueSet (EEPROM address 80)
3. **Fails** if `|calgas - intercept| < 0.020 V` (< 20 mV — insufficient sensor response)
4. `slope = (calgas - intercept) / refPpm`
5. Saves slope to EEPROM address 16
6. Sets `calSuccess = true` only on success
7. Saves calibration timestamp (from RTC) to EEPROM address 120

### ppm Calculation

```
ppm = (voltage_V - intercept) / slope
```

Returns 0 if slope == 0 or result < 0. No GCF correction (single gas, GCF = 1.0).

---

## Time Setting

- Hour and minute menus cycle 0-23 and 0-59; always open at 0
- On S press the `.ino` handler adjusts the RTC directly:
  - Hour save: replaces hour only, reads live minute from RTC
  - Minute save: replaces minute only, reads live hour from RTC
- `syncTimeFromRTC()` pushes updated RTC time to ESP32 system clock

### RTC Auto-Set on Upload

On every boot `autoSyncRTCFromBuild()` compares the `FW_BUILD` string (`__DATE__ __TIME__`)
stored at EEPROM address 152 with the current compile string. If different (new upload),
the RTC is set to compile timestamp and the new string is saved. Normal reboots leave the
RTC untouched.

---

## Modified Files

| File | Role |
|------|------|
| `M10_TCD_H2_VerK.ino` | Main sketch — setup, button handlers, menu construction, `syncSubMenuToCurrent()` |
| `MenuRenderer.cpp` | All OLED screen layouts |
| `inc/GasManager.h` | Calibration logic, ppm calculation, `calSuccess` flag |
| `inc/Menu.h` | `CompositeMenu` navigation, `moveToNext()` guards, `setCurrentMenu()` |
| `inc/ConfigurationManager.cpp` | EEPROM load/save — slope/intercept use 10 dp string precision |
| `inc/HourSet.h` | Hour selection — saves index to EEPROM 220, RTC adjustment done in .ino |
| `inc/MinuteSet.h` | Minute selection — saves index to EEPROM 230, RTC adjustment done in .ino |
