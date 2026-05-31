#pragma once

#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SHT4x.h>

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
            m_lastReadValueTick = now;
            // DEMO: sine-wave reading, 0–~300 ppm range.
            // Calibration pair in 103_Demo.ino: intercept=1.040 V, slope=0.84.
            // ppm = ((mV/1000 - 1.040) / 0.84) * 1000
            // mV range 1040–1292 maps to 0–300 ppm; period = 60 s.
            float t = now / 1000.0f;
            m_lastReadValue = (uint16_t)(1190.0f + 150.0f * sinf(t * 2.0f * (float)M_PI / 60.0f));
        }
        return m_lastReadValue;
    }

    // DEMO: fixed 78% battery (3.9 V on the 3.4–4.05 V display scale).
    // The HUZZAH32 does not have the M103 INA219 / IO-34 battery circuit.
    uint16_t getMiliVolts_battery()
    {
        return 3900;
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
