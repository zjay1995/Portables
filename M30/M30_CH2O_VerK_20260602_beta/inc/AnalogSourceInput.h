#pragma once

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
