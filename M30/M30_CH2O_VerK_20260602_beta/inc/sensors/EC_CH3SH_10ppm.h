#pragma once
// =============================================================================
// Sensor Profile: EC — CH3SH (Methyl Mercaptan), 0–10 ppm
// =============================================================================
// Sensor type : Electrochemical
// Hardware    : M40 unit (per gas-list reference photo)
// Unit        : ppm
// Resolution  : 0.01 ppm
// Over-range  : 11 ppm
// OSHA PEL    : 10 ppm ceiling
// NIOSH REL   : 0.5 ppm ceiling (15 min)
// IDLH        : 150 ppm
// Notes       : Strong sulfur odor at very low ppb; used as a natural-gas
//               odorant and in petrochemical / sewage monitoring.
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-CH3SH"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     2
#define SENSOR_RANGE_FS       10.0f
#define SENSOR_OVERRANGE      11.0f
#define SENSOR_ADS_GAIN       GAIN_EIGHT  // low-level EC, max amplification

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("CH3SH", 1.0)
//   Range:     { 1, 5, 10 } ppm
//   Calvalue:  { 0.5, 1, 5, 10 } ppm
//   Alarm:     { 0, 0.5, 5, 10 } ppm   (10 = OSHA ceiling)
//   PEL alarm: { 0, 0.1, 0.5 } ppm     (0.5 = NIOSH ceiling)

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CH3SH"
#define SENSOR_RANGE_VALUES     { 1.0f, 5.0f, 10.0f }
#define SENSOR_CALVALUE_VALUES  { 0.5f, 1.0f, 5.0f, 10.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 0.5f, 5.0f, 10.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.1f, 0.5f }
