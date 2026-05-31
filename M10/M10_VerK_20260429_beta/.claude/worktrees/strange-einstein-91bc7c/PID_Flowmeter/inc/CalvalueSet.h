#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Calvalue
{
    std::array<int,6> m_calvalueArray{{5000,6000,7000,8000,9000,10000}};

    int m_selectedCalvalue = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    //U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI* m_u8g2;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Calvalue()
    {
        m_selectedCalvalue = 0;
    }

    ~Calvalue()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectCalvalueByValueNoEEPROMSave(int calvalue)
    {
        for(int i=0; i < m_calvalueArray.size(); i++)
            if(m_calvalueArray[i] == calvalue)
            {
                m_selectedCalvalue = i;
                break;
            }

        return;
    }

    void selectCalvalueByIndex(int index)
    {
        if(index >= 0 && index < m_calvalueArray.size())
        {
            m_selectedCalvalue = index;
            EEPROM.writeInt(80, index);
            EEPROM.commit();
            Serial.print("Calvalue saved ");
            Serial.println(index);
        }

        return;
    }

    void selectCalvalueByValue(int calvalue)
    {
        for(int i=0; i < m_calvalueArray.size(); i++)
            if(m_calvalueArray[i] == calvalue)
            {
                m_selectedCalvalue = i;
                m_configurationManager->saveCalvalueToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextCalvalue()
    {
        m_selectedCalvalue = (m_selectedCalvalue + 1) % m_calvalueArray.size();
    }

    void selectPreviousCalvalue()
    {
        if(m_selectedCalvalue == 0)
            m_selectedCalvalue = m_calvalueArray.size() - 1;
        else
            m_selectedCalvalue = m_selectedCalvalue - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedCalvalue() {
        int calvalue = EEPROM.read(80);
        return m_calvalueArray[calvalue];
    }

};
