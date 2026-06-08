#pragma once
// =============================================================================
// Sensor Profile: TCD — H2 (Hydrogen), 0–1 %  (0–10,000 ppm)
// =============================================================================
// Sensor type : Thermal Conductivity Detector — bridge-type
// Hardware    : M10 unit
// Unit        : ppm (display) — sensor reads % vol H2 in carrier
// Resolution  : 1 ppm
// Over-range  : 11,000 ppm (1.1 %)
// LEL         : 4 % H2 in air (40,000 ppm)
// IDLH        : N/A (simple asphyxiant)
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 TCD-H2"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     0        // 1 ppm
#define SENSOR_RANGE_FS       10000.0f
#define SENSOR_OVERRANGE      11000.0f
#define SENSOR_ADS_GAIN       GAIN_ONE

// Reference (from M10_TCD_H2_VerK):
#define SENSOR_INTERCEPT      -2.054456771
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// =============================================================================
//   addGas:    Gas("H2", 1.0)
//   Range:     { 1000, 5000, 10000 } ppm
//   Calvalue:  { 500, 1000, 2000, 5000, 10000 } ppm
//   Alarm:     { 0, 1000, 4000, 10000 } ppm        (4000 = 10 % LEL)
//   PEL alarm: { 0, 500, 1000, 2000 } ppm
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "H2"
#define SENSOR_RANGE_VALUES     { 1000.0f, 5000.0f, 10000.0f }
#define SENSOR_CALVALUE_VALUES  { 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1000.0f, 4000.0f, 10000.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 500.0f, 1000.0f, 2000.0f }
