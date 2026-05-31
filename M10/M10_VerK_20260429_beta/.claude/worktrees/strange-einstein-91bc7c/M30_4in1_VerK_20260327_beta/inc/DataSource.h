#pragma once

#include "AnalogSourceInput.h"
#include "GasManager.h"

class DataSource {
    GasManager* m_gasManager;
    AnalogSourceInput* m_analogSourceInput;

public:
    DataSource(GasManager* gasManager, AnalogSourceInput* analogSourceInput)
            : m_gasManager(gasManager), m_analogSourceInput(analogSourceInput) {}

    ~DataSource() = default;

    void readAllValues() const {
        m_analogSourceInput->readAllValues();
    }

    // Calculates calibrated gas value based on the current gas configuration
    double getCalibratedValue(int gasIndex) const {
        double rawValue = 0.0;
        switch (gasIndex) {
            case 0: rawValue = m_analogSourceInput->getCOValue(); break;
            case 1: rawValue = m_analogSourceInput->getH2SValue(); break;
            case 2: rawValue = m_analogSourceInput->getO2Value();rawValue /= 10.0; break; // Raw value is used directly
            case 3: rawValue = m_analogSourceInput->getCH4Value();  break;
            default: return 0.0;
        }
        return rawValue; // Now returns the scaled value directly
    }

    // Retrieves the gas value based on the currently selected gas in GasManager
    double getSelectedGasValue() const {
        int selectedGasIndex = m_gasManager->getSelectedGasIndex();  // Ensure this method is defined in GasManager
        return getCalibratedValue(selectedGasIndex);
    }

// Inside class DataSource
float getBatteryVoltage() const {
    return m_analogSourceInput->getBatteryVoltage();
}

float getBatteryCurrent() const {
    return m_analogSourceInput->getBatteryCurrent();
}
};
