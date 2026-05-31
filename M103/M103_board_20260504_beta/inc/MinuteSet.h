#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include <RTClib.h>

class Minute
{
    std::array<int,60> m_minuteArray{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59}};
    int m_selectedMinute = 0;

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;
    RTC_PCF8563 m_rtc;

public:

    Minute()
    {
        m_selectedMinute = 0;
    }

    ~Minute()=default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2)
    {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectMinuteByIndex(int index)
    {
        if(index >= 0 && index < m_minuteArray.size())
        {
            m_selectedMinute = index;
            EEPROM.writeInt(230, index);
            EEPROM.commit();
            Serial.print("Minute saved ");
            Serial.println(index);

            if (!m_rtc.begin(&Wire)) {
                Serial.println("RTC not initialized. Aborting hour adjustment.");
                return;
            }
            DateTime now = m_rtc.now();
            uint8_t safeMonth = now.month() >= 1 ? now.month() : 1;
            uint8_t safeDay   = now.day()   >= 1 ? now.day()   : 1;
            m_rtc.adjust(DateTime(now.year(), safeMonth, safeDay, now.hour(), index, now.second()));

            // Mirror to EEPROM so time survives RTC battery loss (offset 240, 5 bytes)
            EEPROM.write(240, (uint8_t)(now.year() - 2000));
            EEPROM.write(241, safeMonth);
            EEPROM.write(242, safeDay);
            EEPROM.write(243, (uint8_t)now.hour());
            EEPROM.write(244, (uint8_t)index);
            EEPROM.commit();

            Serial.print(now.year(), DEC);
            Serial.print('/');
            Serial.print(now.month(), DEC);
            Serial.print('/');
            Serial.print(now.day(), DEC);
            Serial.print(" ");
            Serial.print(now.hour(), DEC);
            Serial.print(':');
            Serial.print(now.minute(), DEC);
            Serial.print(':');
            Serial.println(now.second(), DEC);
            Serial.println("Minute adjusted in RTC");
        }

        return;
    }

    void selectNextMinute()
    {
        m_selectedMinute = (m_selectedMinute + 1) % m_minuteArray.size();
        selectMinuteByIndex(m_selectedMinute);


    }

    void selectPreviousMinute()
    {
        if(m_selectedMinute == 0)
            m_selectedMinute = m_minuteArray.size() - 1;
        else
            m_selectedMinute = m_selectedMinute - 1;
        selectMinuteByIndex(m_selectedMinute);

    }

    void resetIdleCounter()
    {
        m_startMillis = millis();
    }

    int getSelectedMinute() {
        int minute = EEPROM.read(230);
        return m_minuteArray[minute];
    }
};
