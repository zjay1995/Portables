#pragma once
// =============================================================================
// Sensor Profile: Electrochemical — CO (Carbon Monoxide), 0–300 ppm
// =============================================================================
// Sensor type : Electrochemical (amperometric)
// Example part: Alphasense CO-BF, City Technology 4CO/F, Membrapor CO/S-100
// Output      : ~180–400 nA / 100 ppm → transimpedance amp → ADS1115
// Unit        : ppm
// OSHA PEL    : 50 ppm TWA (8 h)
// NIOSH REL   : 35 ppm TWA; STEL 200 ppm (ceiling)
// IDLH        : 1200 ppm
// Last updated: 2026-05-14
// =============================================================================

// --- Bluetooth / identity ---
#define SENSOR_BT_NAME      "ESP32 EC-CO"
#define SENSOR_UNIT_LABEL   "ppm"

// --- ADS1115 gain ---
// EC sensors produce smaller output voltages than PID sensors.
// GAIN_FOUR (±1.024 V, 0.03125 mV/LSB) gives 4× better resolution.
// If transimpedance amp output can exceed 1 V, use GAIN_TWO instead.
#define SENSOR_ADS_GAIN     GAIN_FOUR

// --- GasManager default intercept / slope ---
// NOTE: These MUST be determined by field calibration for your specific unit.
//       Leave slope = 1.0 until calibration so the unit boots without crashing.
//       The field cal procedure (Zero → Calvalue → Cal Gas) will overwrite EEPROM.
//
// Slot 1 — CO (primary gas)
#define SENSOR_INTERCEPT    0.0     // TODO: fill in after zero cal
#define SENSOR_SLOPE        1.0     // TODO: fill in after span cal
// Slots 2–4 — unused
#define SENSOR_INTERCEPT2   0.0
#define SENSOR_SLOPE2       1.0
#define SENSOR_INTERCEPT3   0.0
#define SENSOR_SLOPE3       1.0
#define SENSOR_INTERCEPT4   0.0
#define SENSOR_SLOPE4       1.0

// =============================================================================
// Menu configuration — edit these sections in the .ino when swapping sensors
// =============================================================================

// GasManager addGas() calls:
//   g_gasManager.addGas(Gas("CO", 1.0));

// Gas library (LIBRARY menu) entries:
//   GasMenuItem("CO", "LIBRARY", 0, ...)

// Range menu options:
//   RangeMenuItem("100 ppm", "Range", 0, ...)
//   RangeMenuItem("300 ppm", "Range", 1, ...)
//   → RangeSet.h array:  {100, 300}

// Calvalue menu (cal gas concentration options):
//   10, 25, 50, 100                             (4 entries)
//   → CalvalueSet.h array: {10, 25, 50, 100}

// High alarm menu:
//   0 (Off), 10–300 step 10                     (31 entries)
//   → AlarmSet.h must match
//   Recommended default: 50 ppm  (OSHA PEL)

// PEL alarm menu:
//   0 (Off), 5–50 step 5                        (11 entries)
//   → LowAlarmSet.h must match
//   Recommended default: 35 ppm  (NIOSH REL)

// Transimpedance amp notes:
//   Sensitivity example (CO-BF): ~60 nA/ppm → with 1 MΩ TIA gain = 60 µV/ppm
//   At 300 ppm → ~18 mV output  (GAIN_FOUR resolves to ~0.6 ppm/LSB → good)
//   At 100 ppm → ~6 mV output
//   Increase TIA resistor if output is too small for good ADC resolution.
