#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Lowalarm
{
    // Low-concentration alarm thresholds in ppm (0.0 = Off), 0 to 10000 in 500 ppm steps
    std::array<float, 21> m_lowalarmArray{{
        0.0f, 500.0f, 1000.0f, 1500.0f, 2000.0f, 2500.0f, 3000.0f, 3500.0f, 4000.0f, 4500.0f, 5000.0f,
        5500.0f, 6000.0f, 6500.0f, 7000.0f, 7500.0f, 8000.0f, 8500.0f, 9000.0f, 9500.0f, 10000.0f
    }};
    int m_selectedLowalarm = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    //U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI* m_u8g2;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Lowalarm()
    {
        m_selectedLowalarm = 0;
    }

    ~Lowalarm()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectLowalarmByValueNoEEPROMSave(float lowalarm)
    {
        for(int i=0; i < (int)m_lowalarmArray.size(); i++)
            if(m_lowalarmArray[i] == lowalarm)
            {
                m_selectedLowalarm = i;
                break;
            }

        return;
    }

    void selectLowalarmByIndex(int index)
    {
        if(index >= 0 && index < (int)m_lowalarmArray.size())
        {
            m_selectedLowalarm = index;
            EEPROM.writeInt(92, index);
            EEPROM.commit();
            Serial.print("Low Alarm saved ");
            Serial.println(index);
        }

        return;
    }

    void selectLowalarmByValue(float lowalarm)
    {
        for(int i=0; i < (int)m_lowalarmArray.size(); i++)
            if(m_lowalarmArray[i] == lowalarm)
            {
                m_selectedLowalarm = i;
                m_configurationManager->saveLowalarmToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextLowalarm()
    {
        m_selectedLowalarm = (m_selectedLowalarm + 1) % m_lowalarmArray.size();
    }

    void selectPreviousLowalarm()
    {
        if(m_selectedLowalarm == 0)
            m_selectedLowalarm = m_lowalarmArray.size() - 1;
        else
            m_selectedLowalarm = m_selectedLowalarm - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    float getSelectedLowalarm() {
        int idx = EEPROM.read(92);
        if (idx < 0 || idx >= (int)m_lowalarmArray.size()) idx = 0;
        return m_lowalarmArray[idx];
    }

};
