#pragma once
#include <array>
#include <EEPROM.h>
#include <Arduino.h>

class Range {
    std::array<int, 15> m_rangeArray{{50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000, 30000, 50000, 100000, 200000}};
    const int EEPROM_BASE = 72;

public:
    Range() = default;

    int getRangeValueByIndex(int index) {
        if (index < 0 || index >= m_rangeArray.size()) return 0;
        return m_rangeArray[index];
    }

    int getSelectedRange(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= 4) return 0;
        int index = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        if (index < 0 || index >= m_rangeArray.size()) index = 0;
        return m_rangeArray[index];
    }

    void selectRangeByIndex(int gasIndex, int index) {
        if (gasIndex < 0 || gasIndex >= 4) return;
        EEPROM.writeInt(EEPROM_BASE + (gasIndex * 4), index);
        EEPROM.commit();
    }
};