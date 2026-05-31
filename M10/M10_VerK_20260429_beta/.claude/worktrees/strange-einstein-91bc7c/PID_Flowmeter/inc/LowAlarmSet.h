#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Lowalarm
{
    std::array<int,62> m_lowalarmArray{{0, 500, 525, 550, 575, 600, 625, 650, 675, 700, 725, 750, 775, 800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100, 1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500, 1525, 1550, 1575, 1600, 1625, 1650, 1675, 1700, 1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900, 1925, 1950, 1975, 2000}};

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
        return m_lowalarmArray[lowalarm];
    }

};
