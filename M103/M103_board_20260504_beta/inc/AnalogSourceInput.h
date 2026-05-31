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

    // Battery reading.
    // Primary path: INA219 over I2C (returns bus voltage in V).
    // If the INA219 isn't populated/responding, fall back to the IO 34
    // resistor divider per the M103 schematic (VBATT --100K--+--100K--GND,
    // tap to IO 34). Doubling the ADC reading accounts for the 1:2 divider.
    uint16_t getMiliVolts_battery()
    {
        unsigned long now = millis();

        if (now - m_lastReadValueTick_b > 1000 / m_refreshRate_b)
        {
            m_lastReadValueTick_b = now;
            float busVoltage = m_ina219->getBusVoltage_V();
            // INA219 returns 0.0 when not present or shorted -- treat that
            // as a signal to use the on-board divider.
            if (busVoltage > 0.5f) {
                m_lastReadValue_battery = static_cast<uint16_t>(busVoltage * 1000);
            } else {
                int raw = analogRead(34);
                // 12-bit ADC, ADC_11db ~ 3.3V FS, divider 1:2.
                m_lastReadValue_battery = static_cast<uint16_t>((raw * 3300L * 2L) / 4095L);
            }
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
