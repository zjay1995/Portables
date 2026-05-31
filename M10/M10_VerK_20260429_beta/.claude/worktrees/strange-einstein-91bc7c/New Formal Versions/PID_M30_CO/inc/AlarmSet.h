#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Alarm
{
    std::array<int, 56> m_alarmArray{{
                                             // 10 to 500 (Step 10)
                                             0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
                                             110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
                                             210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
                                             310, 320, 330, 340, 350, 360, 370, 380, 390, 400,
                                             410, 420, 430, 440, 450, 460, 470, 480, 490, 500,
                                             // 600 to 1000 (Step 100)
                                             600, 700, 800, 900, 1000
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

    void selectAlarmByValueNoEEPROMSave(int alarm)
    {
        for(int i=0; i < m_alarmArray.size(); i++)
            if(m_alarmArray[i] == alarm)
            {
                m_selectedAlarm = i;
                break;
            }

        return;
    }

    void selectAlarmByIndex(int index)
    {
        if(index >= 0 && index < m_alarmArray.size())
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
        for(int i=0; i < m_alarmArray.size(); i++)
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

    int getSelectedAlarm() {
        int alarm = EEPROM.read(76);
        return m_alarmArray[alarm];
    }

};
