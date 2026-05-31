#pragma once

#include <array>
#include "ConfigurationManager.h"
#include "Globals.h"
#include <WiFi.h>
#include <RTClib.h>

void NTPTimeSync_Task(void* param);

class TimeSync : public ParamChangeListener
{
    String m_wifiSsid;
    String m_wifiPassword;

    RTC_PCF8563 m_rtc;

    bool m_isNTPSyncRunning = false;

    TaskHandle_t m_task;

public:

    TimeSync();
    ~TimeSync() = default;

    void initTimeFromRTC();

    void stopNTPSync();

    void startNTPSync();

    bool isNTCSyncRunning() const;  // Declare the missing function

    void NTPSyncTask_run();

    void onParamChange(String param, String value);
};
