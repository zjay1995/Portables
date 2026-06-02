#pragma once
// =============================================================================
// Sensor Profile: EC — CO (Carbon Monoxide), 0–1,000 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 1,100 ppm
// OSHA PEL    : 50 ppm TWA
// NIOSH REL   : 35 ppm TWA, STEL 200 ppm
// IDLH        : 1,200 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-CO-1K"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       1000.0f
#define SENSOR_OVERRANGE      1100.0f
#define SENSOR_ADS_GAIN       GAIN_TWO

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// =============================================================================
//   addGas:    Gas("CO", 1.0)
//   Range:     { 100, 300, 1000 } ppm
//   Calvalue:  { 25, 50, 100, 250, 500 } ppm
//   Alarm:     { 0, 35, 50, 100, 200, 500, 1000 } ppm
//   PEL alarm: { 0, 25, 35, 50 } ppm
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CO"
#define SENSOR_RANGE_VALUES     { 100.0f, 300.0f, 1000.0f }
#define SENSOR_CALVALUE_VALUES  { 25.0f, 50.0f, 100.0f, 250.0f, 500.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 35.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 25.0f, 35.0f, 50.0f }
