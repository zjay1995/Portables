#pragma once
#include <array>
#include "U8g2lib.h"
#define SSD1306_DISPLAYOFF          0xAE
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <Wire.h>

class Alarm
{
    std::array<int, 62> m_alarmArray{{0, 500, 525, 550, 575, 600, 625, 650, 675, 700, 725, 750, 775, 800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100, 1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500, 1525, 1550, 1575, 1600, 1625, 1650, 1675, 1700, 1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900, 1925, 1950, 1975, 2000}};
    std::array<int, 4> m_selectedAlarms{{0, 0, 0, 0}};  // Assuming 4 gas types

    unsigned long m_startMillis = 0;

    ConfigurationManager* m_configurationManager;
    U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* m_u8g2;
    SSD1306* m_display;

public:
    Alarm() = default;
    ~Alarm() = default;

    void init(ConfigurationManager* configurationManager, U8G2_SSD1327_MIDAS_128X128_F_4W_SW_SPI* u8g2) {
        m_configurationManager = configurationManager;
        m_u8g2 = u8g2;
    }

    void selectAlarmByValueNoEEPROMSave(int gasIndex, int alarm) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return;

        for (int i = 0; i < m_alarmArray.size(); i++) {
            if (m_alarmArray[i] == alarm) {
                m_selectedAlarms[gasIndex] = i;
                break;
            }
        }
    }

    void selectAlarmByIndex(int gasIndex, int index) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return;
        if (index >= 0 && index < m_alarmArray.size()) {
            m_selectedAlarms[gasIndex] = index;
            EEPROM.writeInt(96 + gasIndex * 4, index);
            EEPROM.commit();
            Serial.print("Alarm saved for gas index ");
            Serial.print(gasIndex);
            Serial.print(": ");
            Serial.println(index);
        }
    }

    void selectAlarmByValue(int gasIndex, int alarm) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return;

        for (int i = 0; i < m_alarmArray.size(); i++) {
            if (m_alarmArray[i] == alarm) {
                m_selectedAlarms[gasIndex] = i;
                m_configurationManager->saveAlarmToEEPROM(gasIndex, i);
                break;
            }
        }
    }

    void selectNextAlarm(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return;

        m_selectedAlarms[gasIndex] = (m_selectedAlarms[gasIndex] + 1) % m_alarmArray.size();
    }

    void selectPreviousAlarm(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return;

        if (m_selectedAlarms[gasIndex] == 0) {
            m_selectedAlarms[gasIndex] = m_alarmArray.size() - 1;
        } else {
            m_selectedAlarms[gasIndex] = m_selectedAlarms[gasIndex] - 1;
        }
    }

    void resetIdleCounter() {
        m_startMillis = millis();
    }

    int getSelectedAlarm(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= m_selectedAlarms.size()) return -1;
        int alarm = EEPROM.read(76 + gasIndex);
        return m_alarmArray[alarm];
    }
};
