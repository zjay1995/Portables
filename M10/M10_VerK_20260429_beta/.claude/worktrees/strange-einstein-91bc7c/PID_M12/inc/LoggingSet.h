#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Loggingset
{
    std::array<int,4> m_loggingsetArray{{0,1,2,3}};

    int m_selectedLoggingset = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Loggingset()
    {
        m_selectedLoggingset = 0;
    }

    ~Loggingset()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectLoggingsetByValueNoEEPROMSave(int loggingset)
    {
        for(int i=0; i < m_loggingsetArray.size(); i++)
            if(m_loggingsetArray[i] == loggingset)
            {
                m_selectedLoggingset = i;
                break;
            }

        return;
    }

    void selectLoggingsetByIndex(int index)
    {
        if(index >= 0 && index < m_loggingsetArray.size())
        {
            m_selectedLoggingset = index;
            EEPROM.writeInt(86, index);
            EEPROM.commit();
            Serial.print("Loggingset saved: ");
            Serial.println(index);
        }

        return;
    }

    void selectLoggingsetByValue(int loggingset)
    {
        for(int i=0; i < m_loggingsetArray.size(); i++)
            if(m_loggingsetArray[i] == loggingset)
            {
                m_selectedLoggingset = i;
                break;
            }

        return;
    }

    void selectNextLoggingset()
    {
        m_selectedLoggingset = (m_selectedLoggingset + 1) % m_loggingsetArray.size();
    }

    void selectPreviousLoggingset()
    {
        if(m_selectedLoggingset == 0)
            m_selectedLoggingset = m_loggingsetArray.size() - 1;
        else
            m_selectedLoggingset = m_selectedLoggingset - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedLoggingset() {
        int loggingset = EEPROM.read(96);
        return m_loggingsetArray[loggingset];
    }

};
