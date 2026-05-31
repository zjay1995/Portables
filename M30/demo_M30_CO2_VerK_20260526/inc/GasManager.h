#pragma once

#include <vector>

#include <Arduino.h>

#include "ConfigurationManager.h"
#include "Globals.h"
#include <EEPROM.h>
#include "CalvalueSet.h"
#include "AlarmSet.h"
#include <RTClib.h>

#define EEPROM_CAL_TIMESTAMP_OFFSET 120  // Make sure this doesn’t conflict


class Gas
{
    String m_name;
    double m_gcf;   // Gas Correction Factor relative to Air (Air = 1.0)

public:

    Gas(String name, double gcf = 1.0) : m_name(name), m_gcf(gcf) {}

    String getName() { return m_name; }

    double getThermalConductivity() const              { return m_gcf; }
    void   setThermalConductivity(double gcf)          { m_gcf = gcf; }

};



class GasManager : public ParamChangeListener
{
    std::vector<Gas> m_gases;

    int m_selectedGas = 0;

    // Single air-based calibration — GCF applied per gas at runtime
    double m_intercept;   // zero-flow voltage (V)
    double m_slope;       // V / SLM (air-equivalent)
    double m_zero;        // copy of intercept; set by calibrate()
    double m_calgas;      // cal-gas voltage (V); set by calibrate2()

    bool calSuccess = false;

    ConfigurationManager* m_configurationManager;

public:
    GasManager(double intercept, double slope, double zero, double calgas)
        : m_intercept(intercept), m_slope(slope), m_zero(zero), m_calgas(calgas)
    {}

    ~GasManager() {}

    double getIntercept() const { return m_intercept; }
    double getSlope()     const { return m_slope; }

    // Single-slot model — selected intercept/slope is always the air cal values
    double getSelectedIntercept() const { return m_intercept; }
    double getSelectedSlope()     const { return m_slope; }
    double getZero()      const { return m_zero; }
    double getCalgas()    const { return m_calgas; }

    void setIntercept(double i) { m_intercept = i; }
    void setSlope(double p)     { m_slope = p; }
    void setZero(double z)      { m_zero = z; }
    void setCalgas(double c)    { m_calgas = c; }

    bool wasCalibrationSuccessful() const { return calSuccess; }

    // Direct ppm calculation — single gas H2, no GCF correction.
    // ppm = (voltage_V - intercept) / slope
    //
    // Noise floor = 2 % of the selected full-scale range. A few mV of ADC
    // jitter around the zero intercept divides by the (small) slope and
    // shows up as tens of ppm even at true zero gas; the 2 %-of-range floor
    // collapses that to 0 for display / logs / alarms.
    //
    // The range array MUST stay in sync with Range::m_rangeArray in
    // inc/RangeSet.h. EEPROM addr 72 holds the index (0..2).
    double calculateFlow(double voltage) {
        if (m_slope == 0.0) return 0.0;
        double ppm = (voltage - m_intercept) / m_slope;

        static const float rangeArr[] = { 1000.0f, 5000.0f, 10000.0f };
        uint8_t ri = EEPROM.read(72);
        if (ri > 2) ri = 0;
        double noiseFloor = rangeArr[ri] * 0.02;  // 20 / 100 / 200 ppm

        if (ppm < noiseFloor) return 0.0;
        return ppm;
    }



    void setConfigurationManager(ConfigurationManager* c) { m_configurationManager = c; }

    void addGas(Gas g) { m_gases.push_back(g); }

    void printinfo(double currentVoltage) {
        Serial.println("M10-Flow Selected Gas Information:");
        Serial.print("Gas Name: ");
        Serial.println(m_gases[m_selectedGas].getName());
        Serial.print("GCF: ");
        Serial.println(m_gases[m_selectedGas].getThermalConductivity());
        double flowValue = calculateFlow(currentVoltage / 1000.0);
        Serial.print("Calculated Flow (SLM) at Voltage ");
        Serial.print(currentVoltage);
        Serial.print(" mV: ");
        Serial.println(flowValue);
        Serial.println("-------------------------------");
    }


    void selectGasByIndex(int index)
    {
        Serial.println("selectGasByIndex");
        if(index >= 0 && index < m_gases.size())
        {
            m_selectedGas = index;
            //m_configurationManager->saveGasSelectedToEEPROM(index);
            Serial.println("gas index saved " + String(index));
            EEPROM.writeInt(4, index);
            EEPROM.commit();
        }
    }

    // Zero calibration — air-based, single intercept for all gases.
    // Also resets calSuccess so Cal Result won't show stale "Success" from a prior session.
    void calibrate(double zero) {
        calSuccess  = false;
        m_zero      = zero / 1000.0;  // mV → V
        m_intercept = m_zero;
        EEPROM.put(8, m_intercept);
        EEPROM.commit();
        Serial.print("Zero intercept saved: ");
        Serial.println(m_intercept);
    }

    // Span calibration — apply a known reference flow (Air) and compute slope
    void calibrate2(double cal) {
        m_calgas = cal / 1000.0;                        // mV → V
        float refFlow = Calvalue().getSelectedCalvalue(); // SLM (float) from selected menu item
        calSuccess = false;

        Serial.print("Cal voltage (V): "); Serial.println(m_calgas);
        Serial.print("Ref flow (SLM): ");  Serial.println(refFlow);

        if (abs(m_calgas - m_zero) < 0.020) {
            Serial.println("Cal failed: span signal < 20 mV from zero.");
        } else if (refFlow > 0.0f) {
            m_slope = (m_calgas - m_zero) / (double)refFlow;
            EEPROM.put(16, m_slope);
            EEPROM.commit();
            calSuccess = true;
            Serial.print("Slope saved: "); Serial.println(m_slope);
        } else {
            Serial.println("Ref flow is 0 — calibration skipped.");
        }

        // Save calibration date if successful
        if (calSuccess) {
            static RTC_PCF8563 rtc;
            static bool rtcReady = false;
            if (!rtcReady) {
                rtcReady = rtc.begin();
                if (!rtcReady) {
                    Serial.println("RTC not initialized. Skipping cal date save.");
                    return;
                }
            }

            DateTime now = rtc.now();

            String calTimestamp = String(now.year()) + "-" +
                                  (now.month() < 10 ? "0" : "") + String(now.month()) + "-" +
                                  (now.day() < 10 ? "0" : "") + String(now.day()) + " " +
                                  (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" +
                                  (now.minute() < 10 ? "0" : "") + String(now.minute());

            Serial.println("Calibration succeeded. Saving date: " + calTimestamp);

            EEPROM.writeString(EEPROM_CAL_TIMESTAMP_OFFSET, calTimestamp);
            EEPROM.commit();

        }
    }


    void calibrate3(){

    }

    void selectPreviousGas()
    {
        Serial.println("selectPreviousGas");

        if(m_selectedGas == 0)
            m_selectedGas = m_gases.size() - 1;
        else
            m_selectedGas = m_selectedGas - 1;
    }

    Gas& getSelectedGas() { return m_gases[m_selectedGas]; }


    std::vector<Gas>& getAllGases() { return m_gases; }

    void onParamChange(String param, String value)
    {
        Serial.println("onGasParamChange: " + param + "=" + value);

        if (param.equals(c_INTERCEPT_PARAM_NAME)) {
            m_intercept = value.toDouble();
            m_zero      = m_intercept;  // keep in sync
            Serial.println("Setting INTERCEPT: " + value);
        } else if (param.equals(c_SLOPE_PARAM_NAME)) {
            m_slope = value.toDouble();
            Serial.println("Setting SLOPE: " + value);
        } else if (param.equals(c_GASINDEX_PARAM_NAME)) {
            m_selectedGas = value.toInt();
            Serial.println("Setting Gas_Index: " + value);
        } else {
            // Try to match a gas name for GCF override
            for (auto& gas : m_gases) {
                if (gas.getName().equals(param)) {
                    double gcf = value.toDouble();
                    gas.setThermalConductivity(gcf);
                    Serial.println("Setting GCF for gas: " + gas.getName() + " " + String(gcf));
                    break;
                }
            }
        }
    }


};

