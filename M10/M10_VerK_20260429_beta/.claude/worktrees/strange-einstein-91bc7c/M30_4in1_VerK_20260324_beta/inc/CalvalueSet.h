#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>

class Calvalue
{
    std::array<double, 7> m_calvalueArray{{
        10.0,   // Common for H2S
        20.0,   // Common for H2S/CO
        20.9,   // Standard Ambient O2
        25.0,   // Common for H2S
        50.0,   // Common for CO/LEL
        100.0,  // Common for CO
        200.0,  // Industrial CO

    }};

    // FIX: Changed from double to int because this stores the array INDEX
    int m_selectedCalvalue = 0;

    unsigned long m_startMillis = 0;
    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;

public:
    Calvalue()
    {
        m_selectedCalvalue = 0;
    }

    ~Calvalue() = default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    // Helper to get a value from the array by index (used for menu labeling)
    double getCalvalueByIndex(int index) {
        if (index < 0 || index >= (int)m_calvalueArray.size()) return 0.0;
        return m_calvalueArray[index];
    }

    void selectCalvalueByValueNoEEPROMSave(double calvalue)
    {
        for(int i = 0; i < (int)m_calvalueArray.size(); i++) {
            if(m_calvalueArray[i] == calvalue)
            {
                m_selectedCalvalue = i;
                break;
            }
        }
    }

    void selectCalvalueByIndex(int index)
    {
        if(index >= 0 && index < (int)m_calvalueArray.size())
        {
            m_selectedCalvalue = index;
            // Ensure consistency with writeInt/readInt
            EEPROM.writeInt(80, index);
            EEPROM.commit();
            Serial.print("Calvalue index saved: ");
            Serial.println(index);
        }
    }

    void selectCalvalueByValue(double calvalue)
    {
        for(int i = 0; i < (int)m_calvalueArray.size(); i++) {
            if(m_calvalueArray[i] == calvalue)
            {
                m_selectedCalvalue = i;
                m_configurationManager->saveCalvalueToEEPROM(i);
                break;
            }
        }
    }

    void selectNextCalvalue()
    {
        // Modulo now works because m_selectedCalvalue is an int
        m_selectedCalvalue = (m_selectedCalvalue + 1) % (int)m_calvalueArray.size();
    }

    void selectPreviousCalvalue()
    {
        if(m_selectedCalvalue == 0)
            m_selectedCalvalue = (int)m_calvalueArray.size() - 1;
        else
            m_selectedCalvalue = m_selectedCalvalue - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    double getSelectedCalvalue() {
        // FIX: Read as integer index and ensure it is within bounds
        int index = EEPROM.readInt(80);
        if (index < 0 || index >= (int)m_calvalueArray.size()) index = 0;
        return m_calvalueArray[index];
    }
};