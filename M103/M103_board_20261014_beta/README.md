# M103 Portable PID Analyzer - Jason Wang
**FW:** 07/08/26 · **MCU:** ESP32-WROOM-32E · **Display:** 3.5" 320×480 TFT (ILI9488)

Single-gas PID (VOC) analyzer. SPIFFS logging, Bluetooth log retrieval, 3-button keypad + power switch.

**Build:** Arduino IDE, ESP32 Dev Module, 4MB No OTA (2MB APP/2MB SPIFFS), 240 MHz. See `TESTING.md` for the bench procedure.

## Pins

| Net | GPIO | Dir | Notes |
|---|---|---|---|
| Button DOWN | 13 | in (pullup) | advance menu |
| Button S | 21 | in (pullup) | save / confirm |
| Button RIGHT | 14 | in (pullup) | cycle value / enter cal |
| Power switch | 12 | in / EXT0 | deep-sleep wake |
| ALARM | 15 | out | buzzer/relay, active-high |
| LAMPOUT | 16 | out | UV lamp, active-high |
| PID_GAIN | 17 | — | reserved, not driven |
| HVON_STATUS | 36 | in only | HV feedback |
| VBATT_ADC | 34 | ADC | 100K/100K divider |
| POWER HOLD | 25 | out | active-high |
| I2C SDA / SCL | 23 / 22 | — | non-default pins — always `Wire.begin(23, 22)` |

**I2C devices:** PCF8563 RTC @0x51 · ADS1115 @0x48 (PID signal, A0, GAIN_ONE, 0.125 mV/bit) · INA219 @0x40 (battery; IO 34 fallback) · SHT4x @0x44 (T/RH).

**Battery scale:** 3.4 V empty → 4.05 V full.

## EEPROM Map

512 bytes (`EEPROM_SIZE` in `inc/ConfigurationManager.h`) — must exceed the highest address or writes are silently dropped.

| Addr | Size | Content |
|---|---|---|
| 0 | 4 | Auto power-off index (0=5m 1=1h 2=2h 3=6h 4=cont) |
| 4 | 4 | Gas index |
| 8–71 | 8×8 | Intercept/slope doubles, gas 1–4 (8,16 / 24,32 / 40,48 / 56,64) |
| 72 | 4 | Range index |
| 76 | 4 | High alarm index |
| 80 | 4 | Cal gas value index |
| 92 | 4 | Low alarm index |
| 96 | 4 | Logging mode index |
| 100 | 1 | Display unit (0=ppm 1=ppb 2=1117) |
| 120 | ~20 | Cal timestamp string |
| 200 | 4 | Logging interval index |
| 210 | 2 | Manual log counter (ushort) |
| 220 / 230 | 4 | Hour / minute setting |
| 240–244 | 5 | Time backup: yr−2000, mo, day, hr, min |

## Menu

```
 0 Run   1 Gas Library   2 Range   3 High Alarm   4 PEL Alarm
 5 Log Mode   6 Log Interval   7 Hour   8 Minute   9 Unit
10 Auto Power-Off   11 Cal Info (view only)
—— navLimit 12 ——
12 Cal Zero   13 Cal Gas Value   14 Cal Span   15 Cal Result
```

| Button | Run (0) | Settings (1–11) | Cal (12–15) |
|---|---|---|---|
| DOWN | — | next; wraps 11→0 | abort → run |
| S | manual log | save + advance | save + advance |
| RIGHT | enter cal (12) | cycle value | cycle value |

Banners: `VALUE SAVED` (settings, cal 12–13) · `CAL COMPLETE` (14) · `CAL FINISHED` (15) · none on run/Cal Info.

## Calibration

RIGHT from run → Zero (apply zero gas, wait `[ STABLE ]`, S) → Cal Gas Value (pick ppm, S) → Span (apply gas, `[ STABLE ]`, S) → Result Success/Failed → run.

`slope = (V_span − V_zero) / (ppm/1000)`. Rejected (Failed, EEPROM unchanged) if slope ≤ 0.00001 or ppm = 0. On success the timestamp is saved; Cal Info shows date, zero mV, slope, FW version, gas.

## Time

- PCF8563 → ESP32 clock at boot; year ≥ 2024 = valid.
- Every valid sync and every hour/minute save mirrors to EEPROM 240–244; if the RTC loses power, time restores from that backup.
- For true battery-backed time, fit a CR2032/supercap on PCF8563 VBAT.

## Logging & Bluetooth

- Modes: Off / Auto (1 s–1 h interval) / Manual (S on run screen) / Clear Log.
- CSV rows `timestamp,TAG,ppm` in SPIFFS `/log.txt`.
- BT Classic, device `ESP32 PID M103`: `PID73*` dumps the log, `CLEARLOG` erases it (both also work over serial).
