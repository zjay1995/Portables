# M103 Bench Test Procedure
**Firmware:** M103_board_20260504_beta 07/08/26 &nbsp;|&nbsp; **Scope:** full functional check after the 07/08/26 update

Work through the tests in order — later tests assume earlier ones passed.
Check the box when a test passes; note anything unexpected in the margin.

## 0. Setup

**Equipment**
- M103 unit, fully assembled (PID sensor, UV lamp, alarm buzzer/relay wired to IO 15)
- Zero gas (clean air) and a span gas bottle of known concentration (e.g. 100 ppm isobutylene)
- PC with Arduino IDE 1.8.x and a USB cable; serial monitor at **115200 baud**
- Phone or PC with Bluetooth Classic (SPP) for the log-retrieval test
- Multimeter (for the alarm-output test)

**Flash**
- Board: ESP32 Dev Module · Flash: 4MB No OTA (2MB APP/2MB SPIFFS) · CPU 240 MHz · Upload 921600
- Compile and upload `M103_board_20260504_beta.ino`. The sketch must compile with **zero errors**.

> **IMPORTANT — recalibration required.** This firmware changes the ADC gain
> setting and fixes a bug that corrupted stored calibration factors on
> reboot. Any calibration performed under older firmware is invalid.
> Perform T9 (calibration) before trusting any reading.

---

## T1 — Boot sequence — [ ]

1. Power the unit on.
2. **Expect:** splash screen — red "PID / ANALYZERS" logo, "Single Gas PID Analyzer",
   `M103`, and version **07/08/26** in yellow. Holds ~2 s.
3. **Expect:** SYSTEM CHECK screen with five rows, each `[ OK ]` in green:
   `RTC PCF8563 · ADC ADS1115 · BAT INA219 · RHT SHT4x · FLASH SPIFFS`,
   then "ALL SYSTEMS OK" (2 s). A `[FAIL]` row holds 4 s and indicates a
   hardware/wiring problem — stop and fix before continuing.
4. **Expect:** serial monitor prints `PID M103 07/08/26`, `EEPROM begin!!`,
   and a series of `config: ...` lines with no garbage values.

## T2 — Run screen — [ ]

1. After boot the run screen appears.
2. **Expect:** top bar shows time (HH:MM), "PID M103", battery icon with a
   sane percentage; reading panel shows a live value in yellow with the unit
   label; ALARM SETPOINTS panel shows HIGH / PEL values; STATUS panel shows
   RANGE (e.g. `5000 ppm`) and SIGNAL in mV; footer shows `ALARM: ON/OFF`
   and `LOG: --`.
3. **Expect:** the mV value now reads the true signal level (older firmware
   showed it ~33% low). Verify against a multimeter on the PID amp output
   if available.

## T3 — Menu navigation — [ ]

1. Press DOWN repeatedly from the run screen.
2. **Expect** this exact sequence, then wrap to run:
   `Gas Library → Range → High Alarm → PEL Alarm → Logging Set → Logging
   Interval → Set Hour → Set Minute → Display Unit → Auto Power-Off →
   Cal Factors → (run)`.
3. On any settings screen: RIGHT cycles values, S saves (green "VALUE SAVED"
   banner) and advances to the next screen.

## T4 — Range page (new) — [ ]

1. DOWN ×2 to Range. RIGHT toggles `5000 ppm` / `10000 ppm`. S to save.
2. **Expect:** run screen STATUS panel immediately shows the new range.
3. Power-cycle. **Expect:** the selected range is retained.

## T5 — Alarms (display + relay/buzzer output) — [ ]

1. Set High Alarm to a value below the current reading (or use span gas to
   raise the reading above the setpoint).
2. **Expect on run screen:** reading turns red, chip shows `HIGH` red,
   the HIGH setpoint cell turns red.
3. **Expect on hardware (new — the output never fired on older firmware):**
   IO 15 goes HIGH (~3.3 V on the meter) and the buzzer/relay activates.
   This works even while on a settings screen.
4. Remove gas / raise the setpoint. **Expect:** IO 15 returns LOW and the
   display shows `NORMAL` green.
5. Repeat briefly for PEL alarm (chip shows `PEL` yellow; IO 15 also drives).

## T6 — Data logging (new — was dead) — [ ]

**Manual:** Logging Set → `Manual` (S). On the run screen press S three times.
**Expect:** footer shows `LOG: M 1 … 2 … 3` incrementing.

**Auto:** Logging Set → `Auto`; Logging Interval → `10 sec`. Wait ~1 min on the
run screen. **Expect:** footer shows `LOG: AUTO`.

**Retrieve:** pair Bluetooth with `ESP32 PID M103`, send `PID73*` (with newline).
**Expect:** CSV lines like `20260708_143012,MANUAL,42.0` and `...,AUTO,...`
between Start/End markers. Timestamps must be current, values plausible.

**Clear:** send `CLEARLOG` (or select `Clear Log` in the menu).
**Expect:** log empties, `LOG: M` counter resets to 0.

## T7 — Display unit — [ ]

1. Display Unit → `ppb`, save. **Expect:** run reading ×1000 with `ppb` label, immediately.
2. Return to `ppm`. Power-cycle. **Expect:** unit retained.

## T8 — Auto power-off (new — was fixed at 30 min) — [ ]

1. Auto Power-Off → `5 min`, save. Leave the unit untouched 5 minutes.
2. **Expect:** unit enters deep sleep (screen/power off). Serial prints
   `SleepTimer: idle timeout -- deep sleep`.
3. Wake with the power switch (IO 12). **Expect:** normal boot.
4. Set `Continuous`. **Expect:** unit stays on well past any timeout.
5. Power-cycle. **Expect:** setting retained.

## T9 — Calibration (CRITICAL) — [ ]

1. From the run screen press RIGHT. **Expect:** "STEP 1/2 – ZERO".
2. Apply zero gas. **Expect:** live mV shown; `[ STABILIZING... ]` for at
   least ~4 s, then `[ STABLE ]` only once the signal truly settles
   (older firmware showed STABLE almost instantly — that was fake).
3. Press S. **Expect:** "VALUE SAVED" → Cal Gas Value page. RIGHT to select
   the bottle concentration, S.
4. **Expect:** "STEP 2/2 – SPAN" with the selected ppm. Apply span gas, wait
   for `[ STABLE ]`, press S.
5. **Expect:** "CAL COMPLETE" → Cal Result **Success** (green). Press S →
   "CAL FINISHED" → run screen.
6. **Verify reading:** with span gas still flowing, the run screen reads the
   bottle value ±10%. With zero gas it reads ~0.
7. **Failure path:** repeat a calibration but leave zero gas on during span
   capture. **Expect:** Cal Result **Failed** (red), serial prints
   "Calibration rejected", and the previous good calibration is unchanged.
8. **Abort path:** enter cal, press DOWN. **Expect:** immediate return to run.

## T10 — Calibration persistence across power-off (CRITICAL) — [ ]

*This is the regression test for the bug that zeroed calibrations on reboot.*

1. After a successful T9, open Cal Factors (DOWN to Cal Info). Write down
   ZERO (mV) and SLOPE (6 decimals).
2. Power the unit fully off (power switch, or pull power). Wait 10 s. Power on.
3. Open Cal Factors again. **Expect:** ZERO and SLOPE **identical** to the
   values written down (all 6 slope decimals), the cal date/time is correct,
   and `FW Version: 07/08/26` and the gas name are shown.
4. **Expect:** run-screen reading with span gas still matches T9 step 6.

## T11 — Clock and time backup — [ ]

1. Set Hour and Set Minute to the current time, save each.
2. Run screen shows the new time; power-cycle → time still correct (RTC).
3. *If the PCF8563 has no backup battery:* power off for >1 min, power on.
   **Expect:** serial prints `TimeSync: RTC invalid — restored from EEPROM`
   and the clock resumes near the set time (was impossible on older
   firmware — the backup was never written).

## T12 — Fresh-board sanity (optional, destructive) — [ ]

*Only on a spare/dev unit — this erases all settings and calibration.*

1. Erase flash (`esptool erase_flash` or IDE "Erase all flash").
2. Reflash and boot. **Expect:** clean boot, no crash/reboot loop; defaults:
   range 5000 ppm, alarms Off, logging Off, unit ppm, auto power-off 2 h,
   log counter 0, Cal Info shows "Not calibrated".
3. Perform T9 — **Expect:** calibration completes normally.

---

## Sign-off

| Item | Value |
|---|---|
| Unit serial # | |
| Tester | |
| Date | |
| Firmware verified on splash | 07/08/26 |
| Span gas used (gas / ppm / bottle #) | |
| All tests passed | YES / NO |

**Known limitations (expected, not failures):**
- PID_GAIN (IO 17) is intentionally not driven; Range affects only the
  over-range threshold and display.
- The 103_Demo build (HUZZAH32) uses a synthetic sine-wave reading and a
  fixed 78% battery — do not use it for measurement tests.
