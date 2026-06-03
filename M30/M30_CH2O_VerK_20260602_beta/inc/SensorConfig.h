//   1. Comment out the currently active profile below.
//   2. Uncomment the profile that matches your installed sensor.
//   3. Re-compile and flash.

// --- Uncomment ONE profile ---

// PID (M10 / M30 PID front end)
// #include "sensors/PID_M10_100_1000.h"          // PID 100/1000 ppm dual range

// Electrochemical (EC) — single-cell M30 builds
//#include "sensors/EC_NH3_100ppm.h"           // Current: NH3   0–100 ppm
// #include "sensors/EC_H2S_50ppm.h"            // H2S   0–50 ppm
// #include "sensors/EC_H2S_100ppm.h"           // H2S   0–100 ppm
// #include "sensors/EC_H2S_2000ppm.h"          // H2S   0–2,000 ppm (special range)
// #include "sensors/EC_CO_300ppm.h"            // CO    0–300 ppm
// #include "sensors/EC_CO_1000ppm.h"           // CO    0–1,000 ppm
// #include "sensors/EC_CH2O_5ppm.h"            // CH2O  0–5.00 ppm   (IAQ)
#include "sensors/EC_CH2O_50ppm.h"           // Current: CH2O  0–50.00 ppm  (industrial)
// #include "sensors/EC_O2_30pct.h"             // O2    0–30 %       (special zero+span)
// #include "sensors/EC_C2H4_100ppm.h"          // C2H4  0–100 ppm
// #include "sensors/EC_O3_10ppm.h"             // O3    0–10 ppm
// #include "sensors/EC_SO2_20ppm.h"            // SO2   0–20 ppm
// #include "sensors/EC_NO2_20ppm.h"            // NO2   0–20 ppm
// #include "sensors/EC_HCl_10ppm.h"            // HCl   0–10 ppm
// #include "sensors/EC_HCN_100ppm.h"           // HCN   0–100 ppm
// #include "sensors/EC_Cl2_20ppm.h"            // Cl2   0–20 ppm
// #include "sensors/EC_HF_10ppm.h"             // HF    0–10 ppm
// #include "sensors/EC_ETO_100ppm.h"           // ETO   0–100 ppm
// #include "sensors/EC_PH3_10ppm.h"            // PH3   0–10 ppm     (special SW)
// #include "sensors/EC_CH3SH_10ppm.h"          // CH3SH 0–10 ppm     (M40)
// #include "sensors/EC_LEL_50pct.h"            // LEL   0–50 % LEL   (catalytic bead)
// #include "sensors/EC_4IN1.h"                  // CO + H2S + O2 + LEL composite

// NDIR — infrared single-gas (M12 platform)
// #include "sensors/NDIR_CO2_5000ppm.h"        // CO2   0–5,000 ppm  (IAQ)
// #include "sensors/NDIR_CO2_10000ppm.h"       // CO2   0–10,000 ppm

// PID — Chlorinated VOC (special-SW variant)
// #include "sensors/PID_CVOC_500ppm.h"         // CVOC  0–500 ppm
// #include "sensors/PID_CVOC_10000ppm.h"       // CVOC  0–10,000 ppm

// TCD — thermal conductivity (M10 platform)
// #include "sensors/TCD_H2_1pct.h"             // H2    0–1 % (0–10,000 ppm)

// Blank template
// #include "sensors/EC_TEMPLATE.h"              // Copy and rename to add a new sensor

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

// Per-sensor display defaults — fall back if profile header is older.
#ifndef SENSOR_UNIT_LABEL
  #define SENSOR_UNIT_LABEL  "ppm"
#endif
#ifndef SENSOR_RES_DIGITS
  #define SENSOR_RES_DIGITS  1
#endif
#ifndef SENSOR_RANGE_FS
  #define SENSOR_RANGE_FS    1000.0f
#endif
#ifndef SENSOR_OVERRANGE
  #define SENSOR_OVERRANGE   (SENSOR_RANGE_FS * 1.10f)
#endif

// Per-sensor menu ladders — fall back to legacy PID values if profile header
// hasn't been migrated yet. New profiles MUST define their own.
#ifndef SENSOR_GAS_NAME
  #define SENSOR_GAS_NAME    "GAS"
#endif
#ifndef SENSOR_RANGE_VALUES
  #define SENSOR_RANGE_VALUES    { 100.0f, 1000.0f }
#endif
#ifndef SENSOR_CALVALUE_VALUES
  #define SENSOR_CALVALUE_VALUES { 10.0f, 25.0f, 50.0f, 100.0f, 250.0f, 500.0f }
#endif
#ifndef SENSOR_ALARM_VALUES
  #define SENSOR_ALARM_VALUES    { 0.0f, 10.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f }
#endif
#ifndef SENSOR_PEL_VALUES
  #define SENSOR_PEL_VALUES      { 0.0f, 5.0f, 10.0f, 25.0f, 50.0f }
#endif
