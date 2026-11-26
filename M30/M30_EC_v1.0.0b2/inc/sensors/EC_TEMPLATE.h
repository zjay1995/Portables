/*
 *  ____ ___ ____       _                _
 * |  _ \_ _|  _ \     / \   _ __   __ _| |_   _ _______ _ __
 * | |_) | || | | |   / _ \ | '_ \ / _` | | | | |_  / _ \ '__|
 * |  __/| || |_| |  / ___ \| | | | (_| | | |_| |/ /  __/ |
 * |_|  |___|____/  /_/   \_\_| |_|\__,_|_|\__, /___\___|_|
 *                                         |___/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/sensors/EC_TEMPLATE.h
 * Description : Template for adding a new sensor profile.
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
#pragma once
// =============================================================================
// Sensor Profile TEMPLATE — copy to EC_<GAS>_<RANGE>.h to add a sensor
// =============================================================================
// To add a sensor:
//   1. Copy this file, fill in every field.
//   2. In inc/SensorConfig.h add a NEW profile ID at the END of the ID list
//      (never reuse or renumber IDs; they are stored in each unit's EEPROM)
//      and an #elif that includes the new file.
//   3. Add the name to the list in BuildConfig.h (sketch folder), select it, build.
//   4. Bump MINOR in BuildConfig.h (section 4) and note it in CHANGELOG.md.
//
// Sensor type : [Electrochemical / pellistor / ...]
// Part number : [manufacturer + part]
// Sensitivity : [nA/ppm from datasheet] x [board amplifier gain] = [mV/ppm]
// OSHA PEL    : [value + TWA / STEL / ceiling]   NIOSH REL / IDLH: [value]
// Last updated: [YYYY-MM-DD]
// =============================================================================

// --- Identity & display ------------------------------------------------------
#define SENSOR_BT_NAME        "ESP32 EC-[GAS]"   // Bluetooth name
#define SENSOR_GAS_NAME       "[GAS]"            // shown on screen, <= 5 chars
#define SENSOR_UNIT_LABEL     "ppm"              // ppm / % / %LEL
#define SENSOR_RES_DIGITS     1                  // decimals on the Run screen
#define SENSOR_RANGE_FS       100.0f             // full scale
#define SENSOR_OVERRANGE      110.0f             // above this the screen shows OVER

// --- ADS1115 gain: pick the highest gain whose range still fits the sensor's
//     largest output (zero mV + full scale x sensitivity) with some headroom.
//   GAIN_ONE ±4.096 V · GAIN_TWO ±2.048 V · GAIN_FOUR ±1.024 V · GAIN_EIGHT ±0.512 V
#define SENSOR_ADS_GAIN       GAIN_FOUR

// --- Factory placeholder calibration (replaced by zero + span cal) ----------
#define SENSOR_INTERCEPT      0.0                // keep: marks "never calibrated"
#define SENSOR_SLOPE          1.0                // keep: marks "never calibrated"

// --- Menu ladders --------------------------------------------------------------
#define SENSOR_RANGE_VALUES     { 20.0f, 50.0f, 100.0f }
#define SENSOR_CALVALUE_VALUES  { 10.0f, 25.0f, 50.0f }        // cal gas concentrations
#define SENSOR_ALARM_VALUES     { 0.0f, 10.0f, 25.0f, 50.0f }  // first entry 0 = Off
#define SENSOR_PEL_VALUES       { 0.0f, 5.0f, 10.0f }          // first entry 0 = Off

// --- Signal & calibration checks -----------------------------------------------
#define SENSOR_SIGNAL_POLARITY      0            // +1 output rises with gas, -1 falls, 0 unknown
#define SENSOR_NOMINAL_ZERO_MV      SENSOR_TBD   // mV in zero gas on this board
#define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD   // mV per ppm on this board

// --- Optional (defaults in inc/SensorConfig.h) --------------------------------
// #define SENSOR_FAULT_ON_0MV    0              // if 0 mV can be a valid reading
// #define SENSOR_AMBIENT_VALUE   20.9f          // normal-air reading (O2)
// #define SENSOR_ALARM_TITLE     "Alarm : High"
// #define SENSOR_ALARM_FALLING   1              // alarm when BELOW setpoint
// #define SENSOR_PEL_TITLE       "Alarm : Low"
// #define SENSOR_PEL_FALLING     1
// #define SENSOR_SENS_MIN_RATIO  0.30f          // span sensitivity window vs nominal
// #define SENSOR_SENS_MAX_RATIO  3.00f
// #define SENSOR_DEMO_MIN        0.0f           // DEMO build reading range
// #define SENSOR_DEMO_MAX        1.0f
