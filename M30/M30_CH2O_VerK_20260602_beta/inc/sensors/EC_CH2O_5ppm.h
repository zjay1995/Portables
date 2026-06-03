#pragma once
// =============================================================================
// Sensor Profile: EC — CH2O (Formaldehyde), 0–5.00 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.01 ppm
// Over-range  : 5.5 ppm
// OSHA PEL    : 0.75 ppm TWA, STEL 2 ppm
// NIOSH REL   : 0.016 ppm TWA, ceiling 0.1 ppm
// IDLH        : 20 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-CH2O-5"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     2        // 0.01 ppm
#define SENSOR_RANGE_FS       5.0f
#define SENSOR_OVERRANGE      5.5f
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
//   addGas:    Gas("CH2O", 1.0)
//   Range:     { 1, 5 } ppm
//   Calvalue:  { 0.5, 1, 2, 5 } ppm
//   Alarm:     { 0, 0.1, 0.75, 2, 5 } ppm
//   PEL alarm: { 0, 0.1, 0.5, 0.75 } ppm   (0.75 = OSHA PEL TWA)
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CH2O"
#define SENSOR_RANGE_VALUES     { 1.0f, 5.0f }
#define SENSOR_CALVALUE_VALUES  { 0.5f, 1.0f, 2.0f, 5.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 0.1f, 0.75f, 2.0f, 5.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.1f, 0.5f, 0.75f }
