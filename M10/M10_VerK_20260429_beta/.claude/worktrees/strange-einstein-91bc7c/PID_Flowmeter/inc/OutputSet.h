#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Outport
{
    std::array<int,4> m_outportArray{{0,1,2,3}};

    int m_selectedOutport = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Outport()
    {
        m_selectedOutport = 0;
    }

    ~Outport()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectOutportByValueNoEEPROMSave(int outport)
    {
        for(int i=0; i < m_outportArray.size(); i++)
            if(m_outportArray[i] == outport)
            {
                m_selectedOutport = i;
                break;
            }

        return;
    }

    void selectOutportByIndex(int index)
    {
        if(index >= 0 && index < m_outportArray.size())
        {
            m_selectedOutport = index;
            EEPROM.writeInt(88, index);
            EEPROM.commit();
            Serial.print("Outport saved: ");
            Serial.println(index);
        }

        return;
    }

    void selectOutportByValue(int outport)
    {
        for(int i=0; i < m_outportArray.size(); i++)
            if(m_outportArray[i] == outport)
            {
                m_selectedOutport = i;
                break;
            }

        return;
    }

    void selectNextOutport()
    {
        m_selectedOutport = (m_selectedOutport + 1) % m_outportArray.size();
    }

    void selectPreviousOutport()
    {
        if(m_selectedOutport == 0)
            m_selectedOutport = m_outportArray.size() - 1;
        else
            m_selectedOutport = m_selectedOutport - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedOutport() {
        int outport = EEPROM.read(88);
        return m_outportArray[outport];
    }

};
