#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>
#include "GasManager.h"
#include "SensorConfig.h"

// Built from the active sensor profile's SENSOR_RANGE_VALUES macro.
static const float k_rangeArr[] = SENSOR_RANGE_VALUES;
static constexpr int k_rangeCount = sizeof(k_rangeArr) / sizeof(k_rangeArr[0]);

class Range
{
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

    void selectRangeByValueNoEEPROMSave(int range)
    {
        for(int i=0; i < k_rangeCount; i++)
            if(k_rangeArr[i] == range)
            {
                m_selectedRange = i;
                break;
            }

        return;
    }

    void selectRangeByIndex(int index)
    {
        if(index >= 0 && index < k_rangeCount)
        {
            m_selectedRange = index;
            EEPROM.writeInt(72, index);
            EEPROM.commit();
            Serial.print("Range saved: ");
            Serial.println(index);
        }

        return;
    }

    void selectRangeByValue(int range)
    {
        for(int i=0; i < k_rangeCount; i++)
            if(k_rangeArr[i] == range)
            {
                m_selectedRange = i;
                m_configurationManager->saveRangeToEEPROM(i);
                break;
            }

        return;
    }

    void selectNextRange()
    {
        m_selectedRange = (m_selectedRange + 1) % k_rangeCount;
    }

    void selectPreviousRange()
    {
        if(m_selectedRange == 0)
            m_selectedRange = k_rangeCount - 1;
        else
            m_selectedRange = m_selectedRange - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedRange() {
        int range = EEPROM.read(72);
        if (range < 0 || range >= k_rangeCount) range = 0;
        return (int)k_rangeArr[range];
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
