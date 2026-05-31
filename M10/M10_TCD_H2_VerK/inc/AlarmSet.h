#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Alarm
{
    // High-alarm thresholds in ppm (0.0 = Off)
    std::array<float, 13> m_alarmArray{{
        0.0f,
        100.0f, 500.0f, 1000.0f, 2000.0f, 3000.0f, 4000.0f,
        5000.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f, 10000.0f
    }};
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

    void selectAlarmByValueNoEEPROMSave(float alarm)
    {
        for(int i=0; i < (int)m_alarmArray.size(); i++)
            if(m_alarmArray[i] == alarm)
            {
                m_selectedAlarm = i;
                break;
            }

        return;
    }

    void selectAlarmByIndex(int index)
    {
        if(index >= 0 && index < (int)m_alarmArray.size())
        {
            m_selectedAlarm = index;
            EEPROM.writeInt(76, index);
            EEPROM.commit();
            Serial.print("Alarm saved ");
            Serial.println(index);
        }

        return;
    }

    void selectAlarmByValue(float alarm)
    {
        for(int i=0; i < (int)m_alarmArray.size(); i++)
            if(m_alarmArray[i] == alarm)
            {
                m_selectedAlarm = i;
                m_configurationManager->saveAlarmToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextAlarm()
    {
        m_selectedAlarm = (m_selectedAlarm + 1) % m_alarmArray.size();
    }

    void selectPreviousAlarm()
    {
        if(m_selectedAlarm == 0)
            m_selectedAlarm = m_alarmArray.size() - 1;
        else
            m_selectedAlarm = m_selectedAlarm - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    float getSelectedAlarm() {
        int idx = EEPROM.read(76);
        if (idx < 0 || idx >= (int)m_alarmArray.size()) idx = 0;
        return m_alarmArray[idx];
    }

};
