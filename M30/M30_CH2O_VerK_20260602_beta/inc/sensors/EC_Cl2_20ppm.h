#pragma once
// =============================================================================
// Sensor Profile: EC — Cl2 (Chlorine), 0–20 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 22 ppm
// OSHA PEL    : 1 ppm ceiling
// NIOSH REL   : 0.5 ppm STEL (15 min)
// IDLH        : 10 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-Cl2"
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

//   addGas:    Gas("Cl2", 1.0)
//   Range:     { 5, 10, 20 } ppm
//   Calvalue:  { 0.5, 1, 5, 10 } ppm
//   Alarm:     { 0, 0.5, 1, 5, 10 } ppm   (1 = OSHA ceiling)
//   PEL alarm: { 0, 0.5, 1 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "Cl2"
#define SENSOR_RANGE_VALUES     { 5.0f, 10.0f, 20.0f }
#define SENSOR_CALVALUE_VALUES  { 0.5f, 1.0f, 5.0f, 10.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 0.5f, 1.0f, 5.0f, 10.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.5f, 1.0f }
