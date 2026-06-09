#pragma once
#include <array>
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>

class SleepTimer
{
	// Idle minutes before auto power-off; -1 = continuous (never off).
	// Values must match the "Auto Power-Off" menu labels in the .ino.
	std::array <int,5> m_intervalArray{{5,60,120,360,-1}};
	int m_selectedIndex = 0;

	unsigned long m_startMillis = 0;

	ConfigurationManager* m_configurationManager;
    //U8G2_SSD1327_MIDAS_128X128_F_4W_HW_SPI* m_u8g2;
    SSD1306* m_display;


public:

	SleepTimer()
	{
		m_selectedIndex = 4;
	}

	~SleepTimer()=default;

	void init(ConfigurationManager* configurationManager)
	{
		m_configurationManager = configurationManager;
	}

    void selectIntervalByValueNoEEPROMSave(int interval)
    {
        for(int i=0; i < m_intervalArray.size(); i++)
            if(m_intervalArray[i] == interval)
            {
                m_selectedIndex = i;
                break;
            }

        return;
    }

	void selectIntervalByIndex(int index)
	{
		if(index >= 0 && index < m_intervalArray.size())
		{
			m_selectedIndex = index;
			EEPROM.writeInt(0, index);
			EEPROM.commit();
			Serial.print("Sleep timer saved: ");
			Serial.println(index);
		}

		return;
	}

	void selectIntervalByValue(int interval)
	{
		for(int i=0; i < m_intervalArray.size(); i++)
			if(m_intervalArray[i] == interval)
			{
				selectIntervalByIndex(i);
				break;
			}

		return;
	}

	void selectNextInterval()
	{
		m_selectedIndex = (m_selectedIndex + 1) % m_intervalArray.size();
	}

	void selectPreviousInterval()
	{
		if(m_selectedIndex == 0)
			m_selectedIndex = m_intervalArray.size() - 1;
		else
			m_selectedIndex = m_selectedIndex - 1;
	}

	void resetIdleCounter()
	{
		m_startMillis = millis();
	}

	int getSelectedInterval() { return m_intervalArray[m_selectedIndex]; }
	int getSelectedIndex()    { return m_selectedIndex; }

	// Auto power-off after the selected idle time (was hardcoded to 30 min
	// and ignored the setting). Sequence mirrors the ON-button handler.
	void handleTick()
	{
		int minutes = m_intervalArray[m_selectedIndex];
		if (minutes < 0) return;  // continuous — never auto-off

		unsigned long passedMillis = millis() - m_startMillis;
		if (passedMillis >= (unsigned long)minutes * 60UL * 1000UL)
		{
			Serial.println("SleepTimer: idle timeout -- deep sleep");
			Serial.flush();
			gpio_hold_en(GPIO_NUM_5);
			gpio_deep_sleep_hold_en();
			digitalWrite(25, LOW);  // release power hold
			delay(1000);
			esp_deep_sleep_start();
		}
	}
};
