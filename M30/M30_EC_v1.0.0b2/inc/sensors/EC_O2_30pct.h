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
 * File        : inc/sensors/EC_O2_30pct.h
 * Description : Sensor profile: EC — O2 (Oxygen), 0–30 %
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
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
#define SENSOR_ALARM_VALUES     { 0.0f, 23.5f }          // enrichment (above)
#define SENSOR_PEL_VALUES       { 0.0f, 19.5f }          // deficiency (below)

// --- Signal & calibration checks (v1.0.0) -----------------------------------
// Fill the nominal values from the datasheet (nA/ppm x board amplifier gain)
// or a good bench calibration (Info screen Zero / Sens). SENSOR_TBD keeps the
// sensitivity check and realistic DEV simulation off for this sensor.
#define SENSOR_SIGNAL_POLARITY      0    // galvanic O2 cell on this board not confirmed; 0 = no direction check
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per %
// O2 zero is done in N2, where the cell may legitimately read ~0 mV.
#define SENSOR_FAULT_ON_0MV         0
// Normal air is 20.9 % (DEV simulator background, DEMO range).
#define SENSOR_AMBIENT_VALUE        20.9f
// Alarms: "High" = enrichment (reading ABOVE setpoint), "Low" = deficiency
// (reading BELOW setpoint). Previously both lists alarmed only when the
// reading went above, so 19.5 % alarmed in normal air and never on low O2.
#define SENSOR_ALARM_TITLE          "Alarm : High"
#define SENSOR_ALARM_FALLING        0
#define SENSOR_PEL_TITLE            "Alarm : Low"
#define SENSOR_PEL_FALLING          1
