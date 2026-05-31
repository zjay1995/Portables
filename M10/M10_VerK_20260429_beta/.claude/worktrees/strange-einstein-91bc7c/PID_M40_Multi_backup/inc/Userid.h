#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Userid
{
    std::array<int, 50> m_useridArray;

    void initializeUserIDArray() {
        for (int i = 0; i < m_useridArray.size(); ++i) {
            m_useridArray[i] = i;
        }
    }

    int m_selectedUserid = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;


public:

    Userid()
    {
        m_selectedUserid = 0;
    }

    ~Userid()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectUseridByValueNoEEPROMSave(int userid)
    {
        for(int i=0; i < m_useridArray.size(); i++)
            if(m_useridArray[i] == userid)
            {
                m_selectedUserid = i;
                break;
            }

        return;
    }

    void selectUseridByIndex(int index)
    {
        if(index >= 0 && index < m_useridArray.size())
        {
            m_selectedUserid = index;
            EEPROM.write(238, index);
            EEPROM.commit();
        }

        return;
    }

    void selectUseridByValue(int userid)
    {
        for(int i=0; i < m_useridArray.size(); i++)
            if(m_useridArray[i] == userid)
            {
                m_selectedUserid = i;
                break;
            }

        return;
    }

    void selectNextUserid()
    {
        m_selectedUserid = (m_selectedUserid + 1) % m_useridArray.size();
    }

    void selectPreviousUserid()
    {
        if(m_selectedUserid == 0)
            m_selectedUserid = m_useridArray.size() - 1;
        else
            m_selectedUserid = m_selectedUserid - 1;
    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedUserid() {
        int userid = m_selectedUserid;
        return m_useridArray[userid];
    }

};
