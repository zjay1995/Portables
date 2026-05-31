#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include "SensorConfig.h"

// Built from the active sensor profile's SENSOR_CALVALUE_VALUES macro.
static const float k_calvalueArr[] = SENSOR_CALVALUE_VALUES;
static constexpr int k_calvalueCount = sizeof(k_calvalueArr) / sizeof(k_calvalueArr[0]);

class Calvalue
{
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
        for(int i=0; i < k_calvalueCount; i++)
            if((int)k_calvalueArr[i] == calvalue)
            {
                m_selectedCalvalue = i;
                break;
            }

        return;
    }

    void selectCalvalueByIndex(int index)
    {
        if(index >= 0 && index < k_calvalueCount)
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
        for(int i=0; i < k_calvalueCount; i++)
            if((int)k_calvalueArr[i] == calvalue)
            {
                m_selectedCalvalue = i;
                m_configurationManager->saveCalvalueToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextCalvalue()
    {
        m_selectedCalvalue = (m_selectedCalvalue + 1) % k_calvalueCount;
    }

    void selectPreviousCalvalue()
    {
        if(m_selectedCalvalue == 0)
            m_selectedCalvalue = k_calvalueCount - 1;
        else
            m_selectedCalvalue = m_selectedCalvalue - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedCalvalue() {
        int calvalue = EEPROM.read(80);
        if (calvalue < 0 || calvalue >= k_calvalueCount) calvalue = 0;
        return (int)k_calvalueArr[calvalue];
    }

};
