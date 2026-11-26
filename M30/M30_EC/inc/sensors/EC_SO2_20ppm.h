#pragma once
// =============================================================================
// Sensor Profile: EC — SO2 (Sulfur Dioxide), 0–20 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 22 ppm
// OSHA PEL    : 5 ppm TWA
// NIOSH REL   : 2 ppm TWA, STEL 5 ppm
// IDLH        : 100 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-SO2"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       20.0f
#define SENSOR_OVERRANGE      22.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("SO2", 1.0)
//   Range:     { 5, 10, 20 } ppm
//   Calvalue:  { 1, 5, 10, 20 } ppm
//   Alarm:     { 0, 2, 5, 10, 20 } ppm   (5 = OSHA PEL)
//   PEL alarm: { 0, 1, 2, 5 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "SO2"
#define SENSOR_RANGE_VALUES     { 5.0f, 10.0f, 20.0f }
#define SENSOR_CALVALUE_VALUES  { 1.0f, 5.0f, 10.0f, 20.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 2.0f, 5.0f, 10.0f, 20.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 2.0f, 5.0f }
