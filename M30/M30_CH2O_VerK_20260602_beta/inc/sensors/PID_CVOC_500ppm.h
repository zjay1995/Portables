#pragma once
// =============================================================================
// Sensor Profile: PID — CVOC (Chlorinated VOC), 0–500 ppm  [SPECIAL SW]
// =============================================================================
// Sensor type : Photoionization Detector, halogen-tuned lamp
// Unit        : ppm
// Resolution  : 0.10 % of FS  (i.e. 0.5 ppm on 0–500 ppm scale)
// Over-range  : 550 ppm
//
// SPECIAL SOFTWARE NOTES:
//   Chlorinated VOC response factors differ wildly from the isobutylene
//   reference (RF values 0.5–6+ depending on species). The active gas
//   selection in the LIBRARY menu applies the species-specific RF to the
//   raw count before display. See GasManager addGas() calls for the gas
//   table (TCE, PCE, vinyl chloride, methylene chloride, etc.).
//
// Common CVOCs to monitor:
//   Trichloroethylene (TCE) : OSHA PEL 100 ppm TWA, STEL 200 ppm
//   Perchloroethylene (PCE) : OSHA PEL 100 ppm TWA
//   Vinyl chloride          : OSHA PEL 1 ppm TWA, STEL 5 ppm
//   Methylene chloride      : OSHA PEL 25 ppm TWA, STEL 125 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 PID-CVOC-500"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1        // 0.1 ppm display granularity
#define SENSOR_RANGE_FS       500.0f
#define SENSOR_OVERRANGE      550.0f
#define SENSOR_ADS_GAIN       GAIN_ONE

// PID has multiple gas slots — each with its own intercept/slope
#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("TCE",   2.7)
//              Gas("PCE",   1.7)
//              Gas("VCl",   2.0)
//              Gas("MeCl2", 0.6)
//   Range:     { 100, 250, 500 } ppm
//   Calvalue:  { 10, 50, 100, 250 } ppm  (isobutylene reference cylinder)
//   Alarm:     { 0, 1, 25, 100, 250 } ppm   (species-dependent, set per use)
//   PEL alarm: { 0, 1, 5, 25 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CVOC"
#define SENSOR_RANGE_VALUES     { 100.0f, 250.0f, 500.0f }
#define SENSOR_CALVALUE_VALUES  { 10.0f, 50.0f, 100.0f, 250.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1.0f, 25.0f, 100.0f, 250.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 5.0f, 25.0f }
