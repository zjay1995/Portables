#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>
#include "GasManager.h"

class Range
{
    // Full-scale in sccm. Index 0 = 500 sccm, index 1 = 1000 sccm.
    std::array<float, 2> m_rangeArray{{500.0f, 1000.0f}};  // sccm full-scale
    int m_selectedRange = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    //U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI* m_u8g2;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Range()
    {
        m_selectedRange = 0;
    }

    ~Range()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectRangeByValueNoEEPROMSave(float range)
    {
        for(int i=0; i < (int)m_rangeArray.size(); i++)
            if(m_rangeArray[i] == range)
            {
                m_selectedRange = i;
                break;
            }

        return;
    }

    void selectRangeByIndex(int index)
    {
        if(index >= 0 && index < (int)m_rangeArray.size())
        {
            m_selectedRange = index;
            EEPROM.writeInt(72, index);
            EEPROM.commit();
            Serial.print("Range saved: ");
            Serial.println(index);
        }

        return;
    }

    void selectRangeByValue(float range)
    {
        for(int i=0; i < (int)m_rangeArray.size(); i++)
            if(m_rangeArray[i] == range)
            {
                m_selectedRange = i;
                m_configurationManager->saveRangeToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextRange()
    {
        m_selectedRange = (m_selectedRange + 1) % m_rangeArray.size();
    }

    void selectPreviousRange()
    {
        if(m_selectedRange == 0)
            m_selectedRange = m_rangeArray.size() - 1;
        else
            m_selectedRange = m_selectedRange - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    float getSelectedRange() {
        int idx = EEPROM.read(72);
        if (idx < 0 || idx >= (int)m_rangeArray.size()) idx = 0;
        return m_rangeArray[idx];
    }

    void onParamChange(String param, String value){
        Serial.println("onRangeParamChange: " + param + "=" + value);

        if(param.equals(c_RANGE_PARAM_NAME))
        {
            m_selectedRange = value.toInt();
            Serial.println("Setting Range: " + value);
        }
    };

};
