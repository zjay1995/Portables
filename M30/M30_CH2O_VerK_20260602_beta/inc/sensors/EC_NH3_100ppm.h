#pragma once
// =============================================================================
// Sensor Profile: EC — NH3 (Ammonia), 0–100 ppm
// =============================================================================
// Sensor type : Electrochemical
// Output      : Voltage via TIA → ADS1115 (typically 0–1 V at full scale)
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 110 ppm (FS × 1.10)
// OSHA PEL    : 50 ppm TWA
// NIOSH REL   : 25 ppm TWA, STEL 35 ppm
// IDLH        : 300 ppm
// Last updated: 2026-05-27
// =============================================================================

// --- Bluetooth / identity ---
#define SENSOR_BT_NAME        "ESP32 EC-NH3"
#define SENSOR_GAS_NAME       "NH3"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1        // 0.1 ppm resolution
#define SENSOR_RANGE_FS       100.0f   // ppm full-scale (used by GasManager noise floor)
#define SENSOR_OVERRANGE      110.0f   // ppm — show "OVR" above this

// --- ADS1115 gain ---
#define SENSOR_ADS_GAIN       GAIN_FOUR

// --- GasManager default intercept / slope ---
#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// --- Menu ladders (auto-built into Range/Alarm/PEL/Calvalue menus + Set arrays) ---
//   Numbers expressed as float; ints render without decimals via SENSOR_RES_DIGITS.
//   Each list must start with 0 (= Off) for ALARM and PEL.
#define SENSOR_RANGE_VALUES   { 100.0f }
#define SENSOR_CALVALUE_VALUES { 10.0f, 25.0f, 50.0f, 100.0f }
#define SENSOR_ALARM_VALUES   { 0.0f, 10.0f, 25.0f, 50.0f, 75.0f, 100.0f }  // 50 = OSHA PEL
#define SENSOR_PEL_VALUES     { 0.0f, 5.0f, 10.0f, 25.0f, 35.0f }            // 25 = NIOSH REL
