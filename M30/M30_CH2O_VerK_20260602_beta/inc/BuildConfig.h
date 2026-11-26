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

// =============================================================================
// Feature switches — set to 0 to roll a feature back to the original behaviour
// without touching the code. Independent of BUILD_MODE.
// =============================================================================

// DOWN / S / RIGHT accept any press of 30-800 ms as a click.
// 0 = original window: only presses of 110-310 ms count (quick taps and
//     longer presses are silently ignored). The ON (power) button always
//     keeps the original window.
#ifndef FEATURE_WIDE_CLICK_WINDOW
  #define FEATURE_WIDE_CLICK_WINDOW  1
#endif

// Holding DOWN on the Hour / Minute screens keeps stepping the value:
// first repeat after 0.5 s, then ~5 steps/s, ~12 steps/s after 2 s held.
// Releasing stops without an extra step; the value is still saved only by S.
// 0 = original behaviour (one step per press, holding does nothing).
#ifndef FEATURE_HOLD_TO_SCROLL
  #define FEATURE_HOLD_TO_SCROLL  1
#endif
