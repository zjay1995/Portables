#pragma once
// =============================================================================
// Sensor Profile: [FILL IN TYPE] — [FILL IN GAS], 0–[FILL IN RANGE] ppm
// =============================================================================
// Sensor type : [PID / Electrochemical / NDIR / etc.]
// Part number : [manufacturer + part]
// Output      : [voltage range and sensitivity, e.g., 0–2 V / 0–100 ppm]
// Unit        : [ppm / ppb / %LEL / % vol]
// OSHA PEL    : [value + type: TWA / STEL / ceiling]
// NIOSH REL   : [value]
// IDLH        : [value]
// Last updated: [YYYY-MM-DD]
// =============================================================================

// --- Bluetooth / identity ---
#define SENSOR_BT_NAME      "ESP32 [GAS]"
#define SENSOR_UNIT_LABEL   "ppm"

// --- ADS1115 gain ---
// GAIN_ONE  = ±4.096 V, 0.1250 mV/LSB  → use for PID, wide-range sensors
// GAIN_TWO  = ±2.048 V, 0.0625 mV/LSB  → use if max output ~1–2 V
// GAIN_FOUR = ±1.024 V, 0.0313 mV/LSB  → use for EC with small output (<1 V)
// GAIN_EIGHT= ±0.512 V, 0.0156 mV/LSB  → use for very small EC signals
#define SENSOR_ADS_GAIN     GAIN_ONE

// --- GasManager default intercept / slope ---
// Slot 1 (primary / only gas)
#define SENSOR_INTERCEPT    0.0     // zero-voltage at 0 ppm
#define SENSOR_SLOPE        1.0     // mV per ppm  (avoid 0 — divide-by-zero)
// Slot 2
#define SENSOR_INTERCEPT2   0.0
#define SENSOR_SLOPE2       1.0
// Slot 3
#define SENSOR_INTERCEPT3   0.0
#define SENSOR_SLOPE3       1.0
// Slot 4
#define SENSOR_INTERCEPT4   0.0
#define SENSOR_SLOPE4       1.0

// =============================================================================
// Menu configuration — fill in and paste into .ino
// =============================================================================

// GasManager addGas() calls:
//   g_gasManager.addGas(Gas("[GAS_NAME]", 1.0));

// Gas library (LIBRARY menu):
//   GasMenuItem("[GAS_NAME]", "LIBRARY", 0, ...)

// Range menu options:
//   RangeMenuItem("[X] ppm", "Range", 0, ...)
//   RangeMenuItem("[Y] ppm", "Range", 1, ...)
//   → RangeSet.h array: {X, Y}

// Calvalue menu:
//   [list cal gas concentrations available for this sensor]
//   → CalvalueSet.h array: {v1, v2, v3, ...}

// High alarm menu:
//   → AlarmSet.h — list threshold values; first entry = 0 (Off)

// PEL/Low alarm menu:
//   → LowAlarmSet.h — list threshold values; first entry = 0 (Off)
//   Recommended: OSHA PEL or NIOSH REL as default high end
