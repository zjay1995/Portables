#pragma once
#include <array>
#include "ConfigurationManager.h"
#include <SSD1306.h>
#include <EEPROM.h>

class Zerovalue {
    std::array<double, 3> m_zeroArray{{ 0.0, 1.0, 20.9}};
    int m_selectedIndex = 0;
    const int EEPROM_OFFSET = 240; // Choose an unused EEPROM location

public:
    Zerovalue() { m_selectedIndex = 0; }

    void selectZerovalueByIndex(int index) {
        if (index >= 0 && index < m_zeroArray.size()) {
            m_selectedIndex = index;
            EEPROM.writeInt(EEPROM_OFFSET, index);
            EEPROM.commit();
        }
    }

    double getSelectedZerovalue() {
        int index = EEPROM.readInt(EEPROM_OFFSET);
        if (index < 0 || index >= m_zeroArray.size()) index = 0;
        return m_zeroArray[index];
    }
    
    String getValueName(int index) {
        return String(m_zeroArray[index], 1);
    }

    size_t getSize() { return m_zeroArray.size(); }
};