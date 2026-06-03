#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include "SensorConfig.h"

// Built from the active sensor profile's SENSOR_ALARM_VALUES macro.
static const float k_alarmArr[] = SENSOR_ALARM_VALUES;
static constexpr int k_alarmCount = sizeof(k_alarmArr) / sizeof(k_alarmArr[0]);

class Alarm
{
    int m_selectedAlarm = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    //U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI* m_u8g2;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Alarm()
    {
        m_selectedAlarm = 0;
    }

    ~Alarm()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectAlarmByValueNoEEPROMSave(int alarm)
    {
        for(int i=0; i < k_alarmCount; i++)
            if((int)k_alarmArr[i] == alarm)
            {
                m_selectedAlarm = i;
                break;
            }

        return;
    }

    void selectAlarmByIndex(int index)
    {
        if(index >= 0 && index < k_alarmCount)
        {
            m_selectedAlarm = index;
            EEPROM.writeInt(76, index);
            EEPROM.commit();
            Serial.print("Alarm saved ");
            Serial.println(index);
        }

        return;
    }

    void selectAlarmByValue(int alarm)
    {
        for(int i=0; i < k_alarmCount; i++)
            if((int)k_alarmArr[i] == alarm)
            {
                m_selectedAlarm = i;
                m_configurationManager->saveAlarmToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextAlarm()
    {
        m_selectedAlarm = (m_selectedAlarm + 1) % k_alarmCount;
    }

    void selectPreviousAlarm()
    {
        if(m_selectedAlarm == 0)
            m_selectedAlarm = k_alarmCount - 1;
        else
            m_selectedAlarm = m_selectedAlarm - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedAlarm() {
        int alarm = EEPROM.read(76);
        if (alarm < 0 || alarm >= k_alarmCount) alarm = 0;
        return (int)k_alarmArr[alarm];
    }

};
