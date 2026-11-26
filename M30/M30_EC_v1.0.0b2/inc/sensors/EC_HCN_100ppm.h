#pragma once
// =============================================================================
// Sensor Profile: EC — HCN (Hydrogen Cyanide), 0–100 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 110 ppm
// OSHA PEL    : 10 ppm ceiling
// NIOSH REL   : 4.7 ppm STEL
// IDLH        : 50 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-HCN"
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

//   addGas:    Gas("HCN", 1.0)
//   Range:     { 10, 50, 100 } ppm
//   Calvalue:  { 5, 10, 25, 50 } ppm
//   Alarm:     { 0, 5, 10, 50, 100 } ppm   (10 = OSHA ceiling)
//   PEL alarm: { 0, 1, 5, 10 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "HCN"
#define SENSOR_RANGE_VALUES     { 10.0f, 50.0f, 100.0f }
#define SENSOR_CALVALUE_VALUES  { 5.0f, 10.0f, 25.0f, 50.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 5.0f, 10.0f, 50.0f, 100.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 5.0f, 10.0f }

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      +1   // reducing gas: output rises, as CH2O does on this board (verify)
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per ppm
