# M30 Sensor Configuration Reference

This document describes every parameter that changes when you swap sensors on the M30 platform.
It covers PID and electrochemical (EC) sensor types.

---

## Sensor catalog

Full menu of gas sensors supported (or supportable) by the M30 platform.
Per-sensor profile headers live under `inc/sensors/`. "Hardware unit" is the
physical box-level part that ships with the sensor pre-installed.

| Gas      | Range            | Resolution | Over-range¹ | Hardware | Special SW notes              |
|----------|------------------|-----------:|------------:|----------|-------------------------------|
| NH3      | 0–100 ppm        | 0.1        | 110         | M30      | —                             |
| H2S      | 0–100 ppm        | 0.1        | 110         | M30      | —                             |
| H2S      | 0–2,000 ppm      | 0.1        | 1,100²      | M30      | Special range                 |
| CO       | 0–1,000 ppm      | 0.1        | 1,100       | M30      | —                             |
| CH2O     | 0–5.00 ppm       | 0.01       | 5.5         | M30      | —                             |
| CH2O     | 0–50.00 ppm      | 0.01       | 55          | M30      | —                             |
| O2       | 0–30 %           | 0.01 %     | 33          | M30      | Special zero + span (air = 20.9 %) |
| C2H4     | 0–100 ppm        | 0.1        | 110         | M30      | —                             |
| O3       | 0–10 ppm         | 0.1        | 11          | M30      | —                             |
| SO2      | 0–20 ppm         | 0.1        | 22          | M30      | —                             |
| NO2      | 0–20 ppm         | 0.1        | 22          | M30      | —                             |
| HCl      | 0–10 ppm         | 0.1        | 11          | M30      | —                             |
| HCN      | 0–100 ppm        | 0.1        | 110         | M30      | —                             |
| Cl2      | 0–20 ppm         | 0.1        | 22          | M30      | —                             |
| HF       | 0–10 ppm         | 0.1        | 11          | M30      | —                             |
| ETO      | 0–100 ppm        | 0.1        | 110         | M30      | —                             |
| PH3      | 0–10 ppm         | 0.01       | 11          | M30      | Special SW (low-conc EC)      |
| 4-in-1   | ppm + %          | 0.1 / 0.01 % | per gas   | M30      | Composite CO + H2S + O2 + LEL |
| LEL      | 0–50 % LEL       | 0.01 %     | 55          | M30      | —                             |
| CH3SH    | 0–10 ppm         | 0.01       | 11          | M40      | —                             |
| CO2      | 0–5,000 ppm      | 0.1        | 5,500       | M12      | NDIR — split range option     |
| CO2      | 0–10,000 ppm     | 0.1        | 11,000      | M12      | NDIR                          |
| CVOC     | 0–500 ppm        | 0.10 %     | 550         | M30      | Special SW (chlorinated VOC)  |
| CVOC     | 0–10,000 ppm     | 0.10 %     | 11,000      | M30      | Special SW                    |
| TCD (H2) | 0–1 %            | LEL / ppm  | 1.1 %       | M10      | Thermal-conductivity, H2 only |

¹ "Over-range" = the displayed-but-flagged ceiling = full-scale × 1.10. Above this
  the run screen shows `OVR` instead of a number.
² 2,000 ppm H2S range over-range is capped at 1,100 ppm in firmware (display
  range, not sensor saturation — the sensor itself reads to full-scale).

When picking a profile from the table above, the editable parameters are:
- `SENSOR_RANGE_FS`    full-scale value used for over-range clipping
- `SENSOR_RES_DIGITS`  decimal digits shown on the run screen (`0` for 1-ppm steps, `1` for 0.1, `2` for 0.01)
- `SENSOR_UNIT_LABEL`  "ppm" / "ppb" / "%LEL" / "% vol" / "%"
- `SENSOR_OVERRANGE`   value above which `"OVR"` displays (typically FS × 1.1)

These four are read by the run-screen renderer; the alarm / range / cal-value
menus stay in the .ino as before so each unit can pick its own ladder of
threshold values.

---

## Quick-swap checklist

| Step | File | What to change |
|------|------|----------------|
| 1 | `SensorConfig.h` | Uncomment the correct `#include` line |
| 2 | `.ino` — gas list | `g_gasManager.addGas(...)` calls |
| 3 | `.ino` — LIBRARY menu | `GasMenuItem(...)` calls |
| 4 | `.ino` — Range menu | `RangeMenuItem(...)` calls |
| 5 | `.ino` — Alarm menu | `m_alarmArray` values |
| 6 | `.ino` — PEL/Low alarm menu | Loop values |
| 7 | `.ino` — Calvalue menu | Loop or array values |
| 8 | `inc/RangeSet.h` | `m_rangeArray` values |
| 9 | `inc/AlarmSet.h` | `m_alarmArray` values |
| 10 | `inc/LowAlarmSet.h` | `m_lowalarmArray` values |
| 11 | `inc/CalvalueSet.h` | `m_calvalueArray` values |

Steps 1–7 are in the `.ino`.
Steps 8–11 update the backing arrays (must match the menu entries in the `.ino`).

The following are **automatic** (sourced from `SensorConfig.h`, no manual edit needed):
- GasManager constructor (intercept, slope × 4 slots)
- Bluetooth device name
- ADS1115 gain

---

## Parameter reference

### GasManager constructor

```cpp
GasManager g_gasManager(
    intercept,   slope,    // Gas slot 1 — primary range
    zero, calgas,          // working variables (leave as 0, 0)
    intercept2,  slope2,   // Gas slot 2
    intercept3,  slope3,   // Gas slot 3
    intercept4,  slope4    // Gas slot 4
);
```

| Parameter | Meaning | Typical range |
|-----------|---------|---------------|
| `intercept` | Sensor voltage at zero gas (V) | −5 V to +5 V |
| `slope` | Change in voltage per unit concentration (V/ppm or V/SLM) | 0.001 to 10 |

> **Rule:** slope must never be 0 — it is the denominator in `calculateSLM()`.
> Set it to `1.0` as the safe boot default; field calibration will overwrite it.

After `calibrate()` (zero) and `calibrate2()` (span), the real values are saved to EEPROM
at addresses 8 (intercept) and 16 (slope) per slot (stride 16).

---

### ADS1115 Gain

The ADS1115 gain determines the full-scale voltage range and resolution per LSB.

| Constant | Full-scale | Resolution | Use when |
|----------|------------|------------|----------|
| `GAIN_TWOTHIRDS` | ±6.144 V | 0.1875 mV/LSB | Very wide range, low-noise env |
| `GAIN_ONE` | ±4.096 V | 0.1250 mV/LSB | **PID sensors (default M30)** |
| `GAIN_TWO` | ±2.048 V | 0.0625 mV/LSB | EC sensors, amp output 1–2 V |
| `GAIN_FOUR` | ±1.024 V | 0.0313 mV/LSB | **EC sensors, small output (<1 V)** |
| `GAIN_EIGHT` | ±0.512 V | 0.0156 mV/LSB | Very high-gain TIA, µV-level signals |

> **Warning:** If the sensor output exceeds the selected full-scale, readings will saturate
> (clip to max). Always verify the TIA output voltage at full-scale concentration
> before choosing a gain setting.

---

### Range menu (`RangeSet.h`)

```cpp
std::array<int, N> m_rangeArray{{ R1, R2, ... }};
```

These values are the full-scale values shown on the "over-range" indicator and logged in
the data file. The run screen displays "xxx" when `sensorValue > range`.

Must match the `RangeMenuItem(...)` entries in the `.ino`.

---

### Alarm (high) menu (`AlarmSet.h`)

```cpp
std::array<int, N> m_alarmArray{{ 0, v1, v2, ... }};
```

First entry must be `0` (= Off). Values represent the concentration threshold (ppm)
above which the buzzer activates and the display flashes "ALARM".

---

### PEL / low alarm menu (`LowAlarmSet.h`)

```cpp
std::array<int, N> m_lowalarmArray{{ 0, v1, v2, ... }};
```

First entry must be `0` (= Off). Intended for a sustained low-level exposure limit
(OSHA PEL or NIOSH REL). Alarm activates when `sensorValue > lowalarm`.

---

### Calvalue menu (`CalvalueSet.h`)

```cpp
std::array<int, N> m_calvalueArray{{ v1, v2, ... }};
```

These are the available **certified reference gas concentrations** the user can choose
during the span calibration step. They should match the standard cylinders you actually
have on hand. The selected value is read by `GasManager::calibrate2()` to compute slope.

---

## Sensor profiles

### PID — Dual Range 100 / 1000 ppm  *(current M30)*

| Parameter | Value |
|-----------|-------|
| Profile file | `inc/sensors/PID_M10_100_1000.h` |
| BT name | `ESP32 PID M10` |
| ADS gain | `GAIN_ONE` |
| Intercept (slot 1) | −2.054456771 |
| Slope (slot 1) | 1.0 (factory default; EEPROM overwrites after cal) |
| Intercept (slot 2) | 0.0 |
| Slope (slot 2) | 1.0 |
| Gas library | "100", "1000" |
| Range options | 5000 ppm, 10000 ppm |
| Calvalue options | 10–100 ppm (step 10), 200–500 ppm (step 100) |
| High alarm | 0–1000 ppm (0=Off, step 10 up to 500, step 100 to 1000) |
| PEL alarm | 0–60 ppm (step 1) |

---

### EC — CO (Carbon Monoxide), 0–300 ppm

| Parameter | Value |
|-----------|-------|
| Profile file | `inc/sensors/EC_CO_300ppm.h` |
| BT name | `ESP32 EC-CO` |
| ADS gain | `GAIN_FOUR` |
| Intercept (slot 1) | **Calibrate in field** (boot default: 0.0) |
| Slope (slot 1) | **Calibrate in field** (boot default: 1.0) |
| Gas library | "CO" |
| Range options | 100 ppm, 300 ppm |
| Calvalue options | 10, 25, 50, 100 ppm |
| High alarm | 0–300 ppm (step 10); recommended: 50 ppm (OSHA PEL) |
| PEL alarm | 0–50 ppm (step 5); recommended: 35 ppm (NIOSH REL) |
| OSHA PEL | 50 ppm TWA (8 h) |
| NIOSH REL | 35 ppm TWA; STEL 200 ppm |
| IDLH | 1200 ppm |

**`inc/RangeSet.h` change:**
```cpp
std::array<int, 2> m_rangeArray{{ 100, 300 }};
```

**`inc/AlarmSet.h` change:**
```cpp
std::array<int, 31> m_alarmArray{{
    0,
    10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
    110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
    210, 220, 230, 240, 250, 260, 270, 280, 290, 300
}};
```

**`inc/LowAlarmSet.h` change:**
```cpp
std::array<int, 11> m_lowalarmArray{{
    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
}};
```

**`inc/CalvalueSet.h` change:**
```cpp
std::array<int, 4> m_calvalueArray{{ 10, 25, 50, 100 }};
```

---

### EC — H2S (Hydrogen Sulfide), 0–50 ppm

| Parameter | Value |
|-----------|-------|
| Profile file | `inc/sensors/EC_H2S_50ppm.h` |
| BT name | `ESP32 EC-H2S` |
| ADS gain | `GAIN_FOUR` |
| Intercept (slot 1) | **Calibrate in field** (boot default: 0.0) |
| Slope (slot 1) | **Calibrate in field** (boot default: 1.0) |
| Gas library | "H2S" |
| Range options | 10 ppm, 50 ppm |
| Calvalue options | 1, 2, 5, 10, 25 ppm |
| High alarm | 0–50 ppm (step 1); recommended: 10 ppm (OSHA ceiling warning) |
| PEL alarm | 0–10 ppm (step 1); recommended: 1 ppm (ACGIH TLV) |
| OSHA PEL | 20 ppm ceiling |
| NIOSH REL | 1 ppm ceiling (10 min) |
| IDLH | 50 ppm |

**`inc/RangeSet.h` change:**
```cpp
std::array<int, 2> m_rangeArray{{ 10, 50 }};
```

**`inc/AlarmSet.h` change:**
```cpp
std::array<int, 51> m_alarmArray{{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
   11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
   31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50
}};
```

**`inc/LowAlarmSet.h` change:**
```cpp
std::array<int, 11> m_lowalarmArray{{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
}};
```

**`inc/CalvalueSet.h` change:**
```cpp
std::array<int, 5> m_calvalueArray{{ 1, 2, 5, 10, 25 }};
```

---

## Calibration procedure (all sensors)

1. **Zero calibration**
   - Navigate to `CALIBRATION Zero` menu
   - For PID: apply zero-grade air or charcoal-filtered air to the probe
   - For EC CO: apply ambient fresh air (away from any CO source)
   - For EC H2S: apply zero air (clean synthetic air, no sulfur)
   - Wait for reading to stabilize (~30–60 s)
   - Press **S** → saves `intercept` to EEPROM address 8 for the active gas slot

2. **Span calibration**
   - Navigate to `Calvalue` menu → select the concentration of your certified cal gas
   - Navigate to `CALIBRATION Calgas` menu
   - Apply the certified cal gas at the correct flow rate (usually 200–500 mL/min)
   - Wait for reading to stabilize (~60–120 s for EC, ~30 s for PID)
   - Press **S** → computes `slope = (calgas_voltage − zero_voltage) / calvalue_ppm`
   - Saves slope to EEPROM address 16; displays result on Cal Result screen

3. **Verification**
   - Return to Run screen
   - Re-apply cal gas — reading should match the certified concentration ±5%
   - Apply zero gas — reading should return to 0 ±1 digit

---

## EEPROM address map (GasManager parameters)

| Address | Content | Size |
|---------|---------|------|
| 4 | Gas slot index (selected gas) | 4 bytes (int) |
| 8 | Intercept slot 1 | 8 bytes (double) |
| 16 | Slope slot 1 | 8 bytes (double) |
| 24 | Intercept slot 2 | 8 bytes (double) |
| 32 | Slope slot 2 | 8 bytes (double) |
| 40 | Intercept slot 3 | 8 bytes (double) |
| 48 | Slope slot 3 | 8 bytes (double) |
| 56 | Intercept slot 4 | 8 bytes (double) |
| 64 | Slope slot 4 | 8 bytes (double) |
| 72 | Range index | 4 bytes (int) |
| 76 | Alarm index | 4 bytes (int) |
| 80 | Calvalue index | 4 bytes (int) |
| 92 | Low alarm index | 4 bytes (int) |
| 96 | Logging mode | 4 bytes (int) |
| 120 | Cal date string (ASCII) | 31 bytes |
| 210 | Manual log counter | 2 bytes (uint16) |
| 220 | RTC hour | 4 bytes (int) |
| 230 | RTC minute | 4 bytes (int) |

> **Note:** When swapping sensor types with very different sensitivity,
> clear the EEPROM calibration entries (addresses 8–64) or the unit will
> boot with wrong slope/intercept until the first field calibration.
> You can trigger a re-cal by simply running Zero → Calvalue → Cal Gas.
