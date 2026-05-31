#pragma once
// =============================================================================
// Sensor Profile: PID — CVOC (Chlorinated VOC), 0–10,000 ppm  [SPECIAL SW]
// =============================================================================
// Sensor type : Photoionization Detector, halogen-tuned lamp
// Unit        : ppm
// Resolution  : 0.10 % of FS  (i.e. 10 ppm on 0–10,000 ppm scale)
// Over-range  : 11,000 ppm
//
// SPECIAL SOFTWARE NOTES:
//   High-range variant of CVOC for industrial leak surveys and source-area
//   investigations where concentrations exceed the 500 ppm cell.
//   Same RF-table behavior as PID_CVOC_500ppm — see that header for the
//   species-by-species response-factor list.
//
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 PID-CVOC-10K"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     0        // 1 ppm granularity at this range
#define SENSOR_RANGE_FS       10000.0f
#define SENSOR_OVERRANGE      11000.0f
#define SENSOR_ADS_GAIN       GAIN_ONE

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
//   Range:     { 1000, 5000, 10000 } ppm
//   Calvalue:  { 100, 500, 1000, 5000 } ppm
//   Alarm:     { 0, 100, 1000, 5000 } ppm
//   PEL alarm: { 0, 25, 100, 500 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CVOC"
#define SENSOR_RANGE_VALUES     { 1000.0f, 5000.0f, 10000.0f }
#define SENSOR_CALVALUE_VALUES  { 100.0f, 500.0f, 1000.0f, 5000.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 100.0f, 1000.0f, 5000.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 25.0f, 100.0f, 500.0f }
