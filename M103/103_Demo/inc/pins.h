// M103 board pin map (2026-03-02 beta)
//
// Tpad keypad
//   Tpad Up    -> IO 14   (still named c_BUTTON_RIGHT_PIN internally)
//   Tpad Down  -> IO 13
//   Tpad S     -> IO 21
//   Tpad G     -> not connected to the ESP32 (hardware power switch)
//
// Sensor / analog
//   ALARM      -> IO 2    (digital out, buzzer via MOSFET; needs pull-down)
//   PWR_OFF    -> IO 25   (digital out, HIGH = cut 3.3 V; needs pull-down)
//   LAMPOUT    -> IO 16   (digital out, PID UV lamp enable, active-high)
//   PID_GAIN   -> IO 17   (schematic net only — gain select unused; firmware
//                          does not drive it, range is display/over-range only)
//   HVON/OFF   -> IO 36   (SENSOR_VP, INPUT ONLY — HV status read)
//
// I2C bus (RTC PCF8563, ADS1115, INA219, SHT4x)
//   SDA        -> IO 23
//   SCL        -> IO 22
//
// NOTE: The keypad pin values used at runtime are in Globals.cpp
// (c_BUTTON_*_PIN externs). The macros below are sync'd as documentation.
#define BUTTON_UP_PIN     14
#define BUTTON_DOWN_PIN   13
#define BUTTON_S_PIN      21

#define ALARM_PIN         2
#define PWR_OFF_PIN       25
#define LAMPOUT_PIN       16
#define PID_GAIN_PIN      17   // reserved, not driven by firmware
#define HVON_STATUS_PIN   36

#define BUTTON_SINGLE_CLICK_HOLD_DURATION 250
#define BUTTON_COMBO_CLICK_HOLD_DURATION 3000
