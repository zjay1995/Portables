#pragma once
// =============================================================================
// Build mode — change ONE line below, then compile and flash.
// =============================================================================
//   BUILD_PROD : shipped firmware. Real sensor, raw detector mV hidden on the
//                Run screen.
//   BUILD_DEV  : bench testing without a sensor. The detector voltage is
//                simulated (see SensorSimulator.h) so zero/span calibration,
//                alarms, logging and FAULT / CAL? / OVER behave as in the
//                field. Run screen shows "SIM"; serial accepts SIM commands.
//   BUILD_DEMO : trade-show demo. Run screen shows a random 0.00-0.30 ppm
//                reading; no sensor or calibration needed.
//
// Never ship a DEV or DEMO build: the gas reading is not real.
// =============================================================================

#define BUILD_PROD 0
#define BUILD_DEV  1
#define BUILD_DEMO 2

#ifndef BUILD_MODE
  #define BUILD_MODE  BUILD_PROD      // <-- BUILD_PROD | BUILD_DEV | BUILD_DEMO
#endif

#if BUILD_MODE == BUILD_DEV
  #define BUILD_MODE_TAG      "SIM"     // shown next to the gas name
  #define FW_MODE_NAME        "DEV"
  #define RUN_DISPLAY_DIGITS  1
  #define SHOW_RAW_MV_ON_RUN  1
#elif BUILD_MODE == BUILD_DEMO
  #define BUILD_MODE_TAG      "DEMO"
  #define FW_MODE_NAME        "DEMO"
  #define RUN_DISPLAY_DIGITS  2         // make the sub-ppm demo value visible
  #define SHOW_RAW_MV_ON_RUN  0
#elif BUILD_MODE == BUILD_PROD
  #define BUILD_MODE_TAG      ""
  #define FW_MODE_NAME        "VerK"
  #define RUN_DISPLAY_DIGITS  1
  #define SHOW_RAW_MV_ON_RUN  0
#else
  #error "BuildConfig.h: BUILD_MODE must be BUILD_PROD, BUILD_DEV or BUILD_DEMO"
#endif
