#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Calvalue
{
    // Reference flow values in sccm for span calibration.
    // MUST match the calValues[] list in the .ino LIBRARY/Calvalue menu.
    std::array<float, 7> m_calvalueArray{{
        50.0f, 100.0f, 200.0f, 250.0f, 500.0f, 750.0f, 1000.0f
    }};
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

    void selectCalvalueByValueNoEEPROMSave(float calvalue)
    {
        for(int i=0; i < (int)m_calvalueArray.size(); i++)
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

    void selectCalvalueByValue(float calvalue)
    {
        for(int i=0; i < (int)m_calvalueArray.size(); i++)
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

    float getSelectedCalvalue() {
        int idx = EEPROM.read(80);
        if (idx < 0 || idx >= (int)m_calvalueArray.size()) idx = 0;
        return m_calvalueArray[idx];
    }

};
