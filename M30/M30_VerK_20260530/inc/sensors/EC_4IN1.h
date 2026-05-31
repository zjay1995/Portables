#pragma once
// =============================================================================
// Sensor Profile: 4-in-1 Composite — CO + H2S + O2 + LEL
// =============================================================================
// Sensor type : Four parallel EC + catalytic-bead cells in one head
// Hardware    : M30 4-in-1 confined-space monitor
// Unit        : per-slot (ppm or %)
// Resolution  : 0.1 ppm for CO/H2S, 0.01 % for O2 and LEL
// Over-range  : per-slot (110 ppm CO/H2S, 33 % O2, 55 % LEL)
//
// NOTE: This is a multi-sensor profile. Each of the four cells uses its own
// EEPROM slot for intercept/slope; the LIBRARY menu selects which sensor's
// reading is shown on the run screen. Range / alarm / cal-value menus all
// switch with the active gas selection.
//
// Slot map:
//   Slot 0 = CO  (0–500 ppm  EC)
//   Slot 1 = H2S (0–100 ppm  EC)
//   Slot 2 = O2  (0–30 %     EC, depleting cell)
//   Slot 3 = LEL (0–100 %LEL catalytic bead)
//
// Last updated: 2026-05-26
// =============================================================================

#define SENSOR_BT_NAME        "ESP32 4-in-1"
#define SENSOR_UNIT_LABEL     "ppm"   // overridden per-slot by the run renderer
#define SENSOR_RES_DIGITS     1
#define SENSOR_RANGE_FS       500.0f  // CO slot 0 default; slot switch overrides
#define SENSOR_OVERRANGE      550.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR

// Per-slot defaults — calibrated in field per cell
#define SENSOR_INTERCEPT      0.0
#define SENSOR_SLOPE          1.0
#define SENSOR_INTERCEPT2     0.0
#define SENSOR_SLOPE2         1.0
#define SENSOR_INTERCEPT3     0.0
#define SENSOR_SLOPE3         1.0
#define SENSOR_INTERCEPT4     0.0
#define SENSOR_SLOPE4         1.0

//   addGas:    Gas("CO",  1.0)
//              Gas("H2S", 1.0)
//              Gas("O2",  1.0)
//              Gas("LEL", 1.0)
//
//   Per-slot ranges, alarms, calvalues are set in the .ino via the LIBRARY
//   menu selection. Recommended starting values:
//     CO  alarm 35 ppm (NIOSH REL),  PEL 25
//     H2S alarm 10 ppm,              PEL 1
//     O2  alarm 19.5 / 23.5 %        (deficient / enriched)
//     LEL alarm 10 % LEL (warning),  25 % (evacuate)

// --- Identity + menu ladders (per master sensor spec) ---
// NOTE: single-ladder menu shows the CO slot (slot 0) by default. True
// per-slot 4-in-1 switching is not yet wired into the menu system.
#define SENSOR_GAS_NAME         "CO"
#define SENSOR_RANGE_VALUES     { 500.0f }
#define SENSOR_CALVALUE_VALUES  { 25.0f, 50.0f, 100.0f, 250.0f, 500.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 35.0f, 50.0f, 100.0f, 200.0f, 500.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 25.0f, 35.0f, 50.0f }
