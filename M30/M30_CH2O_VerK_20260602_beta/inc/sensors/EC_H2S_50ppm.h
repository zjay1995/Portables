#pragma once
// =============================================================================
// Sensor Profile: Electrochemical — H2S (Hydrogen Sulfide), 0–50 ppm
// =============================================================================
// Sensor type : Electrochemical (amperometric)
// Example part: Alphasense H2S-B4, City Technology 4H2S/F, Membrapor H2S/C-50
// Output      : ~400–800 nA / ppm → transimpedance amp → ADS1115
// Unit        : ppm
// OSHA PEL    : 20 ppm ceiling (any 10-min peak)
// NIOSH REL   : 1 ppm ceiling (10 min); IDLH = 50 ppm
// ACGIH TLV   : 1 ppm TWA, 5 ppm STEL
// Last updated: 2026-05-14
// =============================================================================

// --- Bluetooth / identity ---
#define SENSOR_BT_NAME      "ESP32 EC-H2S"
#define SENSOR_UNIT_LABEL   "ppm"

// --- ADS1115 gain ---
// H2S EC sensors produce small outputs — GAIN_FOUR is typical.
// If your TIA gain is very high and output can reach >1 V at full scale,
// drop to GAIN_TWO.
#define SENSOR_ADS_GAIN     GAIN_FOUR

// --- GasManager default intercept / slope ---
// NOTE: Must calibrate before use. These are safe boot defaults only.
//
// Slot 1 — H2S (primary gas)
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
//   g_gasManager.addGas(Gas("H2S", 1.0));

// Gas library (LIBRARY menu) entries:
//   GasMenuItem("H2S", "LIBRARY", 0, ...)

// Range menu options:
//   RangeMenuItem("10 ppm",  "Range", 0, ...)
//   RangeMenuItem("50 ppm",  "Range", 1, ...)
//   → RangeSet.h array:  {10, 50}

// Calvalue menu (cal gas concentration options):
//   1, 2, 5, 10, 25                             (5 entries)
//   → CalvalueSet.h array: {1, 2, 5, 10, 25}

// High alarm menu:
//   0 (Off), 1–50 step 1                        (51 entries)
//   → AlarmSet.h must match
//   Recommended default: 10 ppm  (OSHA ceiling threshold warning)

// PEL alarm menu:
//   0 (Off), 1–10 step 1                        (11 entries)
//   → LowAlarmSet.h must match
//   Recommended default: 1 ppm  (ACGIH TLV-TWA)

// IMPORTANT — H2S hazard notes:
//   H2S causes rapid olfactory fatigue — DO NOT rely on smell.
//   Set high alarm to 20 ppm (OSHA ceiling) or lower.
//   IDLH = 50 ppm; immediately dangerous to life and health.
//   Cal gas: use certified standard gas (1–10 ppm range typical).

// --- Display + identity + menu ladders (per master sensor spec) ---
#define SENSOR_RES_DIGITS       1
#define SENSOR_RANGE_FS         50.0f
#define SENSOR_OVERRANGE        55.0f
#define SENSOR_GAS_NAME         "H2S"
#define SENSOR_RANGE_VALUES     { 10.0f, 50.0f }
#define SENSOR_CALVALUE_VALUES  { 1.0f, 2.0f, 5.0f, 10.0f, 25.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 5.0f, 10.0f, 20.0f, 50.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 5.0f, 10.0f }
