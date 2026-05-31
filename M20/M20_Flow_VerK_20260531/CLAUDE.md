# M10-TCD-H2 VerK — Claude Code Context

ESP32 Arduino firmware for a single-gas H2 TCD (Thermal Conductivity Detector) portable
analyzer. See README.md for full hardware/screen/calibration reference.

---

## Project Structure

```
M10_TCD_H2_VerK.ino        Main sketch — setup(), loop(), button handlers, menu wiring
MenuRenderer.cpp            All OLED screen drawing code
ConfigurationManager.cpp    EEPROM read/write for all parameters
Globals.cpp                 Pin constants and global ConfigurationManager instance
ButtonPressDetector.cpp     Button debounce logic
DataLogger.cpp              SPIFFS log file management
inc/
  GasManager.h              Calibration (calibrate/calibrate2), ppm calculation
  Menu.h                    CompositeMenu, all MenuItem classes, navigation guards
  ConfigurationManager.h    EEPROM address constants, save/load declarations
  AnalogSourceInput.h       ADS1115 voltage reading (getMiliVolts), INA219 battery
  DataSource.h              Wraps AnalogSourceInput + GasManager -> getDoubleValue()
  AlarmSet.h                High alarm index, EEPROM addr 76
  LowAlarmSet.h             Low alarm index, EEPROM addr 92
  RangeSet.h                Range index, EEPROM addr 72
  CalvalueSet.h             Reference ppm index, EEPROM addr 80
  HourSet.h                 Hour index, EEPROM addr 220 (RTC set done in .ino)
  MinuteSet.h               Minute index, EEPROM addr 230 (RTC set done in .ino)
  LoggingSet.h              Logging mode index, EEPROM addr 96
  LoggingTime.h             Logging interval index, EEPROM addr 200
  GasManager.h              Intercept EEPROM addr 8, Slope EEPROM addr 16
  MenuRenderer.h            Renderer class declarations
  Globals.h                 Button pin extern declarations
  pins.h                    Legacy pin defines (NOT used — Globals.cpp overrides)
```

---

## Critical Hardware Facts

- **Button pins (Globals.cpp — authoritative):**
  - DOWN = GPIO 14, S = GPIO 19, UP = GPIO 13, ON = GPIO 12
  - `pins.h` has different values — it is NOT used by Button.h
- **I2C:** SDA = GPIO 21, SCL = GPIO 22
- **ADS1115 multiplier:** 0.125F (±4.096 V range) → getMiliVolts() returns mV as uint16_t
- **RTC:** PCF8563 at I2C address 0x51
- **Single gas only:** H2. GasManager.m_gases vector is intentionally empty; calculateFlow()
  does NOT use it. ppm = (V - intercept) / slope directly.

---

## Navigation Architecture

Main menu is a horizontal CompositeMenu ("Main Menu") of 15 sub-menus (indices 0-14).

**Index map:** 0=run, 1=zero-only, 2=calzero, 3=calvalue, 4=calgas,
5=calresult(skipped), 6=alarm, 7=lowalarm, 8=loggingset,
9=loggingtime(skipped), 10=hour, 11=minute, 12=range, 13=bumptest, 14=info

The menu splits into two sections:
- **Cal section (1-5):** entered via DOWN from Run. Order is Zero-only → Cal Zero
  → Cal Value → Cal Gas → Cal Result. Zero-only is a standalone drift-correction
  re-zero; Cal Zero is the first step of the full cal sequence.
- **Settings cycle (6-14):** entered via UP from Run. Order is Alarm / Low Alarm /
  Logging Set / Logging Time / Hour / Minute / Range / Bump Test / Info.
  Bump Test sits second-from-last (just before Info) so it stays accessible
  without interrupting the main settings flow.

Bump Test (13): reads ppm with current slope/intercept, compares to Calvalue ref,
banner "BUMP OK" if within ±20% else "RECALIBRATE". No EEPROM writes.
Zero-only (1): re-runs `calibrate(rawMV)` (same as Cal Zero) for drift correction,
but the S handler returns to Run instead of advancing through the cal sequence.

**Button roles:**
- DOWN (GPIO 14): from run(0) -> setCurrentMenu(1) Zero-only;
  from zero-only(1) -> setCurrentMenu(2) Cal Zero (escalate to full cal);
  elsewhere -> ((CompositeMenu*)g_mainMenu->getCurrentMenu())->moveToNext() cycles sub-menu options
- UP (GPIO 13): always calls g_mainMenu->moveToNext() then syncSubMenuToCurrent()
- S (GPIO 19): g_mainMenu->action() then context banner

**moveToNext() guards in Menu.h (Main Menu only):**
- lands on 1 -> jump to 6  (UP from Run skips the cal section to Alarm)
- lands on 5 -> jump to 0  (UP past Cal Gas exits the cal sequence to Run)
- lands on 9 -> jump to 10  (loggingtime skipped in UP sequence)

**syncSubMenuToCurrent() in .ino:** called after every UP press; reads saved EEPROM
index for the new sub-menu and calls setCurrentMenu() so the menu opens on the active
value. Hour (10) and minute (11) always open at index 0.

**S button handler (key cases):**
- idx==1 (zero-only): banner "ZERO SET", then setCurrentMenu(0) -> return
- idx==2 (cal zero, in full cal flow): banner "ZERO SET"; advances naturally to calvalue
- idx==4 (cal gas): banner "CAL SUCCESS"/"CAL FAILED" based on wasCalibrationSuccessful(),
  then setCurrentMenu(0) -> return
- idx==10 or 11 (time): adjusts RTC directly here (not in HourSet/MinuteSet),
  then syncTimeFromRTC() -> return
- idx==13 (bump test): banner "BUMP OK"/"RECALIBRATE" from g_bumpTestItem->wasBumpTestOK(),
  then setCurrentMenu(0) -> return
- idx==5 (cal result) is unreachable in normal flow — skipped as banner

---

## Calibration

**calibrate(zero_mV)** — zero step:
- calSuccess = false
- intercept = zero_mV / 1000.0
- EEPROM.put(8, intercept)

**calibrate2(cal_mV)** — span step:
- calgas = cal_mV / 1000.0
- refPpm from Calvalue().getSelectedCalvalue() (reads EEPROM addr 80)
- FAIL if |calgas - zero| < 0.020 V (20 mV minimum)
- slope = (calgas - zero) / refPpm
- EEPROM.put(16, slope)
- calSuccess = true only on success
- saves timestamp to EEPROM addr 120 via RTC

**calculateFlow(voltage_V):**
- if slope == 0 return 0
- ppm = (voltage - intercept) / slope
- if ppm < 0 return 0

---

## EEPROM — Full Address Map

| Addr | Size | Content |
|------|------|---------|
| 4 | 4 B | Gas index (int, always 0) |
| 8 | 8 B | Intercept double |
| 16 | 8 B | Slope double |
| 72 | 1 B | Range index |
| 76 | 1 B | Alarm high index |
| 80 | 1 B | Cal value index |
| 92 | 1 B | Low alarm index |
| 96 | 1 B | Logging set index |
| 120 | 32 B | Cal timestamp string |
| 152 | 28 B | FW_BUILD string (RTC auto-set) |
| 200 | 1 B | Logging time index |
| 210 | 2 B | Manual log count |
| 220 | 4 B | Hour index |
| 230 | 4 B | Minute index |

**Precision fix:** ConfigurationManager.cpp loads slope/intercept as String(value, 10)
to avoid Arduino String(double) truncating small values like 0.00002 to "0.00".

---

## RTC / Time

**syncTimeFromRTC():** creates local RTC_PCF8563, reads now(), calls settimeofday() to
sync ESP32 system clock. Run screen uses getLocalTime() which reads that clock.

**autoSyncRTCFromBuild():** at boot, compares FW_BUILD (__DATE__+" "+__TIME__) stored at
EEPROM addr 152 to current compile string. If different -> set RTC to compile timestamp
-> save new string. Skipped on normal reboots.

**Time menu S press (in .ino, NOT in HourSet/MinuteSet):**
- Hour (idx 9): rtc.adjust(year, month, day, selectedHour, now.minute(), 0)
- Minute (idx 10): rtc.adjust(year, month, day, now.hour(), selectedMinute, 0)
- HourSet.h / MinuteSet.h only save to EEPROM — no RTC calls.

---

## Display Layout Constants

- Screen: 128x64 px, font ArialMT_Plain_10 / 16 / 24
- Footer line: y=49, footer text: y=51
- Run screen value: ArialMT_Plain_24, TEXT_ALIGN_CENTER, x=64, y=18
- All footer text must set TEXT_ALIGN_LEFT before drawing (center is set for value display)
- Standard footer: LEFT="MENU" CENTER="SET" RIGHT="OPTION"
- Cal Zero and Cal Gas: no footer
- Banner delay: 800 ms (showSavedBanner)

---

## Known Gotchas

1. **pins.h is ignored.** Always edit Globals.cpp for pin changes.
2. **m_gases is empty** in GasManager — never access m_gases[x]; calculateFlow() was
   fixed to not use it.
3. **setCurrentMenu() bug was fixed** — removed spurious `Serial.println("Menu not found")`.
4. **EEPROM.writeInt vs EEPROM.read mismatch** — all indices use writeInt (4 bytes) but
   are read back with read (1 byte). Works because values are all < 256 and ESP32 is
   little-endian (low byte first). Do not change to readInt without updating all reads.
5. **String(double) precision** — always use String(val, N) with N>=6 for calibration
   values; plain String(double) gives 2 dp and destroys small slopes.
6. **CompositeMenu cast** — DOWN handler casts getCurrentMenu() to CompositeMenu*.
   Only valid for sub-menus that ARE CompositeMenus (alarm, range, etc.).
   Cal zero/gas/result are leaf items — DOWN on them will misbehave (not currently fixed).
