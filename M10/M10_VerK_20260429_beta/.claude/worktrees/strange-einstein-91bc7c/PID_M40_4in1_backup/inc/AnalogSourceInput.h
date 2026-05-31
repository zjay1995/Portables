#pragma once
#include <HardwareSerial.h>
#define DATA_LENGTH 11
#define START_BYTE 0xFF

// External declaration of the HardwareSerial object for UART readings
extern HardwareSerial sensorSerial;

// Base class for analog source input
class AnalogSourceInput {
protected:
    unsigned long m_lastReadValueTick = -5000000; // Last read value timestamp
    uint16_t m_lastO2Value; // Last read O2 value
    uint16_t m_lastCH4Value; // Last read CH4 value
    uint16_t m_lastCOValue; // Last read CO value
    uint16_t m_lastH2SValue; // Last read H2S value

public:
    // Pure virtual function for getting millivolt values
    virtual void readAllValues() = 0;

    virtual uint16_t getO2Value() const { return m_lastO2Value; }
    virtual uint16_t getCH4Value() const { return m_lastCH4Value; }
    virtual uint16_t getCOValue() const { return m_lastCOValue; }
    virtual uint16_t getH2SValue() const { return m_lastH2SValue; }

    virtual ~AnalogSourceInput() = default;
};

// Derived class for UART analog source input
// Derived class for UART analog source input
class UARTAnalogSourceInput : public AnalogSourceInput {
public:
    UARTAnalogSourceInput() = default;
    ~UARTAnalogSourceInput() = default;

    // Override function to read all values
    void readAllValues() override {
        unsigned long now = millis();
        if (now - m_lastReadValueTick > 2000) {
            m_lastReadValueTick = now;

            uint8_t rawData[11];
            int length = 0;

            // Attempt to read data until we get a valid start byte
            while (true) {
                if (sensorSerial.available() >= 1) {
                    uint8_t byte = sensorSerial.read();

                    // Start byte found, read the rest of the data
                    if (byte == START_BYTE) {
                        rawData[0] = byte;
                        length = sensorSerial.readBytes(rawData + 1, DATA_LENGTH - 1);

                        // Ensure the data is complete
                        if (length == DATA_LENGTH - 1) {
                            break;
                        }
                    }
                }
                delay(10); // Short delay to avoid busy-waiting
            }


            if (length == DATA_LENGTH - 1) {
                uint8_t checksum = 0;
                for (int i = 1; i < 10; i++) {
                    checksum += rawData[i];
                }
                checksum = (~checksum) + 1;

                if (checksum == rawData[10]) {
                    m_lastH2SValue = (rawData[4] << 8) | rawData[5];       // O2 value
                    m_lastO2Value = (rawData[6] << 8) | rawData[7];      // CH4 value
                    m_lastCOValue = (rawData[2] << 8) | rawData[3];       // CO value
                    m_lastCH4Value = (rawData[8] << 8) | rawData[9];      // H2S value

                    // Print values for debugging
                    Serial.print("O2: " + String(m_lastO2Value / 10.0, 1) + " %VOL  ");
                    Serial.print("CH4: " + String(m_lastCH4Value / 1.0, 1) + " %LEL  ");
                    Serial.print("CO: " + String(m_lastCOValue / 1.0, 1) + " ppm  ");
                    Serial.println("H2S: " + String(m_lastH2SValue / 1.0, 1) + " ppm  ");
                } else {
                }
            } else {
            }
        }
    }
};
