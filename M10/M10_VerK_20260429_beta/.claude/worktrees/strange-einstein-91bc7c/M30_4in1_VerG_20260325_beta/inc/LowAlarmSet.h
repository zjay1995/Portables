#pragma once
#include <array>
#include <EEPROM.h>
#include <Arduino.h>

class Lowalarm {
    std::array<int, 62> m_lowalarmArray{{0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000}};
    const int EEPROM_BASE = 120;

public:
    Lowalarm() = default;

    int getLowAlarmValueByIndex(int index) {
        if (index < 0 || index >= m_lowalarmArray.size()) return 0;
        return m_lowalarmArray[index];
    }

    int getSelectedLowalarm(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= 4) return 0;
        int index = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        if (index < 0 || index >= m_lowalarmArray.size()) index = 0;
        return m_lowalarmArray[index];
    }

    void selectLowalarmByIndex(int gasIndex, int index) {
        if (gasIndex < 0 || gasIndex >= 4) return;
        EEPROM.writeInt(EEPROM_BASE + (gasIndex * 4), index);
        EEPROM.commit();
    }
};