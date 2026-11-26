# M30-EC firmware — change log

Folder `M30/M30_EC_v<version>` — currently `M30/M30_EC_v1.0.0b2` (was
`M30/M30_EC`, before that `M30/M30_CH2O_VerK_20260602_beta`). Versioning
rules are in `BuildConfig.h` (sketch folder), section 4: MAJOR = settings layout changed
(recalibrate), MINOR = feature / new sensor, PATCH = fix, `-beta.N` = not yet
verified on hardware.

## 1.0.0-beta.2 — 2026-11-26

- Every code file starts with a PID Analyzers banner: logo, file, one-line
  description, author (Jason Wang), copyright.
- Removed unused headers: `CalManager.h`, `Gas.h`, `WebServer.h`,
  `MQTTFlashPublisher.h`, `MQTTRealTimePublisher.h`, and `pins.h` (old
  button-pin macros that disagreed with the real pins in `Globals.cpp`).
- **All settings in one file next to the `.ino`:** `inc/Version.h` merged
  into `BuildConfig.h` (section 4, "Firmware version"), and `BuildConfig.h`
  moved from `inc/` to the sketch folder. No setting changed. Entries below
  this one still say `inc/BuildConfig.h` / `inc/Version.h` (where they were then).
- Folder and sketch now carry the version: `M30/M30_EC_v1.0.0b2/
  M30_EC_v1.0.0b2.ino` (was `M30/M30_EC/M30_EC.ino`). Branch renamed to
  `claude/m30-ec-v1.0.0`.
- Fixed `SleepTimer.h` `getSelectedInterval()`: it had no `return`, so it
  returned a garbage value. Nothing calls it today (its only use is commented
  out), so no behaviour changes; it is now safe to use.

## 1.0.0-beta.1 — 2026-11-26

Branch `claude/m30-ch2o-calibration-m8snoe` (no PR yet; `main` does not have
these changes). Compiled and tested on a PC only (see below), not yet on an
ESP32 or a unit. **First boot after flashing reloads defaults: recalibrate.**

### One firmware for 21 sensors
- Renamed `M30_CH2O_VerK_20260602_beta` → **`M30_EC`** (`M30_EC.ino`); history
  kept. Non-M30 profiles removed (NDIR CO2 ×2, PID ×3, TCD H2, EC 4-in-1).
- **Choose the sensor with one line** in `inc/BuildConfig.h`:
  `#define SENSOR_PROFILE EC_CH2O_50PPM`. 21 profiles: C2H4 100, CH2O 5/50,
  CH3SH 10, CO 300/1000, Cl2 20, ETO 100, H2S 50/100/2000, HCN 100, HCl 10,
  HF 10, LEL 50 %, NH3 100, NO2 20, O2 30 %, O3 10, PH3 10, SO2 20. A wrong
  name stops the build and lists the valid ones.
- Each profile now also holds: signal direction, nominal zero (mV) and
  sensitivity (mV/ppm), alarm directions/titles, ambient value, demo range.
  **Nominal zero/sensitivity are `SENSOR_TBD` for all 21** — fill them from
  datasheets or a good bench calibration (Info screen Zero / Sens).
- Run screen uses each sensor's own decimals (CH2O now shows 0.01 ppm in PROD
  and DEV too). DEMO shows a range per sensor (CH2O 50: 0.00–0.30 ppm as
  before) and its "DEMO" tag moved to the footer ("CH3SH DEMO" did not fit).
- `EC_TEMPLATE.h` rewritten as a checklist for adding sensor no. 22+.

### Version system
- `inc/Version.h`: **1.0.0-beta.1**. Boot `M30-EC v1.0.0-b1 PROD`, Info
  `FW: v1.0.0-b1 PROD`, serial/Bluetooth
  `M30-EC v1.0.0-beta.1 PROD EC_CH2O_50PPM board VerK (2026-11-26)`.
- Build mode names are now PROD / DEV / DEMO ("VerK" is the board revision).
- EEPROM stores the settings-layout version and the sensor profile ID: a
  layout change or a firmware built for a different sensor reloads defaults
  (CAL?) instead of applying another sensor's calibration. Units from before
  1.0.0 reseed once.

### Safety
- **O2 alarms fixed.** Both O2 alarm lists alarmed only when the reading went
  above the setpoint, so 19.5 % alarmed constantly in normal air and never on
  low oxygen. Now "Alarm : High" = 23.5 % enrichment (above),
  "Alarm : Low" = 19.5 % deficiency (below).
- **A dead/disconnected sensor no longer passes calibration** (it used to
  pass with a reversed slope and then read 0.0 with no alarms). Span and zero
  are rejected when the detector is faulted; span is rejected when the output
  moves the wrong way (sensors with known direction) or, once nominal values
  are filled in, when sensitivity is < 30 % or > 300 % of nominal. Still
  possible on **O2 and LEL** until their direction is confirmed (see below).
- Span is measured against the **stored zero**; skipping Cal Zero used to
  compare the span against 0 V. A span with no zero ever stored is refused.
- CAL FAILED / ZERO FAILED now say why: `Check gas & sensor`, `Sensor fault`,
  `Wrong direction`, `Do Cal Zero first`, `Sensitivity too low/high`.

### DEV mode
- Simulator follows each profile (direction, range, O2 at 20.9 % with
  low-oxygen dips). New serial commands `SIM NOGAS` / `SIM GAS` (operator
  forgot the span gas → expect CAL FAILED, Check gas & sensor).

### How it was checked (on a PC, not on the unit)
- `M30_EC.ino` and `MenuRenderer.cpp` compile for all 21 sensors × 3 modes
  (63 builds) against stand-in hardware libraries.
- Per sensor in DEV: zero → span passes and a held concentration reads back
  within 1–3 %; no-gas span fails "Check gas & sensor"; dead cell fails
  "Sensor fault" (except O2, LEL); wrong-way span fails; no alarm in clean
  air; O2 low alarm trips below 19.5 %. DEMO values stay in range at the
  display resolution.
- Every header, tag and title measured with the display font (all fit).

### Known open issues
- **Signal direction not confirmed on this board** for HCl, HF, O2, LEL (set
  to 0 = no check); the 17 others are set from chemistry and marked "verify".
  If a correct span shows "Wrong direction", flip that profile's
  `SENSOR_SIGNAL_POLARITY`.
- **O2 and LEL can still accept a dead cell** (0 mV is allowed for them and
  direction is unknown). Confirm their direction / zero offset to close this.
- Nominal sensitivity is TBD everywhere: sensitivity check and realistic
  per-sensor DEV simulation are off until filled in.
- H2S 2000 ppm: over-range (display cap) is 1100 ppm, below its 2000 ppm full
  scale — readings above 1100 show OVER. Unchanged; confirm it is intended.
- ~~`SleepTimer.h`: `getSelectedInterval()` has no `return`.~~ Fixed in beta.2.
- Still open from 0.9.0: PEL is not an 8-hour TWA; missing-sensor input floats
  (~305 mV → CAL?, not FAULT); whole-mV resolution; other product builds
  still have the June EEPROM/ADC bugs.

### Next steps
1. Flash in **DEV** for one sensor; run Zero → Span → `SIM PPM`, `SIM NOGAS`,
   `SIM FAULT`; power-cycle; then flash with a *different* `SENSOR_PROFILE`
   and confirm it shows CAL?.
2. With real sensors: confirm direction (and fill nominal zero / sensitivity)
   per profile, starting with the ones you will ship.
3. When verified on hardware: set `FW_PRERELEASE 0` → **1.0.0**, tag
   `m30-ec-v1.0.0`, open the PR into `main`.

---

## 0.9.0 — 2026-11-26 (CH2O-only build)

Then folder `M30_CH2O_VerK_20260602_beta`.

### Safety fixes
- **Alarm setpoints below 1 ppm work again.** High alarm 0.75 ppm and all PEL
  options (0.1 / 0.75 ppm) were rounded to 0 = OFF, so those alarms could never
  trigger. The cal-gas value had the same rounding (0.5 ppm → 0 on other
  profiles). `be37e13`
- **Saved hour/minute are range-checked**, so a brand-new board no longer reads
  past the end of the hour/minute tables. `aa84038`

### Display
- Main reading shows **FAULT** (sensor reads 0 mV or ADC maxed out), **CAL?**
  (not calibrated) or **OVER** (above range) instead of `xxx`. FAULT and CAL?
  do not trigger gas alarms; OVER does. `5f61d92`
- Calibration wording: `DOWN: Start Cal` / `UP: Exit`,
  `Wait for stable, press S`. After the span step the unit shows
  **CAL PASSED**, or **CAL FAILED** + "Check gas & sensor" (it used to say
  CAL COMPLETE even on failure). `80b4612`
- Boot check typo `ADC1115` → `ADS1115`. `80b4612`
- Settings footer `MENU · SET · OPTION` → **SKIP · SAVE · ▼**; Bump Test shows
  **SKIP · TEST**. `bcfe6e9`
- Info screen fits the display (version line was clipped) and shows
  **Zero (mV)** and **Sens (mV/ppm)** instead of Int/Slp. Boot screen version
  line fixed too. `c11b585`

### Buttons
- **Hold DOWN to scroll** on the Hour / Minute screens: 4 steps/s, then
  8 steps/s after 2.5 s held (hour lap ~4.4 s, 0→59 minutes ~8.9 s). Releasing
  adds no extra step; the value is still saved only with S. Speed is tunable
  via `HOLD_SCROLL_*` in `inc/BuildConfig.h`. `a9dd3cb`, slowed in `47d9a03`
- DOWN / S / RIGHT accept any 30–800 ms press (was 110–310 ms; quick taps were
  ignored). The ON (power) button is unchanged. `154190e`
- **Any key (DOWN / S / RIGHT) exits the Info page.** `c11b585`

### Build modes — `inc/BuildConfig.h`
- One codebase, one line to switch: **PROD** (shipped), **DEV** (bench without
  a sensor: simulated sensor, raw mV on main screen, `SIM ...` serial
  commands), **DEMO** (random 0.00–0.30 ppm). The separate
  `demo_M30_CH2O_VerK_20260602` folder was removed (now DEMO mode). `c44bcc4`

### Rolling back
| Change | Switch in `inc/BuildConfig.h` (set to `0`) | Or revert |
|---|---|---|
| Wider click window | `FEATURE_WIDE_CLICK_WINDOW` | `154190e` |
| Hold to scroll | `FEATURE_HOLD_TO_SCROLL` | `47d9a03`, then `a9dd3cb` |
| New footer labels | `FEATURE_NEW_FOOTER_LABELS` | `bcfe6e9` |

To restore the CH2O build exactly as it was before 0.9.0 (other products
untouched; it comes back under its old folder name):
`git checkout a473a0a -- M30/M30_CH2O_VerK_20260602_beta`

### How it was checked (on a PC, not on the unit)
- Alarm/PEL/cal-value fix: the changed headers compiled with stand-in hardware
  libraries; 0.75 ppm and 0.1 ppm setpoints kept and trip correctly.
- FAULT / CAL? / OVER thresholds against the real CH2O profile and gain.
- DEV simulator: power-on → zero → span → 30 min field use, 50 random runs,
  all passed (cal passes, reading within 0.25 ppm + 3 %, alarms and OVER
  trigger, no false FAULT).
- Buttons: all four on/off switch combinations; scroll speed steady with a
  15–45 ms main loop; Run screen never repeats; ON button timing unchanged.
- Screen text widths measured with the display library's real font; Info
  screen rendered before/after (before matched the photo of the clipped line).

### Known open issues (as of 0.9.0 — see 1.0.0-beta.1 for status)
- A dead/disconnected sensor can **pass span calibration** (voltage drops
  instead of rising); the unit then reads 0.0 and never alarms.
- DEV mode cannot yet simulate a failed calibration (no `SIM NOGAS`).
- "PEL" alarm compares the live reading, not an 8-hour TWA.
- With no sensor connected the input floats at ~305 mV, so a missing sensor
  shows CAL?, not FAULT. Needs a hardware pull-down or a known clean-air range.
- Readings use whole millivolts (~0.04 ppm per mV on this profile), so a low
  span (e.g. 5 ppm) can carry 1–2 % gain error.
- The other builds (M10, M12, M20, other M30s, M40, M103) still have the
  EEPROM-persistence and ADC-scaling bugs fixed in June for this build.

### Next steps (as of 0.9.0)
1. Flash the final branch in **DEV** mode; run zero → span → `SIM PPM 5`
   (expect ~5.0), alarm at 0.75 ppm, `SIM PPM 60` (OVER), `SIM FAULT`;
   power-cycle and confirm the calibration stays.
2. Try hold-to-scroll and quick taps; if one press ever steps twice, raise the
   30 ms minimum or set `FEATURE_WIDE_CLICK_WINDOW` to 0.
3. Flash **PROD** and confirm the Run screen hides the mV and the Info screen
   fits.
4. Decide: fix the dead-sensor span pass + add `SIM NOGAS`; PEL rename or
   real TWA; remaining wording (`ZERO SET`, `RECALIBRATE`, alarm menu titles,
   `TIMER SLEEP`, `Set User ID`, `Alarm OFF` / `LOG-A`).
5. Open a PR into `main` once it is verified on the unit.
