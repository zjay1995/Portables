#pragma once
// =============================================================================
// Sensor Profile: EC — H2S (Hydrogen Sulfide), 0–2,000 ppm  [SPECIAL RANGE]
// =============================================================================
// Sensor type : Electrochemical, extended-range cell
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 1,100 ppm (firmware display cap; sensor reads to FS)
// Notes       : Use when monitoring sour-gas / oilfield exposure where
//               concentrations can exceed the 100 ppm cell saturation.
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-H2S-2K"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       2000.0f
#define SENSOR_OVERRANGE      1100.0f  // firmware display cap
#define SENSOR_ADS_GAIN       GAIN_TWO  // wider range needs less amplifier gain

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// --- Identity + menu ladders ---
#define SENSOR_GAS_NAME         "H2S"
#define SENSOR_RANGE_VALUES     { 500.0f, 1000.0f, 2000.0f }
#define SENSOR_CALVALUE_VALUES  { 50.0f, 100.0f, 500.0f, 1000.0f, 2000.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 50.0f, 100.0f, 500.0f, 1000.0f }   // first = Off
#define SENSOR_PEL_VALUES       { 0.0f, 10.0f, 20.0f, 50.0f, 100.0f }      // first = Off
