#include "inc/Globals.h"

#include "inc/ConfigurationManager.h"

// Firmware version, shown on the boot splash and the Cal Info screen.
// Update the date when releasing a new build.
const char* c_FIRMWARE_NAME    = "M103";
const char* c_FIRMWARE_VERSION = "07/08/26";

const uint8_t c_BUTTON_DOWN_PIN = 13;
const uint8_t c_BUTTON_S_PIN = 21;
const uint8_t c_BUTTON_RIGHT_PIN = 14;
const uint8_t c_BUTTON_ON_PIN = 12;

const uint8_t c_BUTTON_SINGLE_CLICK_HOLD_DURATION = 110;
const uint16_t c_BUTTON_COMBO_CLICK_HOLD_DURATION = 1750;

const char* c_FLASH_LOG_FREQ_PARAM_NAME = "flashLogFreq";

const char* c_GASINDEX_PARAM_NAME = "GASINDEX";
const char* c_RANGE_PARAM_NAME = "RANGE";
const char* c_ALARM_PARAM_NAME = "ALARM";
const char* c_LOWALARM_PARAM_NAME = "LOWALARM";

const char* c_INTERCEPT_PARAM_NAME = "INTERCEPT";
const char* c_SLOPE_PARAM_NAME = "SLOPE";

const char* c_INTERCEPT2_PARAM_NAME = "INTERCEPT2";
const char* c_SLOPE2_PARAM_NAME = "SLOPE2";

const char* c_INTERCEPT3_PARAM_NAME = "INTERCEPT3";
const char* c_SLOPE3_PARAM_NAME = "SLOPE3";

const char* c_INTERCEPT4_PARAM_NAME = "INTERCEPT4";
const char* c_SLOPE4_PARAM_NAME = "SLOPE4";


const char* c_GAS_AIR_PARAM_NAME = "Air";
const char* c_GAS_O2_PARAM_NAME = "O2";
const char* c_GAS_N2_PARAM_NAME = "N2";
const char* c_GAS_He_PARAM_NAME = "He";
const char* c_GAS_H2_PARAM_NAME = "H2";
const char* c_GAS_ARCH4_PARAM_NAME = "ArCH4";


ConfigurationManager g_configurationManager;
