# M30 CH2O firmware — change log

## 2026-11-26

Branch `claude/m30-ch2o-calibration-m8snoe` (11 commits, no PR yet; `main`
does not have these changes). Final state not yet compiled for the ESP32 or
tested on a unit. After flashing, **recalibrate (zero + span)**.

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

To restore this folder exactly as it was before today (other products
untouched): `git checkout a473a0a -- M30/M30_CH2O_VerK_20260602_beta`

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

### Known open issues
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

### Next time
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
