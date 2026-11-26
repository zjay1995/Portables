/*
 *  ____ ___ ____       _                _
 * |  _ \_ _|  _ \     / \   _ __   __ _| |_   _ _______ _ __
 * | |_) | || | | |   / _ \ | '_ \ / _` | | | | |_  / _ \ '__|
 * |  __/| || |_| |  / ___ \| | | | (_| | | |_| |/ /  __/ |
 * |_|  |___|____/  /_/   \_\_| |_|\__,_|_|\__, /___\___|_|
 *                                         |___/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/DataSource.h
 * Description : Sensor reading in ppm, sensor-fault check, DEMO reading.
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzer. All rights reserved.
 */
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

    // Sensor/ADC fault: the detector reads exactly 0 mV (open circuit, dead
    // cell, ADC not answering) or sits at the top of the ADC's input range
    // (railed/saturated input, or a failed I2C read returning all 1s).
    bool isSensorFault() const
    {
        return adsMvIsFault(m_analogSourceInput->getMiliVolts());
    }

    uint16_t getRawMiliVolts_battery() const
    {
        return m_analogSourceInput->getMiliVolts_battery();
    }


    double getDoubleValue()
    {
#if BUILD_MODE == BUILD_DEMO
        // DEMO build: no sensor. Random value in the profile's demo range
        // (SENSOR_DEMO_MIN..MAX, CH2O 50 ppm: 0.00-0.30), refreshed ~1 s so the
        // Run screen looks like a live, slowly changing reading.
        static unsigned long lastUpdate = 0;
        static double demoValue = 0.0;
        unsigned long now = millis();
        if (lastUpdate == 0 || now - lastUpdate > 1000) {
            lastUpdate = now;
            const double lo = SENSOR_DEMO_MIN;
            const double step = pow(10.0, -SENSOR_RES_DIGITS);   // display resolution
            double hi = SENSOR_DEMO_MAX;
            if (hi < lo + 3 * step) hi = lo + 3 * step;          // always visibly change
            long steps = (long)floor((hi - lo) / step + 1e-6);   // never exceed hi
            demoValue = lo + random(0, steps + 1) * step;
        }
        return demoValue;
#endif
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
