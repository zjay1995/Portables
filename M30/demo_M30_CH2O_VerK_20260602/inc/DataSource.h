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
        // DEMO BUILD: do NOT read the ADC. Report a synthetic CH2O reading that
        // drifts randomly within 0.00–0.30 ppm, refreshed about once a second
        // so the run screen looks like a live, slowly-changing measurement.
        static unsigned long lastUpdate = 0;
        static double demoValue = 0.0;
        unsigned long now = millis();
        if (lastUpdate == 0 || now - lastUpdate > 1000) {
            lastUpdate = now;
            demoValue = random(0, 31) / 100.0;  // 0.00–0.30 ppm in 0.01 steps
        }
        return demoValue;
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
        //int selectedGasIndex = m_gasManager->getSelectedGasIndex();  // Ensure this method is defined in GasManager
        return getCalibratedValue(1);
    }

    uint16_t getTemperature() const
    {
        return m_analogSourceInput->getTemperature();
    }

    uint16_t getHumidity() const
    {
        return m_analogSourceInput->getHumidity();
    }
};
