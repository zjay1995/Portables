#pragma once

#include <vector>
#include <Arduino.h>
#include <EEPROM.h>
#include <RTClib.h>

#define EEPROM_CAL_TIMESTAMP_OFFSET 88  // 32 bytes, addr 88-119 (gap after Range 72-87)

#include "ConfigurationManager.h"
#include "Globals.h"
#include "CalvalueSet.h"
#include "AlarmSet.h"

class Gas
{
    String m_name;
    double m_thermalConductivity;
public:
    Gas(String name, double thermalConductivity = 1.0) : m_name(name),
                                                         m_thermalConductivity(thermalConductivity)
    {}

    String getName() { return m_name; }
    double getThermalConductivity() const                   { return m_thermalConductivity; }
    void setThermalConductivity(double thermalConductivity) { m_thermalConductivity = thermalConductivity; }
};

class GasManager : public ParamChangeListener
{
    std::vector<Gas> m_gases;

    int  m_selectedGas   = 0;
    bool m_lastCalFailed = false;

    double m_intercept;
    double m_slope;
    double m_intercept2;
    double m_slope2;
    double m_intercept3;
    double m_slope3;
    double m_intercept4;
    double m_slope4;
    double m_zero;
    double m_calgas;

    ConfigurationManager* m_configurationManager;

public:
    // Constructor updated to support 1:1 defaults for new boards (Slopes = 1.0, Intercepts = 0.0)
    GasManager(double intercept, double slope,
               double zero, double calgas, double intercept2, double slope2,
               double intercept3, double slope3, double intercept4, double slope4) :
                                                                                      m_intercept(intercept),
                                                                                      m_slope(slope),
                                                                                      m_intercept2(intercept2),
                                                                                      m_slope2(slope2),
                                                                                      m_intercept3(intercept3),
                                                                                      m_slope3(slope3),
                                                                                      m_intercept4(intercept4),
                                                                                      m_slope4(slope4),
                                                                                      m_zero(zero),
                                                                                      m_calgas(calgas)
    {}

    ~GasManager() {}

    // Getters for calibration parameters
    double getIntercept() const { return m_intercept; }
    double getSlope() const { return m_slope; }
    double getIntercept2() const { return m_intercept2; }
    double getSlope2() const { return m_slope2; }
    double getIntercept3() const { return m_intercept3; }
    double getSlope3() const { return m_slope3; }
    double getIntercept4() const { return m_intercept4; }
    double getSlope4() const { return m_slope4; }

    void setIntercept(double i) { m_intercept = i; }
    void setSlope(double p) {m_slope = p; }
    void setIntercept2(double ii) { m_intercept2 = ii; }
    void setSlope2(double pp) {m_slope2 = pp; }
    void setIntercept3(double iii) { m_intercept3 = iii; }
    void setSlope3(double ppp) {m_slope3 = ppp; }
    void setIntercept4(double iiii) { m_intercept4 = iiii; }
    void setSlope4(double pppp) {m_slope4 = pppp; }

    // Calculates concentration: (Voltage - Intercept) / Slope
    double calculateSLM(double voltage, int gas_index) {
        double currentIntercept = getInterceptByIndex(gas_index);
        double currentSlope = getSlopeByIndex(gas_index);

        double val = (voltage - currentIntercept) / currentSlope;
        return val < 0 ? 0 : val;
    }

    bool wasLastCalFailed() const { return m_lastCalFailed; }
    void resetCalFlag()           { m_lastCalFailed = false; }

    void setConfigurationManager(ConfigurationManager* c) { m_configurationManager = c; }
    void addGas(Gas g) { m_gases.push_back(g); }

    void selectGasByIndex(int index)
    {
        if(index >= 0 && index < m_gases.size())
        {
            m_selectedGas = index;
            EEPROM.writeInt(4, index);
            EEPROM.commit();
        }
    }
    // Inside class GasManager in inc/GasManager.h
    void printinfo() {
        Serial.println("GasManager: Gas Index " + String(m_selectedGas));
    }

    // UPDATED: Zero Calibration now accepts a targetValue (e.g., 20.9 for O2 in ambient air)
    void calibrate(double rawSensorReading, double targetValue, int gasIndex){
        m_lastCalFailed = false;

        // ±50% guard: current reading must be within 50–150% of target (skipped when target == 0)
        if (targetValue > 0.0) {
            double currentConc = calculateSLM(rawSensorReading, gasIndex);
            double ratio = currentConc / targetValue;
            if (ratio < 0.5 || ratio > 1.5) {
                m_lastCalFailed = true;
                Serial.printf("Gas %d zero cal FAILED: reading=%.2f target=%.2f ratio=%.2f\n",
                              gasIndex, currentConc, targetValue, ratio);
                return;
            }
        }

        double currentSlope = getSlopeByIndex(gasIndex);

        // Formula: Intercept = RawReading - (TargetValue * Slope)
        double newIntercept = rawSensorReading - (targetValue * currentSlope);

        if(gasIndex == 0) m_intercept = newIntercept;
        else if(gasIndex == 1) m_intercept2 = newIntercept;
        else if(gasIndex == 2) m_intercept3 = newIntercept;
        else if(gasIndex == 3) m_intercept4 = newIntercept;

        EEPROM.put(8 + gasIndex * 16, newIntercept);
        EEPROM.commit();
        Serial.printf("Gas %d calibrated to target %f. New Intercept: %f\n", gasIndex, targetValue, newIntercept);
    }

    // Span Calibration: Calculates slope based on target concentration
    void calibrate2(double rawSensorReading, double targetCalValue, int gasIndex){
        if (targetCalValue == 0) return;

        // ±50% guard: current reading must be within 50–150% of target
        double currentConc = calculateSLM(rawSensorReading, gasIndex);
        double ratio = currentConc / targetCalValue;
        if (ratio < 0.5 || ratio > 1.5) {
            m_lastCalFailed = true;
            Serial.printf("Gas %d span cal FAILED: reading=%.2f target=%.2f ratio=%.2f\n",
                          gasIndex, currentConc, targetCalValue, ratio);
            return;
        }

        double currentIntercept = getInterceptByIndex(gasIndex);
        double newSlope = (rawSensorReading - currentIntercept) / targetCalValue;

        // Prevent division by zero or invalid slopes if the reading is below the intercept
        if (rawSensorReading <= currentIntercept) newSlope = 1.0;

        if(gasIndex == 0) m_slope = newSlope;
        else if(gasIndex == 1) m_slope2 = newSlope;
        else if(gasIndex == 2) m_slope3 = newSlope;
        else if(gasIndex == 3) m_slope4 = newSlope;

        EEPROM.put(16 + gasIndex * 16, newSlope);
        EEPROM.commit();
        Serial.printf("Gas %d span calibrated. New Slope: %f\n", gasIndex, newSlope);

        static RTC_PCF8563 rtc;
        static bool rtcReady = false;
        if (!rtcReady) {
            rtcReady = rtc.begin();
            if (!rtcReady) return;
        }
        DateTime now = rtc.now();
        String ts = String(now.year()) + "-" +
            (now.month()  < 10 ? "0" : "") + String(now.month())  + "-" +
            (now.day()    < 10 ? "0" : "") + String(now.day())    + " " +
            (now.hour()   < 10 ? "0" : "") + String(now.hour())   + ":" +
            (now.minute() < 10 ? "0" : "") + String(now.minute());
        EEPROM.writeString(EEPROM_CAL_TIMESTAMP_OFFSET, ts);
        EEPROM.commit();
    }

    Gas& getSelectedGas() { return m_gases[m_selectedGas]; }
    int getSelectedGasIndex() const { return m_selectedGas; }

    double getInterceptByIndex(int index) const {
        switch (index) {
            case 0: return m_intercept;
            case 1: return m_intercept2;
            case 2: return m_intercept3;
            case 3: return m_intercept4;
            default: return 0.0;
        }
    }

    double getSlopeByIndex(int index) const {
        switch (index) {
            case 0: return m_slope;
            case 1: return m_slope2;
            case 2: return m_slope3;
            case 3: return m_slope4;
            default: return 1.0;
        }
    }

    std::vector<Gas>& getAllGases() { return m_gases; }

    void onParamChange(String param, String value) override
    {
        if(param.equals(c_INTERCEPT_PARAM_NAME)) m_intercept = value.toDouble();
        else if(param.equals(c_SLOPE_PARAM_NAME)) m_slope = value.toDouble();
        else if(param.equals(c_INTERCEPT2_PARAM_NAME)) m_intercept2 = value.toDouble();
        else if(param.equals(c_SLOPE2_PARAM_NAME)) m_slope2 = value.toDouble();
        else if(param.equals(c_INTERCEPT3_PARAM_NAME)) m_intercept3 = value.toDouble();
        else if(param.equals(c_SLOPE3_PARAM_NAME)) m_slope3 = value.toDouble();
        else if(param.equals(c_INTERCEPT4_PARAM_NAME)) m_intercept4 = value.toDouble();
        else if(param.equals(c_SLOPE4_PARAM_NAME)) m_slope4 = value.toDouble();
        else if(param.equals(c_GASINDEX_PARAM_NAME)) m_selectedGas = value.toInt();
        else {
            for (auto& gas : m_gases) {
                if (gas.getName().equals(param)) {
                    gas.setThermalConductivity(value.toDouble());
                    break;
                }
            }
        }
    }
};