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
    double getCalibratedValue() const {
        double rawValue = 0.0;
        double CH2OValue = m_analogSourceInput->getCH2OValue();
        rawValue = m_gasManager->calculateSLM(CH2OValue);
        return rawValue; // Now returns the scaled value directly
    }

    // Retrieves the gas value based on the currently selected gas in GasManager
    double getSelectedGasValue() const {
        return m_analogSourceInput->getCH2OValue();
    }
};
