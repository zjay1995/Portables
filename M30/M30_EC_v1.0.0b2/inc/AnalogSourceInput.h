/*
 *     ____  ________     ___                __
 *    / __ \/  _/ __ \   /   |  ____  ____ _/ /_  ______  ___  __________
 *   / /_/ // // / / /  / /| | / __ \/ __ `/ / / / /_  / / _ \/ ___/ ___/
 *  / ____// // /_/ /  / ___ |/ / / / /_/ / / /_/ / / /_/  __/ /  (__  )
 * /_/   /___/_____/  /_/  |_/_/ /_/\__,_/_/\__, / /___/\___/_/  /____/
 *                                         /____/
 *
 * M30-EC Gas Detector Firmware
 *
 * File        : inc/AnalogSourceInput.h
 * Description : ADS1115 detector reading, INA219 battery, SHT4x temp/RH, fault check.
 * Author      : Jason Wang
 * Version     : see BuildConfig.h, section 4
 *
 * Copyright (c) 2026 PID Analyzers. All rights reserved.
 */
#pragma once

#include <Arduino.h>          // millis(); don't rely on the Adafruit headers for it
#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SHT4x.h>

#include "SensorConfig.h"

// ADS1115 LSB size (millivolts per bit) for the configured PGA gain. The
// driver's readADC_SingleEnded() returns raw counts, so they must be scaled by
// the gain-specific LSB to recover millivolts. This used to be hardcoded to
// 0.125 (the GAIN_ONE LSB), which over-reported the detector voltage by 2x on
// GAIN_TWO profiles, 4x on GAIN_FOUR, and 8x on GAIN_EIGHT. (Calibrated ppm was
// unaffected because the factor cancels in the slope/intercept ratio, but the
// "Det: mV" readout and the absolute span-rejection guard were wrong.)
inline float adsMvPerBit(adsGain_t gain)
{
    switch (gain)
    {
        case GAIN_TWOTHIRDS: return 0.1875F;    // +/-6.144 V
        case GAIN_ONE:       return 0.125F;     // +/-4.096 V
        case GAIN_TWO:       return 0.0625F;    // +/-2.048 V
        case GAIN_FOUR:      return 0.03125F;   // +/-1.024 V
        case GAIN_EIGHT:     return 0.015625F;  // +/-0.512 V
        case GAIN_SIXTEEN:   return 0.0078125F; // +/-0.256 V
        default:             return 0.125F;
    }
}

// Compile-time twin of adsMvPerBit() (C++11 constexpr: single expression).
constexpr float adsMvPerBitConst(adsGain_t gain)
{
    return gain == GAIN_TWOTHIRDS ? 0.1875F : gain == GAIN_ONE   ? 0.125F   :
           gain == GAIN_TWO       ? 0.0625F : gain == GAIN_FOUR  ? 0.03125F :
           gain == GAIN_EIGHT     ? 0.015625F : gain == GAIN_SIXTEEN ? 0.0078125F : 0.125F;
}
// Once a profile's nominal zero and sensitivity are filled in, check that the
// detector output at over-range stays inside the ADC range at its gain.
static_assert(!(SENSOR_NOMINAL_ZERO_MV > 0 && SENSOR_NOMINAL_MV_PER_UNIT > 0) ||
              (SENSOR_NOMINAL_ZERO_MV + SENSOR_SIGNAL_POLARITY * SENSOR_OVERRANGE * SENSOR_NOMINAL_MV_PER_UNIT
                   < 0.95f * 32767.0f * adsMvPerBitConst(SENSOR_ADS_GAIN) &&
               SENSOR_NOMINAL_ZERO_MV + SENSOR_SIGNAL_POLARITY * SENSOR_OVERRANGE * SENSOR_NOMINAL_MV_PER_UNIT > 0.0f),
              "Sensor profile: output at over-range does not fit the ADC range at SENSOR_ADS_GAIN - pick a lower gain");

// Detector fault: input railed at the top of the ADC range (or a failed I2C
// read returning all 1s), or exactly 0 mV on profiles where the biased front
// end never legitimately reaches 0 V (SENSOR_FAULT_ON_0MV).
inline bool adsMvIsFault(uint16_t mv)
{
    const float fullScaleMv = 32767.0f * adsMvPerBit(SENSOR_ADS_GAIN);
    return (SENSOR_FAULT_ON_0MV && mv == 0) || mv >= 0.99f * fullScaleMv;
}

#if BUILD_MODE == BUILD_DEV
#include "SensorSimulator.h"   // simulated detector for bench testing
#endif

class AnalogSourceInput
{
protected:
    int m_refreshRate = 2; // refreshes per second for ADS1115
    unsigned long m_lastReadValueTick = -5000000;
    int m_refreshRate_b = 1; // refreshes per second for INA219
    unsigned long m_lastReadValueTick_b = -5000000;
    int m_refreshRate_rht = 1; // refreshes per second for SHT4x (i.e., every 2 seconds)
    unsigned long m_lastReadValueTick_rht = -5000000;
    uint16_t m_lastReadValue;
    uint16_t m_lastReadValue_battery;
    float m_lastTemperature;
    float m_lastHumidity;

public:
    virtual uint16_t getMiliVolts() = 0;
    virtual uint16_t getMiliVolts_battery() = 0;
    virtual float getTemperature() = 0;
    virtual float getHumidity() = 0;
};

class ADS1115AnalogSourceInput : public AnalogSourceInput
{
    Adafruit_ADS1115* m_ads1115;
    Adafruit_INA219* m_ina219;
    Adafruit_SHT4x* m_sht4x;

public:
    ADS1115AnalogSourceInput(Adafruit_ADS1115* ads1115, Adafruit_INA219* ina219, Adafruit_SHT4x* sht4x)
            : m_ads1115(ads1115), m_ina219(ina219), m_sht4x(sht4x)
    {
    }
    ~ADS1115AnalogSourceInput() = default;

    uint16_t getMiliVolts()
    {
        unsigned long now = millis();

        if (now - m_lastReadValueTick > 1000 / m_refreshRate)
        {
#if BUILD_MODE == BUILD_DEV
            // DEV build: the detector voltage comes from the simulated cell.
            m_lastReadValueTick = now;
            m_lastReadValue = simDetectorMv(now);
            return m_lastReadValue;
#endif
            const int numReadings = 4;
            long total = 0;
            m_lastReadValueTick = now;
            const float multiplier = adsMvPerBit(SENSOR_ADS_GAIN); // gain-matched LSB

            for (int i = 0; i < numReadings; i++) {
                total += m_ads1115->readADC_SingleEnded(0) * multiplier;
            }
            m_lastReadValue = total / numReadings;
        }
        return m_lastReadValue;
    }

    uint16_t getMiliVolts_battery()
    {
        unsigned long now = millis();

        if (now - m_lastReadValueTick_b > 1000 / m_refreshRate_b)
        {
            m_lastReadValueTick_b = now;
            float busVoltage = m_ina219->getBusVoltage_V();
            m_lastReadValue_battery = static_cast<uint16_t>(busVoltage * 1000);
        }
        return m_lastReadValue_battery;
    }

    float getTemperature()
    {
        unsigned long now = millis();

        if (now - m_lastReadValueTick_rht > 1000 / m_refreshRate_rht)
        {
            m_lastReadValueTick_rht = now;
            sensors_event_t humidity, temp;
            m_sht4x->getEvent(&humidity, &temp);
            m_lastTemperature = temp.temperature;
            m_lastHumidity = humidity.relative_humidity;
        }
        return m_lastTemperature;
    }

    float getHumidity()
    {
        getTemperature();  // Temperature and humidity are read together
        return m_lastHumidity;
    }
};
