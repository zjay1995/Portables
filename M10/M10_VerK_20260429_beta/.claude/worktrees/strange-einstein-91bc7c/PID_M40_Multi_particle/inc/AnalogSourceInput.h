#pragma once

#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SHT4x.h>
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h" // Include SN-GCJA5 library

class AnalogSourceInput
{
protected:
    int m_refreshRate = 2; // refreshes per second for ADS1115
    unsigned long m_lastReadValueTick = -5000000;
    int m_refreshRate_b = 1; // refreshes per second for INA219
    unsigned long m_lastReadValueTick_b = -5000000;
    int m_refreshRate_rht = 1; // refreshes per second for SHT4x
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
    // New virtual methods for particle sensor
    virtual float getPM1_0() = 0;
    virtual float getPM2_5() = 0;
    virtual float getPM10() = 0;
};

class ADS1115AnalogSourceInput : public AnalogSourceInput
{
    Adafruit_ADS1115* m_ads1115;
    Adafruit_INA219* m_ina219;
    Adafruit_SHT4x* m_sht4x;
    SFE_PARTICLE_SENSOR* m_particleSensor; // Add SN-GCJA5 sensor

    unsigned long m_lastReadValueTick_particle = -5000000;
    int m_refreshRate_particle = 1; // refreshes per second for SN-GCJA5

    float m_lastPM1_0, m_lastPM2_5, m_lastPM10;

public:
    ADS1115AnalogSourceInput(Adafruit_ADS1115* ads1115, Adafruit_INA219* ina219, Adafruit_SHT4x* sht4x, SFE_PARTICLE_SENSOR* particleSensor)
            : m_ads1115(ads1115), m_ina219(ina219), m_sht4x(sht4x), m_particleSensor(particleSensor)
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
            const float multiplier = 0.125F;

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

    float getPM1_0()
    {
        unsigned long now = millis();
        if (now - m_lastReadValueTick_particle > 1000 / m_refreshRate_particle)
        {
            m_lastReadValueTick_particle = now;
            m_lastPM1_0 = m_particleSensor->getPM1_0();
        }
        return m_lastPM1_0;
    }

    float getPM2_5()
    {
        unsigned long now = millis();
        if (now - m_lastReadValueTick_particle > 1000 / m_refreshRate_particle)
        {
            m_lastReadValueTick_particle = now;
            m_lastPM2_5 = m_particleSensor->getPM2_5();
        }
        return m_lastPM2_5;
    }

    float getPM10()
    {
        unsigned long now = millis();
        if (now - m_lastReadValueTick_particle > 1000 / m_refreshRate_particle)
        {
            m_lastReadValueTick_particle = now;
            m_lastPM10 = m_particleSensor->getPM10();
        }
        return m_lastPM10;
    }
};
