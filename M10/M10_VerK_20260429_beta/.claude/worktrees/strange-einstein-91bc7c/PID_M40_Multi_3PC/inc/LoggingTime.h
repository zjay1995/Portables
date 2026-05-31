#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Loggingtime
{
    std::array<int,5> m_loggingtimeArray{{0,1,2,3,4}};

    int m_selectedLoggingtime = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Loggingtime()
    {
        m_selectedLoggingtime = 0;
    }

    ~Loggingtime()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectLoggingtimeByValueNoEEPROMSave(int loggingtime)
    {
        for(int i=0; i < m_loggingtimeArray.size(); i++)
            if(m_loggingtimeArray[i] == loggingtime)
            {
                m_selectedLoggingtime = i;
                break;
            }

        return;
    }

    void selectLoggingtimeByIndex(int index)
    {
        if(index >= 0 && index < m_loggingtimeArray.size())
        {
            m_selectedLoggingtime = index;
            EEPROM.writeInt(86, index);
            EEPROM.commit();
            Serial.print("Loggingtime saved: ");
            Serial.println(index);
        }

        return;
    }

    void selectLoggingtimeByValue(int loggingtime)
    {
        for(int i=0; i < m_loggingtimeArray.size(); i++)
            if(m_loggingtimeArray[i] == loggingtime)
            {
                m_selectedLoggingtime = i;
                break;
            }

        return;
    }

    void selectNextLoggingtime()
    {
        m_selectedLoggingtime = (m_selectedLoggingtime + 1) % m_loggingtimeArray.size();
    }

    void selectPreviousLoggingtime()
    {
        if(m_selectedLoggingtime == 0)
            m_selectedLoggingtime = m_loggingtimeArray.size() - 1;
        else
            m_selectedLoggingtime = m_selectedLoggingtime - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedLoggingtime() {
        int loggingtime = EEPROM.read(200);
        return m_loggingtimeArray[loggingtime];
    }

};
