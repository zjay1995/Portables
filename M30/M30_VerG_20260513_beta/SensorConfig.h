#pragma once
// =============================================================================
// SensorConfig.h — Active sensor profile selector
// =============================================================================
// HOW TO SWAP SENSORS:
//   1. Comment out the currently active profile below.
//   2. Uncomment the profile that matches your installed sensor.
//   3. Open the .ino and update the five "menu config" sections
//      (gas list, range menu, alarm menu, PEL alarm menu, calvalue menu)
//      to match the values documented in the new profile header.
//   4. Re-compile and flash.
//
// The GasManager constructor, BT name, and ADS gain are wired to the
// #defines below and will change automatically — no manual edit needed for those.
// =============================================================================

// --- Uncomment ONE profile ---
#include "inc/sensors/PID_M10_100_1000.h"       // Current: PID 100/1000 ppm dual range
// #include "inc/sensors/EC_CO_300ppm.h"         // EC: CO  0–300 ppm
// #include "inc/sensors/EC_H2S_50ppm.h"         // EC: H2S 0–50 ppm
// #include "inc/sensors/EC_TEMPLATE.h"           // Blank template for a new sensor

// =============================================================================
// Sanity-check: verify a profile was included
// =============================================================================
#ifndef SENSOR_BT_NAME
  #error "SensorConfig.h: No sensor profile selected. Uncomment one #include above."
#endif
#ifndef SENSOR_ADS_GAIN
  #error "SensorConfig.h: SENSOR_ADS_GAIN not defined. Check sensor profile header."
#endif
#ifndef SENSOR_SLOPE
  #error "SensorConfig.h: SENSOR_SLOPE not defined. Check sensor profile header."
#endif
