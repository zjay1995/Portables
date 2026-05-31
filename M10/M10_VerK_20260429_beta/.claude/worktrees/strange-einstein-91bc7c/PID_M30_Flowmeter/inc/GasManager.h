#pragma once

#include <vector>
#include <Arduino.h>
#include <EEPROM.h>
#include <RTClib.h>

#include "ConfigurationManager.h"
#include "Globals.h"
#include "CalvalueSet.h"
#include "AlarmSet.h"

#define EEPROM_CAL_TIMESTAMP_OFFSET 120  // Ensure this doesn't conflict

class Gas {
    String m_name;
    double m_thermalConductivity;

public:
    Gas(String name, double thermalConductivity = 1.0)
            : m_name(name), m_thermalConductivity(thermalConductivity) {}

    String getName() const { return m_name; }
    double getThermalConductivity() const { return m_thermalConductivity; }
    void setThermalConductivity(double tc) { m_thermalConductivity = tc; }
};

class GasManager : public ParamChangeListener {
    std::vector<Gas> m_gases;
    int m_selectedGas = EEPROM.readInt(4);

    double m_intercept;
    double m_slope;
    double m_zero;
    double m_calgas;

    bool calSuccess;
    ConfigurationManager* m_configurationManager;

public:
    GasManager(double intercept, double slope, double zero, double calgas)
            : m_intercept(intercept),
              m_slope(slope),
              m_zero(zero),
              m_calgas(calgas),
              calSuccess(false),
              m_configurationManager(nullptr) {}

    ~GasManager() {}

    double getIntercept() const { return m_intercept; }
    double getSlope() const { return m_slope; }
    double getZero() const { return m_zero; }
    double getCalgas() const { return m_calgas; }

    void setIntercept(double i) { m_intercept = i; }
    void setSlope(double p) { m_slope = p; }
    void setZero(double z) { m_zero = z; }
    void setCalgas(double c) { m_calgas = c; }

    bool wasCalibrationSuccessful() const { return calSuccess; }

    double calculateSLM(double voltage) {
        double baseSLM = ((voltage - m_intercept) / m_slope) * 1000;
        double factor = m_gases[m_selectedGas].getThermalConductivity();
        return max(0.0, baseSLM * factor);
    }

    void setConfigurationManager(ConfigurationManager* c) { m_configurationManager = c; }

    void addGas(Gas g) { m_gases.push_back(g); }

    void printinfo(double currentVoltage) {
        Serial.println("M # Selected Gas Information:");
        Serial.print("Gas Name: ");
        Serial.println(m_gases[m_selectedGas].getName());
        Serial.println(getCalgas());

        double gasLevel = calculateSLM(currentVoltage / 1000.0); // mV to V
        Serial.print("Calculated Gas Level (SLM) at ");
        Serial.print(currentVoltage);
        Serial.print(" mV: ");
        Serial.println(gasLevel);
        Serial.println("-------------------------------");
    }

    void selectGasByIndex(int index) {
        if (index >= 0 && index < m_gases.size()) {
            m_selectedGas = index;
            EEPROM.writeInt(4, index);
            EEPROM.commit();
            Serial.println("Gas index saved: " + String(index));
        }
    }

    void selectPreviousGas() {
        if (m_selectedGas == 0)
            m_selectedGas = m_gases.size() - 1;
        else
            m_selectedGas -= 1;
    }

    Gas& getSelectedGas() { return m_gases[m_selectedGas]; }
    std::vector<Gas>& getAllGases() { return m_gases; }

    void calibrate(double zero) {
        m_zero = zero / 1000.0;
        m_intercept = m_zero;
        EEPROM.put(8, m_intercept);
        EEPROM.commit();
        Serial.println("Intercept saved to EEPROM: " + String(m_intercept));
    }

    void calibrate2(double cal) {
        m_calgas = cal / 1000.0;
        int calvalue = Calvalue().getSelectedCalvalue();
        double calgasv = calvalue / 1000.0;

        m_slope = (m_calgas - m_zero) / calgasv;
        EEPROM.put(16, m_slope);
        EEPROM.commit();
        calSuccess = true;

        Serial.print("Slope saved to EEPROM: ");
        Serial.println(m_slope);

        // Save timestamp
        RTC_PCF8563 rtc;
        if (!rtc.begin()) {
            Serial.println("RTC not initialized. Skipping date save.");
            return;
        }

        DateTime now = rtc.now();
        String calTimestamp = String(now.year()) + "-" +
                              (now.month() < 10 ? "0" : "") + String(now.month()) + "-" +
                              (now.day() < 10 ? "0" : "") + String(now.day()) + " " +
                              (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" +
                              (now.minute() < 10 ? "0" : "") + String(now.minute());

        EEPROM.writeString(EEPROM_CAL_TIMESTAMP_OFFSET, calTimestamp);
        EEPROM.commit();

        Serial.println("Calibration succeeded. Saved date: " + calTimestamp);
    }

    void calibrate3() {
        // Placeholder if needed later
    }

    void onParamChange(String param, String value) {
        Serial.println("onGasParamChange: " + param + "=" + value);

        if (param.equals(c_INTERCEPT_PARAM_NAME)) {
            m_intercept = value.toDouble();
            Serial.println("Set INTERCEPT: " + value);
        } else if (param.equals(c_SLOPE_PARAM_NAME)) {
            m_slope = value.toDouble();
            Serial.println("Set SLOPE: " + value);
        } else if (param.equals(c_GASINDEX_PARAM_NAME)) {
            m_selectedGas = value.toInt();
            Serial.println("Set GAS INDEX: " + value);
        } else {
            for (auto& gas : m_gases) {
                if (gas.getName().equals(param)) {
                    double tc = value.toDouble();
                    gas.setThermalConductivity(tc);
                    Serial.println("Set TC for gas " + gas.getName() + ": " + String(tc));
                    break;
                }
            }
        }
    }
};
