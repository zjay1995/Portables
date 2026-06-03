#pragma once
// =============================================================================
// Sensor Profile: EC — ETO (Ethylene Oxide, C2H4O), 0–100 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 110 ppm
// OSHA PEL    : 1 ppm TWA, STEL 5 ppm
// NIOSH REL   : 0.1 ppm TWA, ceiling 5 ppm (10 min/day)
// IDLH        : 800 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-ETO"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       100.0f
#define SENSOR_OVERRANGE      110.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("ETO", 1.0)
//   Range:     { 10, 50, 100 } ppm
//   Calvalue:  { 1, 5, 10, 50 } ppm
//   Alarm:     { 0, 1, 5, 50, 100 } ppm   (1 = OSHA PEL)
//   PEL alarm: { 0, 0.5, 1 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "ETO"
#define SENSOR_RANGE_VALUES     { 10.0f, 50.0f, 100.0f }
#define SENSOR_CALVALUE_VALUES  { 1.0f, 5.0f, 10.0f, 50.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1.0f, 5.0f, 50.0f, 100.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.5f, 1.0f }
