#pragma once
// =============================================================================
// Sensor Profile: EC — O2 (Oxygen), 0–30 %  [SPECIAL ZERO + SPAN]
// =============================================================================
// Sensor type : Electrochemical (galvanic) — depleting cell
// Unit        : % volume
// Resolution  : 0.01 %
// Over-range  : 33 %
//
// SPECIAL CALIBRATION NOTES:
//   Cannot zero in ambient air (always 20.9 % O2). Zero requires a sealed
//   N2 atmosphere. Span uses ambient air as the 20.9 % reference (since
//   atmospheric O2 is one of the most stable known references on Earth).
//
//   Typical procedure:
//     1. Flush sensor with N2 → Cal Zero
//     2. Expose to ambient air → set Calvalue = 20.9 → Cal Span
//
// OSHA PEL    : Enriched > 23.5 %, deficient < 19.5 %  (IDLH < 19.5 %)
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-O2"
#define SENSOR_UNIT_LABEL     "%"
#define SENSOR_RES_DIGITS     2        // 0.01 %
#define SENSOR_RANGE_FS       30.0f    // % vol
#define SENSOR_OVERRANGE      33.0f
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
//   addGas:    Gas("O2", 1.0)
//   Range:     { 25, 30 } %
//   Calvalue:  { 20.9 } %                       (atmospheric reference only)
//   Alarm:     { 0, 19.5, 23.5 } %              (deficient / enriched limits)
//   PEL alarm: { 0, 19.5 } %                    (lower safety threshold)
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "O2"
#define SENSOR_RANGE_VALUES     { 25.0f, 30.0f }
#define SENSOR_CALVALUE_VALUES  { 20.9f }
#define SENSOR_ALARM_VALUES     { 0.0f, 19.5f, 23.5f }
#define SENSOR_PEL_VALUES       { 0.0f, 19.5f }
