#pragma once
#include <HardwareSerial.h>
#include <Adafruit_INA219.h>

#define DATA_LENGTH 11
#define START_BYTE 0xFF

// These should be defined in your main .ino file
extern HardwareSerial sensorSerial;
extern Adafruit_INA219 ina219;

class AnalogSourceInput {
protected:
    unsigned long m_lastReadValueTick = -5000000;

    // Gas values
    uint16_t m_lastO2Value = 0;
    uint16_t m_lastCH4Value = 0;
    uint16_t m_lastCOValue = 0;
    uint16_t m_lastH2SValue = 0;

    // Battery values
    float m_batteryVoltage = 0.0;
    float m_batteryCurrent = 0.0;

public:
    virtual void readAllValues() = 0;

    // Getters for Gas
    virtual uint16_t getO2Value() const { return m_lastO2Value; }
    virtual uint16_t getCH4Value() const { return m_lastCH4Value; }
    virtual uint16_t getCOValue() const { return m_lastCOValue; }
    virtual uint16_t getH2SValue() const { return m_lastH2SValue; }

    // Getters for Battery
    virtual float getBatteryVoltage() const { return m_batteryVoltage; }
    virtual float getBatteryCurrent() const { return m_batteryCurrent; }

    virtual ~AnalogSourceInput() = default;
};

class UARTAnalogSourceInput : public AnalogSourceInput {
public:
    void readAllValues() override {
        unsigned long now = millis();

        // Execute updates every 2 seconds
        if (now - m_lastReadValueTick > 2000) {
            m_lastReadValueTick = now;

            // 1. Read Battery Data (I2C is non-blocking)
            m_batteryVoltage = ina219.getBusVoltage_V();
            m_batteryCurrent = ina219.getCurrent_mA();

            // 2. Non-blocking Sensor Check
            // If the sensor is disconnected, available() will be 0 and we exit early.
            if (sensorSerial.available() < DATA_LENGTH) {
                Serial.println("Waiting for sensor data...");
                return;
            }

            uint8_t rawData[DATA_LENGTH];
            bool frameFound = false;

            // 3. Search buffer for the START_BYTE without infinite while(true)
            while (sensorSerial.available() > 0) {
                // Peek at the first byte to see if it's the start of a frame
                if (sensorSerial.peek() == START_BYTE) {
                    // Check if the rest of the frame has arrived
                    if (sensorSerial.available() >= DATA_LENGTH) {
                        int length = sensorSerial.readBytes(rawData, DATA_LENGTH);
                        if (length == DATA_LENGTH) {
                            frameFound = true;
                        }
                        break; // Exit while loop after processing a frame
                    } else {
                        // Start byte is there, but full frame hasn't arrived.
                        // Exit and try again next tick to avoid blocking.
                        return;
                    }
                } else {
                    // Not a start byte, discard it to clear the buffer
                    sensorSerial.read();
                }
            }

            // 4. Validate Checksum and Assign Values
            if (frameFound) {
                uint8_t checksum = 0;
                for (int i = 1; i < 10; i++) {
                    checksum += rawData[i];
                }
                checksum = (~checksum) + 1;

                if (checksum == rawData[10]) {
                    // Mapping bytes to variables based on your sensor protocol
                    m_lastCOValue  = (rawData[2] << 8) | rawData[3];
                    m_lastH2SValue = (rawData[4] << 8) | rawData[5];
                    m_lastO2Value  = (rawData[6] << 8) | rawData[7];
                    m_lastCH4Value = (rawData[8] << 8) | rawData[9];

                    // Debug print to Serial Monitor
                    Serial.printf("Battery: %.2fV | O2: %.1f%% | CH4: %.1f%% | CO: %dppm | H2S: %dppm\n",
                                  m_batteryVoltage,
                                  m_lastO2Value / 10.0,
                                  m_lastCH4Value / 1.0,
                                  m_lastCOValue,
                                  m_lastH2SValue);
                } else {
                    Serial.println("Checksum Error!");
                }
            }
        }
    }
};