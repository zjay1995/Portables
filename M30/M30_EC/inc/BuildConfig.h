#pragma once
// =============================================================================
// M30-EC build configuration — the only file to edit before compiling.
//   1. SENSOR_PROFILE : which gas sensor is fitted
//   2. BUILD_MODE     : PROD / DEV / DEMO
//   3. Feature switches (rollback)
// Firmware version lives in inc/Version.h.
// =============================================================================

// =============================================================================
// 1. Sensor — set to the fitted sensor, then compile and flash.
// =============================================================================
//   EC_C2H4_100PPM   EC_CH2O_5PPM    EC_CH2O_50PPM   EC_CH3SH_10PPM
//   EC_CO_300PPM     EC_CO_1000PPM   EC_CL2_20PPM    EC_ETO_100PPM
//   EC_H2S_50PPM     EC_H2S_100PPM   EC_H2S_2000PPM  EC_HCN_100PPM
//   EC_HCL_10PPM     EC_HF_10PPM     EC_LEL_50PCT    EC_NH3_100PPM
//   EC_NO2_20PPM     EC_O2_30PCT     EC_O3_10PPM     EC_PH3_10PPM
//   EC_SO2_20PPM
// Each profile (inc/sensors/*.h) holds the sensor's range, ADC gain,
// sensitivity, cal-gas / alarm menus and display resolution.
// Flashing a different sensor makes the unit reload factory defaults and show
// CAL? until it is calibrated for the new sensor.
#ifndef SENSOR_PROFILE
  #define SENSOR_PROFILE  EC_CH2O_50PPM   // <-- fitted sensor
#endif

// =============================================================================
// 2. Build mode — change ONE line below, then compile and flash.
// =============================================================================
//   BUILD_PROD : shipped firmware. Real sensor, raw detector mV hidden on the
//                Run screen.
//   BUILD_DEV  : bench testing without a sensor. The detector voltage is
//                simulated (see SensorSimulator.h) so zero/span calibration,
//                alarms, logging and FAULT / CAL? / OVER behave as in the
//                field. Run screen shows "SIM"; serial accepts SIM commands.
//   BUILD_DEMO : trade-show demo. Run screen shows a random reading in the
//                profile's demo range (CH2O 50 ppm: 0.00-0.30 ppm); no sensor
//                or calibration needed.
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
  #define SHOW_RAW_MV_ON_RUN  1
#elif BUILD_MODE == BUILD_DEMO
  #define BUILD_MODE_TAG      ""        // header has no room for "CH3SH DEMO"...
  #define BUILD_FOOTER_TAG    "DEMO"    // ...so DEMO is shown where DEV shows the mV
  #define FW_MODE_NAME        "DEMO"
  #define SHOW_RAW_MV_ON_RUN  0
#elif BUILD_MODE == BUILD_PROD
  #define BUILD_MODE_TAG      ""
  #define FW_MODE_NAME        "PROD"
  #define SHOW_RAW_MV_ON_RUN  0
#else
  #error "BuildConfig.h: BUILD_MODE must be BUILD_PROD, BUILD_DEV or BUILD_DEMO"
#endif

// =============================================================================
// 3. Feature switches — set to 0 to roll a feature back to the original behaviour
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
// first repeat after 0.6 s, then 4 steps/s, 8 steps/s after 2.5 s held
// (full hour lap ~4.4 s, 0->59 minutes ~8.9 s). Tune with HOLD_SCROLL_* below.
// Releasing stops without an extra step; the value is still saved only by S.
// 0 = original behaviour (one step per press, holding does nothing).
#ifndef FEATURE_HOLD_TO_SCROLL
  #define FEATURE_HOLD_TO_SCROLL  1
#endif
#define HOLD_SCROLL_FIRST_MS      600   // hold this long before the first repeat
#define HOLD_SCROLL_STEP_MS       250   // then one step every 250 ms (4/s)
#define HOLD_SCROLL_FAST_AFTER_MS 2500  // speed up once held this long
#define HOLD_SCROLL_FAST_STEP_MS  125   // then one step every 125 ms (8/s)

// Settings-screen footer labels: SKIP (RIGHT key: next screen, unsaved change
// discarded) · SAVE (S key) · drawn down-arrow (DOWN key: next value).
// Bump Test shows SKIP · TEST. 0 = original MENU · SET · OPTION.
#ifndef FEATURE_NEW_FOOTER_LABELS
  #define FEATURE_NEW_FOOTER_LABELS  1
#endif
