// M103 board pin map (2026-03-02 beta)
//
// Tpad keypad
//   Tpad Up    -> IO 14   (still named c_BUTTON_RIGHT_PIN internally)
//   Tpad Down  -> IO 13
//   Tpad S     -> IO 21
//   Tpad G     -> removed; replaced by power switch on IO 12
//
// Sensor / analog
//   ALARM      -> IO 15   (digital out, alarm relay/buzzer drive)
//   VBATT      -> IO 34   (ADC, via 100K/100K divider)
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
#define BUTTON_PWR_PIN    12

#define ALARM_PIN         15
#define VBATT_ADC_PIN     34
#define LAMPOUT_PIN       16
#define PID_GAIN_PIN      17   // reserved, not driven by firmware
#define HVON_STATUS_PIN   36

#define BUTTON_SINGLE_CLICK_HOLD_DURATION 250
#define BUTTON_COMBO_CLICK_HOLD_DURATION 3000
