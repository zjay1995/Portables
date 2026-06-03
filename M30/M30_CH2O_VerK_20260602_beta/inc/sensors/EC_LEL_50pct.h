#pragma once
// =============================================================================
// Sensor Profile: Catalytic Bead — Combustible (LEL), 0–50 % LEL
// =============================================================================
// Sensor type : Pellistor (catalytic bead, Wheatstone bridge)
// Unit        : % LEL  (100 % LEL = lower explosive limit of the calibration gas)
// Resolution  : 0.01 % LEL
// Over-range  : 55 % LEL
// Cal gas     : Typically 50 % LEL methane in air (= 2.5 % CH4 by vol)
// Alarm       : Industry standard 10 % LEL warning, 25 % LEL danger
// IDLH        : Above 10 % LEL = evacuation
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 LEL"
#define SENSOR_UNIT_LABEL     "%LEL"
#define SENSOR_RES_DIGITS     2        // 0.01 % LEL
#define SENSOR_RANGE_FS       50.0f
#define SENSOR_OVERRANGE      55.0f
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
//   addGas:    Gas("LEL", 1.0)   // Cal gas determines target species
//   Range:     { 25, 50 } %LEL
//   Calvalue:  { 25, 50 } %LEL   (most cylinders are 50 %LEL CH4)
//   Alarm:     { 0, 10, 25, 50 } %LEL          (10 = warn, 25 = danger)
//   PEL alarm: { 0, 5, 10 } %LEL
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "LEL"
#define SENSOR_RANGE_VALUES     { 25.0f, 50.0f }
#define SENSOR_CALVALUE_VALUES  { 25.0f, 50.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 10.0f, 25.0f, 50.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 5.0f, 10.0f }
