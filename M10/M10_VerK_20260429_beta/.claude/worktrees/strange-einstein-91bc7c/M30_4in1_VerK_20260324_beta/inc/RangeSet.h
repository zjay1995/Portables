#pragma once
#include <array>
#include <EEPROM.h>
#include <Arduino.h>

class Range {
    // Standard ppm scale for CO/H2S (15 items)
    std::array<int, 15> m_rangeArray{{50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000, 30000, 50000, 100000, 200000}};

    // NEW: Expanded 1-100% scale for O2/CH4 (15 items to match)
    // This removes the "stuck" feeling by providing a value for every menu index
    std::array<int, 15> m_percentArray{{1, 2, 3, 4, 5, 7, 10, 15, 20, 25, 30, 40, 50, 75, 100}};

    const int EEPROM_BASE = 72;

public:
    Range() = default;

    int getRangeValueByIndex(int gasIndex, int index) {
        if (index < 0 || index >= 15) index = 0; // Safety bounds

        if (gasIndex == 2 || gasIndex == 3) {
            // Percentage gases use the new 15-step scale
            return m_percentArray[index];
        } else {
            // ppm gases use the standard scale
            return m_rangeArray[index];
        }
    }

    int getSelectedRange(int gasIndex) {
        if (gasIndex < 0 || gasIndex >= 4) return 0;
        int index = EEPROM.readInt(EEPROM_BASE + (gasIndex * 4));
        return getRangeValueByIndex(gasIndex, index);
    }

    void selectRangeByIndex(int gasIndex, int index) {
        if (gasIndex < 0 || gasIndex >= 4) return;
        if (index < 0 || index >= 15) return;
        EEPROM.writeInt(EEPROM_BASE + (gasIndex * 4), index);
        EEPROM.commit();
    }
};