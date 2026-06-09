#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Lowalarm
{
    std::array<int, 61> m_lowalarmArray{{
                                                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                                21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                                31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                                41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                                                51, 52, 53, 54, 55, 56, 57, 58, 59, 60
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

    void selectLowalarmByValueNoEEPROMSave(int lowalarm)
    {
        for(int i=0; i < m_lowalarmArray.size(); i++)
            if(m_lowalarmArray[i] == lowalarm)
            {
                m_selectedLowalarm = i;
                break;
            }

        return;
    }

    void selectLowalarmByIndex(int index)
    {
        if(index >= 0 && index < m_lowalarmArray.size())
        {
            m_selectedLowalarm = index;
            EEPROM.writeInt(92, index);
            EEPROM.commit();
            Serial.print("Low Alarm saved ");
            Serial.println(index);
        }

        return;
    }

    void selectLowalarmByValue(int lowalarm)
    {
        for(int i=0; i < m_lowalarmArray.size(); i++)
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

    int getSelectedLowalarm() {
        int lowalarm = EEPROM.read(92);
        if (lowalarm < 0 || lowalarm >= (int)m_lowalarmArray.size()) lowalarm = 0;  // blank EEPROM → 255
        return m_lowalarmArray[lowalarm];
    }

};
