#pragma once
// =============================================================================
// Sensor Profile: EC — CO2 (Carbon Dioxide), 0–5,000 ppm   [Winsen EC]
// =============================================================================
// Sensor type : Electrochemical (Winsen) — NOT the NDIR CO2 module
// Hardware    : M40 unit
// Unit        : ppm
// Resolution  : 1 ppm
// Over-range  : 5,500 ppm (FS x 1.10)
// OSHA PEL    : 5,000 ppm TWA
// NIOSH REL   : 5,000 ppm TWA, STEL 30,000 ppm
// Notes       : Winsen EC CO2 cell. Output is a small voltage via TIA -> ADS1115;
//               confirm full-scale output and adjust SENSOR_ADS_GAIN so the
//               reading uses as much of the ADC span as possible without
//               clipping. Field cal (Zero -> Calvalue -> Cal Gas) sets slope.
// Last updated: 2026-05-31
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-CO2"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     0        // 1 ppm
#define SENSOR_RANGE_FS       5000.0f
#define SENSOR_OVERRANGE      5500.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR  // EC small signal — tune to TIA output

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "CO2"
#define SENSOR_RANGE_VALUES     { 1000.0f, 2000.0f, 5000.0f }
#define SENSOR_CALVALUE_VALUES  { 500.0f, 1000.0f, 2500.0f, 5000.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1000.0f, 2000.0f, 5000.0f }   // first = Off; 5000 = OSHA PEL
#define SENSOR_PEL_VALUES       { 0.0f, 500.0f, 1000.0f, 2500.0f }    // first = Off
