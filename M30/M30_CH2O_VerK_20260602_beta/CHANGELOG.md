# M30 CH2O firmware — change log

## 2026-11-26

Branch `claude/m30-ch2o-calibration-m8snoe`. Not yet compiled for the ESP32
or tested on a unit. After flashing, **recalibrate (zero + span)**.

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
- **Hold DOWN to scroll** on the Hour / Minute screens. `a9dd3cb`; slowed down
  afterwards to 4 steps/s, then 8 steps/s after 2.5 s held (hour lap ~4.4 s,
  0→59 minutes ~8.9 s). Speed is tunable via `HOLD_SCROLL_*` in
  `inc/BuildConfig.h`.
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
| Hold to scroll | `FEATURE_HOLD_TO_SCROLL` | `a9dd3cb` |
| New footer labels | `FEATURE_NEW_FOOTER_LABELS` | `bcfe6e9` |

To restore this folder exactly as it was before today (other products
untouched): `git checkout a473a0a -- M30/M30_CH2O_VerK_20260602_beta`

### Known open issues
- A dead/disconnected sensor can **pass span calibration** (voltage drops
  instead of rising); the unit then reads 0.0 and never alarms.
- DEV mode cannot yet simulate a failed calibration (no `SIM NOGAS`).
- "PEL" alarm compares the live reading, not an 8-hour TWA.
