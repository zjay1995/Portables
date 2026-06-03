#pragma once
// =============================================================================
// Sensor Profile: NDIR — CO2 (Carbon Dioxide), 0–5,000 ppm
// =============================================================================
// Sensor type : Non-Dispersive InfraRed
// Hardware    : M12 demo unit (lower-range option)
// Unit        : ppm
// Resolution  : 0.1 ppm (display), ~1 ppm actual
// Over-range  : 5,500 ppm
// OSHA PEL    : 5,000 ppm TWA
// NIOSH REL   : 5,000 ppm TWA, STEL 30,000 ppm
// Use case    : Indoor air quality / building ventilation (OSHA PEL fits in FS)
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 NDIR-CO2-5K"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       5000.0f
#define SENSOR_OVERRANGE      5500.0f
#define SENSOR_ADS_GAIN       GAIN_ONE

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("CO2", 1.0)
//   Range:     { 1000, 2000, 5000 } ppm
//   Calvalue:  { 500, 1000, 2500, 5000 } ppm
//   Alarm:     { 0, 1000, 2000, 5000 } ppm  (5000 = OSHA PEL)
//   PEL alarm: { 0, 500, 1000, 2500 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CO2"
#define SENSOR_RANGE_VALUES     { 1000.0f, 2000.0f, 5000.0f }
#define SENSOR_CALVALUE_VALUES  { 500.0f, 1000.0f, 2500.0f, 5000.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1000.0f, 2000.0f, 5000.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 500.0f, 1000.0f, 2500.0f }
