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
    uint16_t m_lastCH2OValue; // Last read CH2O value
    uint16_t m_lastCH4Value; // Last read CH4 value
    uint16_t m_lastCOValue; // Last read CO value
    uint16_t m_lastH2SValue; // Last read H2S value

public:
    // Pure virtual function for getting millivolt values
    virtual void readAllValues() = 0;

    virtual uint16_t getCH2OValue() const { return m_lastCH2OValue; }


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
        if (now - m_lastReadValueTick > 1500) { // Read every 2 seconds
            m_lastReadValueTick = now;

            uint8_t rawData[9];
            int length = 0;

            // Attempt to read data until we get a valid start byte
            while (true) {
                if (sensorSerial.available() >= 1) {
                    uint8_t byte = sensorSerial.read();

                    // Start byte found, read the rest of the data
                    if (byte == 0xFF) {
                        rawData[0] = byte;
                        length = sensorSerial.readBytes(rawData + 1, 8);

                        // Ensure the data is complete
                        if (length == 8) {
                            break;
                        }
                    }
                }
                delay(10); // Short delay to avoid busy-waiting
            }

            if (length == 8) {
                uint8_t checksum = 0;
                for (int i = 1; i < 8; i++) {
                    checksum += rawData[i];
                }
                checksum = (~checksum) + 1;

                if (checksum == rawData[8]) { // Validate checksum
                    uint16_t concentration = (rawData[4] << 8) | rawData[5];
                    m_lastCH2OValue = concentration;

                    // Print CH2O value for debugging
                    Serial.print("CH2O: ");
                    Serial.print(m_lastCH2OValue);
                    Serial.println(" ug/m3");
                } else {
                    Serial.println("Checksum mismatch.");
                }
            } else {
                Serial.println("Incomplete data received.");
            }
        }
    }
};
