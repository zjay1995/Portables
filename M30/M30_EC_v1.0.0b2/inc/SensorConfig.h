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
 * File        : inc/SensorConfig.h
 * Description : Sensor profile table, defaults and checks (select in BuildConfig.h).
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzers. All rights reserved.
 */
#pragma once
// =============================================================================
// Sensor profile table — DO NOT edit to choose a sensor.
// Select the sensor with SENSOR_PROFILE in BuildConfig.h (sketch folder).
// =============================================================================
#include "../BuildConfig.h"   // sketch root

// --- Profile IDs -------------------------------------------------------------
// Each ID is stored in EEPROM so the unit can tell when its firmware was built
// for a different sensor (it then reloads factory defaults and shows CAL?).
// IDs are permanent: never renumber or reuse one; add new profiles at the end.
#define EC_C2H4_100PPM    1
#define EC_CH2O_5PPM      2
#define EC_CH2O_50PPM     3
#define EC_CH3SH_10PPM    4
#define EC_CO_300PPM      5
#define EC_CO_1000PPM     6
#define EC_CL2_20PPM      7
#define EC_ETO_100PPM     8
#define EC_H2S_50PPM      9
#define EC_H2S_100PPM     10
#define EC_H2S_2000PPM    11
#define EC_HCN_100PPM     12
#define EC_HCL_10PPM      13
#define EC_HF_10PPM       14
#define EC_LEL_50PCT      15
#define EC_NH3_100PPM     16
#define EC_NO2_20PPM      17
#define EC_O2_30PCT       18
#define EC_O3_10PPM       19
#define EC_PH3_10PPM      20
#define EC_SO2_20PPM      21

#ifndef SENSOR_PROFILE
  #error "Select a sensor: set SENSOR_PROFILE in BuildConfig.h (sketch folder)"
#endif

#if   SENSOR_PROFILE == EC_C2H4_100PPM
  #include "sensors/EC_C2H4_100ppm.h"
  #define SENSOR_PROFILE_NAME "EC_C2H4_100PPM"
#elif SENSOR_PROFILE == EC_CH2O_5PPM
  #include "sensors/EC_CH2O_5ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CH2O_5PPM"
#elif SENSOR_PROFILE == EC_CH2O_50PPM
  #include "sensors/EC_CH2O_50ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CH2O_50PPM"
#elif SENSOR_PROFILE == EC_CH3SH_10PPM
  #include "sensors/EC_CH3SH_10ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CH3SH_10PPM"
#elif SENSOR_PROFILE == EC_CO_300PPM
  #include "sensors/EC_CO_300ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CO_300PPM"
#elif SENSOR_PROFILE == EC_CO_1000PPM
  #include "sensors/EC_CO_1000ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CO_1000PPM"
#elif SENSOR_PROFILE == EC_CL2_20PPM
  #include "sensors/EC_Cl2_20ppm.h"
  #define SENSOR_PROFILE_NAME "EC_CL2_20PPM"
#elif SENSOR_PROFILE == EC_ETO_100PPM
  #include "sensors/EC_ETO_100ppm.h"
  #define SENSOR_PROFILE_NAME "EC_ETO_100PPM"
#elif SENSOR_PROFILE == EC_H2S_50PPM
  #include "sensors/EC_H2S_50ppm.h"
  #define SENSOR_PROFILE_NAME "EC_H2S_50PPM"
#elif SENSOR_PROFILE == EC_H2S_100PPM
  #include "sensors/EC_H2S_100ppm.h"
  #define SENSOR_PROFILE_NAME "EC_H2S_100PPM"
#elif SENSOR_PROFILE == EC_H2S_2000PPM
  #include "sensors/EC_H2S_2000ppm.h"
  #define SENSOR_PROFILE_NAME "EC_H2S_2000PPM"
#elif SENSOR_PROFILE == EC_HCN_100PPM
  #include "sensors/EC_HCN_100ppm.h"
  #define SENSOR_PROFILE_NAME "EC_HCN_100PPM"
#elif SENSOR_PROFILE == EC_HCL_10PPM
  #include "sensors/EC_HCl_10ppm.h"
  #define SENSOR_PROFILE_NAME "EC_HCL_10PPM"
#elif SENSOR_PROFILE == EC_HF_10PPM
  #include "sensors/EC_HF_10ppm.h"
  #define SENSOR_PROFILE_NAME "EC_HF_10PPM"
#elif SENSOR_PROFILE == EC_LEL_50PCT
  #include "sensors/EC_LEL_50pct.h"
  #define SENSOR_PROFILE_NAME "EC_LEL_50PCT"
#elif SENSOR_PROFILE == EC_NH3_100PPM
  #include "sensors/EC_NH3_100ppm.h"
  #define SENSOR_PROFILE_NAME "EC_NH3_100PPM"
#elif SENSOR_PROFILE == EC_NO2_20PPM
  #include "sensors/EC_NO2_20ppm.h"
  #define SENSOR_PROFILE_NAME "EC_NO2_20PPM"
#elif SENSOR_PROFILE == EC_O2_30PCT
  #include "sensors/EC_O2_30pct.h"
  #define SENSOR_PROFILE_NAME "EC_O2_30PCT"
#elif SENSOR_PROFILE == EC_O3_10PPM
  #include "sensors/EC_O3_10ppm.h"
  #define SENSOR_PROFILE_NAME "EC_O3_10PPM"
#elif SENSOR_PROFILE == EC_PH3_10PPM
  #include "sensors/EC_PH3_10ppm.h"
  #define SENSOR_PROFILE_NAME "EC_PH3_10PPM"
#elif SENSOR_PROFILE == EC_SO2_20PPM
  #include "sensors/EC_SO2_20ppm.h"
  #define SENSOR_PROFILE_NAME "EC_SO2_20PPM"
#else
  #error "Unknown SENSOR_PROFILE in BuildConfig.h (sketch folder). Use one of: EC_C2H4_100PPM EC_CH2O_5PPM EC_CH2O_50PPM EC_CH3SH_10PPM EC_CO_300PPM EC_CO_1000PPM EC_CL2_20PPM EC_ETO_100PPM EC_H2S_50PPM EC_H2S_100PPM EC_H2S_2000PPM EC_HCN_100PPM EC_HCL_10PPM EC_HF_10PPM EC_LEL_50PCT EC_NH3_100PPM EC_NO2_20PPM EC_O2_30PCT EC_O3_10PPM EC_PH3_10PPM EC_SO2_20PPM"
#endif

#define SENSOR_PROFILE_ID  SENSOR_PROFILE

// =============================================================================
// Required fields — every profile must define these
// =============================================================================
#if !defined(SENSOR_BT_NAME) || !defined(SENSOR_GAS_NAME) || !defined(SENSOR_UNIT_LABEL) || \
    !defined(SENSOR_RES_DIGITS) || !defined(SENSOR_RANGE_FS) || !defined(SENSOR_OVERRANGE) || \
    !defined(SENSOR_ADS_GAIN) || !defined(SENSOR_INTERCEPT) || !defined(SENSOR_SLOPE) || \
    !defined(SENSOR_RANGE_VALUES) || !defined(SENSOR_CALVALUE_VALUES) || \
    !defined(SENSOR_ALARM_VALUES) || !defined(SENSOR_PEL_VALUES)
  #error "Sensor profile is missing a required field (see sensors/EC_TEMPLATE.h)."
#endif

// =============================================================================
// Per-sensor behaviour — defaults for anything a profile leaves out
// =============================================================================

// "Not known yet" marker for datasheet / bench values. Features that need the
// value stay off until it is filled in.
#define SENSOR_TBD  0.0f

// Detector output direction when gas rises: +1 rises, -1 falls, 0 = unknown.
// Used to reject a span calibration that moves the wrong way.
#ifndef SENSOR_SIGNAL_POLARITY
  #define SENSOR_SIGNAL_POLARITY  0
#endif

// Nominal detector values on this board (mV). SENSOR_TBD = unknown.
#ifndef SENSOR_NOMINAL_ZERO_MV
  #define SENSOR_NOMINAL_ZERO_MV  SENSOR_TBD      // clean-air / zero-gas output
#endif
#ifndef SENSOR_NOMINAL_MV_PER_UNIT
  #define SENSOR_NOMINAL_MV_PER_UNIT  SENSOR_TBD  // output change per ppm (or %)
#endif
// A span whose measured sensitivity is outside nominal x [MIN, MAX] fails
// (aged/poisoned cell, wrong gas). Only applies when the nominal is known.
#ifndef SENSOR_SENS_MIN_RATIO
  #define SENSOR_SENS_MIN_RATIO  0.30f
#endif
#ifndef SENSOR_SENS_MAX_RATIO
  #define SENSOR_SENS_MAX_RATIO  3.00f
#endif

// 1 = a detector reading of exactly 0 mV means a fault (biased front end that
// never legitimately reaches 0 V). 0 = 0 mV can be a valid reading.
#ifndef SENSOR_FAULT_ON_0MV
  #define SENSOR_FAULT_ON_0MV  1
#endif

// Normal clean-air reading (O2: 20.9 %). Used by the DEV simulator and DEMO.
#ifndef SENSOR_AMBIENT_VALUE
  #define SENSOR_AMBIENT_VALUE  0.0f
#endif

// Alarm menus: title and direction (0 = alarm when reading goes ABOVE the
// setpoint, 1 = alarm when it goes BELOW, e.g. O2 deficiency).
#ifndef SENSOR_ALARM_TITLE
  #define SENSOR_ALARM_TITLE   "Alarm : High"
#endif
#ifndef SENSOR_ALARM_FALLING
  #define SENSOR_ALARM_FALLING 0
#endif
#ifndef SENSOR_PEL_TITLE
  #define SENSOR_PEL_TITLE     "Alarm : PEL"
#endif
#ifndef SENSOR_PEL_FALLING
  #define SENSOR_PEL_FALLING   0
#endif

// DEMO build reading range (random within it). Plain C++ expressions: the
// preprocessor cannot compare floating-point values such as 20.9f.
#ifndef SENSOR_DEMO_MIN
  #define SENSOR_DEMO_MIN  ((SENSOR_AMBIENT_VALUE) > 0 ? (SENSOR_AMBIENT_VALUE) - 0.005f * SENSOR_RANGE_FS : 0.0f)
#endif
#ifndef SENSOR_DEMO_MAX                                   // CH2O 50 ppm -> 0.00-0.30
  #define SENSOR_DEMO_MAX  ((SENSOR_AMBIENT_VALUE) > 0 ? (SENSOR_AMBIENT_VALUE) + 0.005f * SENSOR_RANGE_FS : 0.006f * SENSOR_RANGE_FS)
#endif

// Extra gas slots (multi-gas builds); single-gas profiles may omit them.
#ifndef SENSOR_INTERCEPT2
  #define SENSOR_INTERCEPT2 0.0
  #define SENSOR_SLOPE2     1.0
#endif
#ifndef SENSOR_INTERCEPT3
  #define SENSOR_INTERCEPT3 0.0
  #define SENSOR_SLOPE3     1.0
#endif
#ifndef SENSOR_INTERCEPT4
  #define SENSOR_INTERCEPT4 0.0
  #define SENSOR_SLOPE4     1.0
#endif

#if SENSOR_SIGNAL_POLARITY != 1 && SENSOR_SIGNAL_POLARITY != -1 && SENSOR_SIGNAL_POLARITY != 0
  #error "SENSOR_SIGNAL_POLARITY must be +1, -1 or 0"
#endif
