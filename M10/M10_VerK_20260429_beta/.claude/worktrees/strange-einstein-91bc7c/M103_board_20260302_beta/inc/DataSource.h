#pragma once

#include "AnalogSourceInput.h"
#include "GasManager.h"

class DataSource {
    GasManager* m_gasManager;
    AnalogSourceInput* m_analogSourceInput;

public:
    DataSource(GasManager* gasManager, AnalogSourceInput* analogSourceInput)
            : m_gasManager(gasManager), m_analogSourceInput(analogSourceInput)
    {
    }
    ~DataSource() = default;

    void readAllValues() const {
//        m_analogSourceInput->readAllValues();
    }

    uint16_t getRawMiliVolts() const
    {
        return m_analogSourceInput->getMiliVolts();
    }

    uint16_t getRawMiliVolts_battery() const
    {
        return m_analogSourceInput->getMiliVolts_battery();
    }


    double getDoubleValue()
    {
        uint16_t miliVolts = m_analogSourceInput->getMiliVolts();
        return m_gasManager->calculateSLM(miliVolts / 1000.0);
    }

    double getCalibratedValue(int gasIndex) const {
        double rawValue = 0.0;
        switch (gasIndex) {
            case 0: rawValue = 1;
            case 1: rawValue = 2;
            case 2: rawValue = 3;
            case 3: rawValue = 4;
            default: return 0.0;
        }
        return rawValue; // Now returns the scaled value directly
    }

    double getSelectedGasValue() const {
        //int selectedGasIndex = m_gasManager->getSelectedGasIndex();  // Ensure this method is defined in GasManager
        return getCalibratedValue(1);
    }

    // CHANGED: Return float instead of uint16_t for precision
    float getTemperature() const
    {
        return m_analogSourceInput->getTemperature();
    }

    // CHANGED: Return float instead of uint16_t for precision
    float getHumidity() const
    {
        return m_analogSourceInput->getHumidity();
    }
};