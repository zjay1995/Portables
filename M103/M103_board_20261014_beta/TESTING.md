# M103 Test Procedure — FW 10/14/26 Jason Wang

Run in order. Check each box on pass.

**Need:** unit, zero gas, span gas (known ppm), USB + serial monitor (115200), Bluetooth phone/PC, multimeter.

**Flash:** ESP32 Dev Module, 4MB No OTA, 240 MHz. Must compile clean.

> **Recalibrate after flashing.** Old calibrations are invalid on this version.

## T1 — Boot [ ]
- Splash shows logo + `M103` + `10/14/26`.
- SYSTEM CHECK: all 5 rows `[ OK ]`, then "ALL SYSTEMS OK".
- Serial prints `PID M103 10/14/26`.

## T2 — Run screen [ ]
- Time, battery %, live reading, HIGH/PEL setpoints, RANGE, SIGNAL mV all shown.
- mV matches meter on amp output (was ~33% low before).

## T3 — Menu [ ]
- DOWN cycles: Library → Range → High Alarm → PEL → Log Set → Log Interval → Hour → Minute → Unit → Power-Off → Cal Info → run.
- RIGHT cycles values. S saves ("VALUE SAVED") and advances.

## T4 — Range [ ]
- Set 10000 ppm. Run screen updates.
- Power cycle. Setting kept.

## T5 — Alarm [ ]
- Set High Alarm below reading (or apply gas).
- Display: reading red, chip `HIGH`.
- IO 15 goes HIGH (~3.3 V), buzzer/relay on.
- Clear condition. IO 15 LOW, chip `NORMAL`.
- Repeat once for PEL.

## T6 — Logging [ ]
- Manual mode: S on run screen 3×. Footer `LOG: M 1…2…3`.
- Auto mode, 10 s interval: wait 1 min. Footer `LOG: AUTO`.
- Bluetooth `PID73*`: prints CSV lines with correct timestamps.
- `CLEARLOG`: log empty, counter 0.

## T7 — Unit [ ]
- ppb: reading ×1000, label `ppb`.
- Back to ppm. Power cycle. Kept.

## T8 — Auto power-off [ ]
- Set 5 min. Leave idle. Unit sleeps at 5 min.
- Power switch wakes it.
- Set Continuous. Stays on.

## T9 — Calibration [ ]
- RIGHT from run → ZERO. Apply zero gas.
- Zero: `[ STABLE ]` only after signal holds steady 20 s (not instant). Span: 4 s.
- S → select bottle ppm → S → SPAN. Apply span gas, wait `[ STABLE ]`, S.
- Result: **Success**.
- Reading = bottle value ±10% on span, ~0 on zero.
- Fail path: span capture with zero gas → **Failed**, old cal kept.
- DOWN anywhere in cal → back to run.

## T10 — Cal survives power-off [ ]
- Note ZERO and SLOPE on Cal Info.
- Power off 10 s. Power on.
- Values identical (all slope decimals). Date and FW version correct.

## T11 — Clock [ ]
- Set hour/minute. Power cycle. Time correct.
- No RTC battery: power off >1 min → serial `restored from EEPROM`, time near correct.

## T12 — Fresh board (optional, erases everything) [ ]
- Erase flash, reflash. Boots clean, defaults sane, Cal Info "Not calibrated".
- T9 works.

---

| Unit # | Tester | Date | Span gas | Pass |
|---|---|---|---|---|
| | | | | YES / NO |

Notes: IO 17 (gain) intentionally not driven — Range is display/threshold only. 103_Demo build is synthetic; don't use for measurement tests.
