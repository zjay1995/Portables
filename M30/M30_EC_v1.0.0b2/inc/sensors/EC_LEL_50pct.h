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
 * File        : inc/sensors/EC_LEL_50pct.h
 * Description : Sensor profile: Catalytic Bead — Combustible (LEL), 0–50 % LEL
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzers. All rights reserved.
 */
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

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      0    // pellistor bridge output direction depends on the bridge wiring; 0 = no direction check
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per % LEL
// A balanced bridge can sit at ~0 mV in clean air: don't treat 0 mV as a
// fault until the board's zero offset is confirmed (then set to 1).
#define SENSOR_FAULT_ON_0MV         0
