/*
 *     ____  ________     ___                __
 *    / __ \/  _/ __ \   /   |  ____  ____ _/ /_  ______  ___  _____
 *   / /_/ // // / / /  / /| | / __ \/ __ `/ / / / /_  / / _ \/ ___/
 *  / ____// // /_/ /  / ___ |/ / / / /_/ / / /_/ / / /_/  __/ /
 * /_/   /___/_____/  /_/  |_/_/ /_/\__,_/_/\__, / /___/\___/_/
 *                                         /____/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/sensors/EC_NO2_20ppm.h
 * Description : Sensor profile: EC — NO2 (Nitrogen Dioxide), 0–20 ppm
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
#pragma once
// =============================================================================
// Sensor Profile: EC — NO2 (Nitrogen Dioxide), 0–20 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 22 ppm
// OSHA PEL    : 5 ppm ceiling
// NIOSH REL   : 1 ppm STEL
// IDLH        : 20 ppm
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-NO2"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       20.0f
#define SENSOR_OVERRANGE      22.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR

#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("NO2", 1.0)
//   Range:     { 5, 10, 20 } ppm
//   Calvalue:  { 1, 5, 10, 20 } ppm
//   Alarm:     { 0, 1, 5, 10, 20 } ppm   (5 = OSHA ceiling)
//   PEL alarm: { 0, 0.5, 1 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "NO2"
#define SENSOR_RANGE_VALUES     { 5.0f, 10.0f, 20.0f }
#define SENSOR_CALVALUE_VALUES  { 1.0f, 5.0f, 10.0f, 20.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 1.0f, 5.0f, 10.0f, 20.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 0.5f, 1.0f }

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      -1   // oxidizing gas: output falls (verify on this board)
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per ppm
