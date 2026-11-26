#pragma once
// =============================================================================
// Sensor Profile: EC — H2S (Hydrogen Sulfide), 0–100 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 110 ppm
// OSHA PEL    : 20 ppm ceiling
// NIOSH REL   : 1 ppm ceiling (10 min)
// IDLH        : 100 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-H2S-100"
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

// =============================================================================
//   addGas:    Gas("H2S", 1.0)
//   Range:     { 100 } ppm
//   Calvalue:  { 5, 10, 25, 50, 100 } ppm
//   Alarm:     { 0, 5, 10, 20, 50, 100 } ppm  (20 = OSHA ceiling)
//   PEL alarm: { 0, 1, 2, 5, 10 } ppm         (1 = NIOSH ceiling)
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "H2S"
#define SENSOR_RANGE_VALUES     { 100.0f }
#define SENSOR_CALVALUE_VALUES  { 5.0f, 10.0f, 25.0f, 50.0f, 100.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 2.0f, 5.0f, 10.0f }
