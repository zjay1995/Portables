/*
 *     ____  ________     ___                __
 *    / __ \/  _/ __ \   /   |  ____  ____ _/ /_  ______  ___  __________
 *   / /_/ // // / / /  / /| | / __ \/ __ `/ / / / /_  / / _ \/ ___/ ___/
 *  / ____// // /_/ /  / ___ |/ / / / /_/ / / /_/ / / /_/  __/ /  (__  )
 * /_/   /___/_____/  /_/  |_/_/ /_/\__,_/_/\__, / /___/\___/_/  /____/
 *                                         /____/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/sensors/EC_PH3_10ppm.h
 * Description : Sensor profile: EC — PH3 (Phosphine), 0–10 ppm
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzers. All rights reserved.
 */
#pragma once
// =============================================================================
// Sensor Profile: EC — PH3 (Phosphine), 0–10 ppm  [SPECIAL SW]
// =============================================================================
// Sensor type : Electrochemical (low-concentration cell)
// Unit        : ppm
// Resolution  : 0.01 ppm
// Over-range  : 11 ppm
// OSHA PEL    : 0.3 ppm TWA
// NIOSH REL   : 0.3 ppm TWA, STEL 1 ppm
// IDLH        : 50 ppm
//
// SPECIAL SOFTWARE NOTES:
//   PH3 sensors typically need higher-resolution sampling and longer averaging
//   than the standard 4-sample / 0.5 Hz read loop. Consider:
//     - 16-sample boxcar average instead of 4
//     - 5-second median filter for the displayed value
//   Implement these in DataSource.h getDoubleValue() when this profile is active.
//
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-PH3"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     2        // 0.01 ppm
#define SENSOR_RANGE_FS       10.0f
#define SENSOR_OVERRANGE      11.0f
#define SENSOR_ADS_GAIN       GAIN_EIGHT  // small EC signal — max amplification

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

// =============================================================================
//   addGas:    Gas("PH3", 1.0)
//   Range:     { 1, 5, 10 } ppm
//   Calvalue:  { 0.5, 1, 5, 10 } ppm
//   Alarm:     { 0, 0.3, 1, 5, 10 } ppm        (0.3 = OSHA PEL)
//   PEL alarm: { 0, 0.1, 0.3 } ppm
// =============================================================================

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "PH3"
#define SENSOR_RANGE_VALUES     { 1.0f, 5.0f, 10.0f }
#define SENSOR_CALVALUE_VALUES  { 0.5f, 1.0f, 5.0f, 10.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 0.3f, 1.0f, 5.0f, 10.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.1f, 0.3f }

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      +1   // reducing gas: output rises, as CH2O does on this board (verify)
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per ppm
