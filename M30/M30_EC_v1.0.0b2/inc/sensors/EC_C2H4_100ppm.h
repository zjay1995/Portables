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
 * File        : inc/sensors/EC_C2H4_100ppm.h
 * Description : Sensor profile: EC — C2H4 (Ethylene), 0–100 ppm
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzers. All rights reserved.
 */
#pragma once
// =============================================================================
// Sensor Profile: EC — C2H4 (Ethylene), 0–100 ppm
// =============================================================================
// Sensor type : Electrochemical
// Unit        : ppm
// Resolution  : 0.1 ppm
// Over-range  : 110 ppm
// Use cases   : Fruit ripening monitoring, refinery / petrochemical safety
// OSHA PEL    : Simple asphyxiant — no specific PEL
// NIOSH REL   : 200 ppm TWA
// IDLH        : 100 ppm (flammable / asphyxiant context)
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 EC-C2H4"
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

//   addGas:    Gas("C2H4", 1.0)
//   Range:     { 50, 100 } ppm
//   Calvalue:  { 10, 25, 50, 100 } ppm
//   Alarm:     { 0, 10, 50, 100 } ppm
//   PEL alarm: { 0, 1, 5, 10 } ppm

// --- Identity + menu ladders (per master sensor spec) ---
#define SENSOR_GAS_NAME         "C2H4"
#define SENSOR_RANGE_VALUES     { 50.0f, 100.0f }
#define SENSOR_CALVALUE_VALUES  { 10.0f, 25.0f, 50.0f, 100.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 10.0f, 50.0f, 100.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 5.0f, 10.0f }

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      +1   // reducing gas: output rises, as CH2O does on this board (verify)
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per ppm
