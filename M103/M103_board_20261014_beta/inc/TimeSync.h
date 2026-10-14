#pragma once

#include <RTClib.h>
#include <EEPROM.h>
#include "ConfigurationManager.h"

// EEPROM layout (5 bytes): year-2000 | month | day | hour | minute
#define EEPROM_TIME_BACKUP_OFFSET  240

class TimeSync : public ParamChangeListener
{
    RTC_PCF8563 m_rtc;

public:
    TimeSync() = default;
    ~TimeSync() = default;

    void initTimeFromRTC();

    void onParamChange(String param, String value) {}
};
