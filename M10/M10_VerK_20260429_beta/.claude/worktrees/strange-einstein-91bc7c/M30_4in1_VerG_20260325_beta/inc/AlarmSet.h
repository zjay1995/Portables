#pragma once
#include <array>
#include <EEPROM.h>
#include <Arduino.h>

class Alarm
{
    // Common industrial alarm thresholds
    std::array<int, 62> m_alarmArray{{0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000}};

    const int EEPROM_BASE = 140; // High Alarms Offset

public:
    Alarm() = default;

    // ADD THIS METHOD: Returns the ppm value for a specific index in the array
    int getAlarmValueByIndex(int index) {
        if (index < 0 || index >= m_alarmArray.size()) return 0;
        return m_alarmArray[index];
    }

    int getSelectedAlarm(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= 4) return 0;
        int index = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        if (index < 0 || index >= m_alarmArray.size()) index = 0;
        return m_alarmArray[index];
    }

    void selectAlarmByIndex(int gasIndex, int index) {
        if (gasIndex < 0 || gasIndex >= 4) return;
        if (index < 0 || index >= m_alarmArray.size()) index = 0;

        EEPROM.writeInt(EEPROM_BASE + (gasIndex * 4), index);
        EEPROM.commit();
    }

    void selectNextAlarm(int gasIndex) {
        int currentIndex = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        int nextIndex = (currentIndex + 1) % m_alarmArray.size();
        selectAlarmByIndex(gasIndex, nextIndex);
    }

    void selectPreviousAlarm(int gasIndex) {
        int currentIndex = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        int prevIndex = (currentIndex <= 0) ? m_alarmArray.size() - 1 : currentIndex - 1;
        selectAlarmByIndex(gasIndex, prevIndex);
    }
};