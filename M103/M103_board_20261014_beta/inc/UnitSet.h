#pragma once
#include <EEPROM.h>

class UnitSet {
    int m_selectedUnit;  // 0 = ppm, 1 = ppb, 2 = 1117 lamp (ppm scale)
public:
    UnitSet() : m_selectedUnit(0) {}

    void selectUnitByIndex(int index) {
        if (index >= 0 && index <= 2) {
            m_selectedUnit = index;
            EEPROM.write(100, (uint8_t)index);
            EEPROM.commit();
        }
    }

    int  getSelectedUnit() const { return m_selectedUnit; }
    bool isPpb()           const { return m_selectedUnit == 1; }
    const char* getUnitLabel() const {
        switch (m_selectedUnit) {
            case 1:  return "ppb";
            case 2:  return "1117";
            default: return "ppm";
        }
    }
};
