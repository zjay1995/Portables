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
        // Optional: Call methods to read all sensor values at once if needed.
    }

    uint16_t getRawMiliVolts() const {
        return m_analogSourceInput->getMiliVolts();
    }

    uint16_t getRawMiliVolts_battery() const {
        return m_analogSourceInput->getMiliVolts_battery();
    }

    double getDoubleValue() {
        uint16_t miliVolts = m_analogSourceInput->getMiliVolts();
        return m_gasManager->calculateSLM(miliVolts / 1000.0);
    }

    double getCalibratedValue(int gasIndex) const {
        double rawValue = 0.0;
        switch (gasIndex) {
            case 0: rawValue = 1; break;
            case 1: rawValue = 2; break;
            case 2: rawValue = 3; break;
            case 3: rawValue = 4; break;
            default: return 0.0;
        }
        return rawValue;
    }

    double getSelectedGasValue() const {
        // Example: Replace `1` with dynamic gas selection logic
        return getCalibratedValue(1);
    }

    float getTemperature() const {
        return m_analogSourceInput->getTemperature();
    }

    float getHumidity() const {
        return m_analogSourceInput->getHumidity();
    }

    // New Methods for Particle Sensor Data
    float getPM1_0() const {
        return m_analogSourceInput->getPM1_0();
    }

    float getPM2_5() const {
        return m_analogSourceInput->getPM2_5();
    }

    float getPM10() const {
        return m_analogSourceInput->getPM10();
    }
};
