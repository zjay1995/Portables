#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include "SensorConfig.h"

// Built from the active sensor profile's SENSOR_PEL_VALUES macro.
static const float k_lowalarmArr[] = SENSOR_PEL_VALUES;
static constexpr int k_lowalarmCount = sizeof(k_lowalarmArr) / sizeof(k_lowalarmArr[0]);

class Lowalarm
{
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
        for(int i=0; i < k_lowalarmCount; i++)
            if((int)k_lowalarmArr[i] == lowalarm)
            {
                m_selectedLowalarm = i;
                break;
            }

        return;
    }

    void selectLowalarmByIndex(int index)
    {
        if(index >= 0 && index < k_lowalarmCount)
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
        for(int i=0; i < k_lowalarmCount; i++)
            if((int)k_lowalarmArr[i] == lowalarm)
            {
                m_selectedLowalarm = i;
                m_configurationManager->saveLowalarmToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextLowalarm()
    {
        m_selectedLowalarm = (m_selectedLowalarm + 1) % k_lowalarmCount;
    }

    void selectPreviousLowalarm()
    {
        if(m_selectedLowalarm == 0)
            m_selectedLowalarm = k_lowalarmCount - 1;
        else
            m_selectedLowalarm = m_selectedLowalarm - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedLowalarm() {
        int lowalarm = EEPROM.read(92);
        if (lowalarm < 0 || lowalarm >= k_lowalarmCount) lowalarm = 0;
        return (int)k_lowalarmArr[lowalarm];
    }

};
