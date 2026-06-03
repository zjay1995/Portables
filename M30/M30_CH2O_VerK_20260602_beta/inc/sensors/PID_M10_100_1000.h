#pragma once
// =============================================================================
// Sensor Profile: PID — Dual Range  100 ppm / 1000 ppm
// =============================================================================
// Sensor type : Photoionization Detector (PID)
// Application : VOC detection, two selectable sensor ranges
// Output      : Voltage via transimpedance amp → ADS1115
// Unit        : ppm
// Last updated: 2026-05-14
// =============================================================================

// --- Bluetooth / identity ---
#define SENSOR_BT_NAME      "ESP32 PID M10"
#define SENSOR_UNIT_LABEL   "ppm"

// --- ADS1115 gain ---
// GAIN_ONE  = ±4.096 V  → 0.1250 mV/LSB   (default, wider range)
// GAIN_TWO  = ±2.048 V  → 0.0625 mV/LSB
// GAIN_FOUR = ±1.024 V  → 0.0313 mV/LSB
#define SENSOR_ADS_GAIN     GAIN_ONE

// --- GasManager default intercept / slope (factory defaults) ---
// These are the fallback values used before the first field calibration.
// Field calibration overwrites these values in EEPROM.
//
// Slot 1 — "100" range sensor
#define SENSOR_INTERCEPT    -2.054456771
#define SENSOR_SLOPE        1.0
// Slot 2 — "1000" range sensor
#define SENSOR_INTERCEPT2   0.0
#define SENSOR_SLOPE2       1.0
// Slots 3 & 4 — unused; keep at (0, 1) to avoid divide-by-zero
#define SENSOR_INTERCEPT3   0.0
#define SENSOR_SLOPE3       1.0
#define SENSOR_INTERCEPT4   0.0
#define SENSOR_SLOPE4       1.0

// =============================================================================
// Menu configuration — edit these sections in the .ino when swapping sensors
// =============================================================================

// GasManager addGas() calls:
//   g_gasManager.addGas(Gas("100",  1.0));
//   g_gasManager.addGas(Gas("1000", 1.0));

// Gas library (LIBRARY menu) entries:
//   GasMenuItem("100",  "LIBRARY", 0, ...)
//   GasMenuItem("1000", "LIBRARY", 1, ...)

// Range menu options:
//   RangeMenuItem("5000 ppm",  "Range", 0, ...)
//   RangeMenuItem("10000 ppm", "Range", 1, ...)
//   → AlarmSet.h / RangeSet.h arrays must match

// Calvalue menu (cal gas concentration options):
//   10, 20, 30, 40, 50, 60, 70, 80, 90, 100,   ← step 10
//   200, 300, 400, 500                           ← step 100
//   → CalvalueSet.h must match

// High alarm menu:
//   0 (Off), 10–500 step 10, 600–1000 step 100  (56 entries)
//   → AlarmSet.h must match

// PEL alarm menu:
//   0 (Off), 1–60 ppm step 1                    (61 entries)
//   → LowAlarmSet.h must match

// OSHA reference (common VOCs):
//   Benzene PEL: 1 ppm TWA
//   Isobutylene: no PEL; set PEL alarm to 0 (Off) if unused

// --- Display + identity + menu ladders (per master sensor spec) ---
#define SENSOR_RES_DIGITS       1
#define SENSOR_RANGE_FS         1000.0f
#define SENSOR_OVERRANGE        1100.0f
#define SENSOR_GAS_NAME         "VOC"
#define SENSOR_RANGE_VALUES     { 100.0f, 1000.0f }
#define SENSOR_CALVALUE_VALUES  { 10.0f, 25.0f, 50.0f, 100.0f, 250.0f, 500.0f }
#define SENSOR_ALARM_VALUES     { 0.0f, 10.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f }
#define SENSOR_PEL_VALUES       { 0.0f, 1.0f, 5.0f, 10.0f, 25.0f, 50.0f }
