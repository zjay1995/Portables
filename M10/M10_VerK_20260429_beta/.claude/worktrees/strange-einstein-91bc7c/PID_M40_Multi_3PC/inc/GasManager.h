#pragma once

#include <vector>

#include <Arduino.h>

#include "ConfigurationManager.h"
#include "Globals.h"
#include <EEPROM.h>
#include "CalvalueSet.h"
#include "AlarmSet.h"


class Gas
{
    String m_name;
    double m_intercept;
    double m_slope;
    double m_thermalConductivity;
public:

    Gas(String name, double thermalConductivity = 1.0) : m_name(name),
                                                         m_thermalConductivity(thermalConductivity)
    {

    }

    String getName() { return m_name; }

    double getThermalConductivity() const 					{ return m_thermalConductivity; }
    void setThermalConductivity(double thermalConductivity) { m_thermalConductivity = thermalConductivity; }

};



class GasManager : public ParamChangeListener
{
    std::vector<Gas> m_gases;

    int m_selectedGas =EEPROM.readInt(4);

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
    bool calSuccess;

    double voltage_zero, voltage_low, voltage_high;
    double calgas_low, calgas_high;

    ConfigurationManager* m_configurationManager;

public:
    GasManager(double intercept, double slope,
               double zero, double calgas, double intercept2,double slope2,
               double intercept3,double slope3,double intercept4,double slope4,
               double voltage_zero, double voltage_low, double voltage_high,
               double calgas_low, double calgas_high,bool calSuccess) :
                                                                                      m_intercept(intercept),
                                                                                      m_slope(slope),
                                                                                      m_intercept2(intercept2),
                                                                                      m_slope2(slope2),
                                                                                      m_intercept3(intercept3),
                                                                                      m_slope3(slope3),
                                                                                      m_intercept4(intercept4),
                                                                                      m_slope4(slope4),
                                                                                      m_zero(zero),
                                                                                      m_calgas(calgas),
                                                                                      voltage_zero(0),
                                                                                      voltage_low(0),
                                                                                      voltage_high(0),
                                                                                      calgas_low(0),
                                                                                      calgas_high(0),
                                                                                      calSuccess(false)


    {}

    ~GasManager() {}

    double getIntercept() const { return m_intercept; }
    double getSlope() const { return m_slope; }
    double getIntercept2() const { return m_intercept2; }
    double getSlope2() const { return m_slope2; }
    double getIntercept3() const { return m_intercept3; }
    double getSlope3() const { return m_slope3; }
    double getIntercept4() const { return m_intercept4; }
    double getSlope4() const { return m_slope4; }
    double getZero() const {return m_zero; }
    double getCalgas() const {return m_calgas; }

    void setIntercept(double i) { m_intercept = i; }
    void setSlope(double p) {m_slope = p; }
    void setIntercept2(double ii) { m_intercept2 = ii; }
    void setSlope2(double pp) {m_slope2 = pp; }
    void setIntercept3(double iii) { m_intercept3 = iii; }
    void setSlope3(double ppp) {m_slope3 = ppp; }
    void setIntercept4(double iiii) { m_intercept4 = iiii; }
    void setSlope4(double pppp) {m_slope4 = pppp; }
    void setZero(double z) {m_zero = z; }
    void setCalgas(double c) {m_calgas = c;}

    bool wasCalibrationSuccessful() const {
        return calSuccess;
    }

    double calculateSLM(double voltage) {
        double val = 0;
        val = ((voltage - m_intercept ) / m_slope)  ; //votlage in volts

        return val < 0 ? 0 : val;
    }



    void setConfigurationManager(ConfigurationManager* c) { m_configurationManager = c; }

    void addGas(Gas g) { m_gases.push_back(g); }

    void printinfo(double currentVoltage) {
        // Print information about the gas configuration
        Serial.println("M # Selected Gas Information:");
        Serial.print("Gas Name: ");
        Serial.println(m_gases[m_selectedGas].getName());

        Serial.println(getCalgas());

        // Calculate gas level using the provided current voltage
        double gasLevel = calculateSLM(currentVoltage/1000);
        Serial.print("Calculated Gas Level (SLM) at Voltage ");
        Serial.print(currentVoltage);  // Use the passed voltage
        Serial.print(" mV: ");
        Serial.println(gasLevel);
        Serial.println("-------------------------------");
    }


    void selectGasByIndex(int index)
    {
        Serial.println("selectGasByIndex");
        if(index >= 0 && index < m_gases.size())
        {
            m_selectedGas = index;
            //m_configurationManager->saveGasSelectedToEEPROM(index);
            Serial.println("gas index saved"+index);
            EEPROM.writeInt(4, index);
            EEPROM.commit();
        }
    }

    void calibrate(double zero) {
        voltage_zero = zero/1000;  // in V
        Serial.println("Zero point calibrated.");
    }

    // Calibrate low point, retrieves concentration from EEPROM
    void calibrate2(double low) {
        voltage_low = low/1000;  // in V
        calgas_low = 1;
        Serial.println("Low point calibrated.");
        Serial.println(voltage_low);
    }

    // Calibrate high point, retrieves concentration from EEPROM
    void calibrate3(double high) {
        voltage_high = high/1000;  // in V
        calgas_high = 4;  // Read high concentration (calgas_high)
        Serial.println("High point calibrated.");
        Serial.println(voltage_high);
    }

    void calculateSI2() {
        // Ensure the calibration points are valid
            // Calculate slope using only voltage_zero and voltage_low
            if(voltage_low != voltage_zero) {
                m_slope = (voltage_low - voltage_zero) / calgas_low;

                // Calculate intercept using voltage_zero and voltage_low
                m_intercept = voltage_zero;  // Setting zero concentration at voltage_zero

                Serial.print("Slope (SI2) calculated: ");
                Serial.println(m_slope);
                Serial.print("Intercept (SI2) calculated: ");
                Serial.println(m_intercept);

                // Store slope and intercept in EEPROM for persistence
                EEPROM.put(16, m_slope);
                EEPROM.put(8, m_intercept);
                EEPROM.commit();
                calSuccess = true;
            }else{
                calSuccess = false;

            }

    }


    // Calculate slope and intercept after setting zero, low, and high points
    void calculateSlopeIntercept() {
        if ((voltage_zero != voltage_low) && (voltage_zero != voltage_high) && (voltage_low != voltage_high)) {
            double sum_x = voltage_zero + voltage_low + voltage_high;
            double sum_y = 0 + 1 + 4;
            double sum_xy = (voltage_low * calgas_low) + (voltage_high * calgas_high);
            double sum_x2 = (voltage_zero * voltage_zero) + (voltage_low * voltage_low) + (voltage_high * voltage_high);
            int n = 3;

            // Calculate slope (m)
            m_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);

            // Calculate intercept (b)
            m_intercept = (sum_y - m_slope * sum_x) / n;
            Serial.print("Slope calculated: ");
            Serial.println(m_slope);
            Serial.print("Intercept calculated: ");
            Serial.println(m_intercept);
            EEPROM.put(16, m_slope);
            EEPROM.put(8, m_intercept);
            EEPROM.commit();
            calSuccess = true;
        } else {
            Serial.println("Invalid calibration points for regression.");
            calSuccess = false;
        }
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

        if(param.equals(c_INTERCEPT_PARAM_NAME))
        {
            m_intercept = value.toDouble();
            Serial.println("Setting INTERCEPT: " + value);
        }
        else if(param.equals(c_SLOPE_PARAM_NAME))
        {
            m_slope = value.toDouble();
            Serial.println("Setting SLOPE: " + value);
        }
        else if(param.equals(c_INTERCEPT2_PARAM_NAME))
        {
            m_intercept2 = value.toDouble();
            Serial.println("Setting INTERCEPT2: " + value);
        }
        else if(param.equals(c_SLOPE2_PARAM_NAME))
        {
            m_slope2 = value.toDouble();
            Serial.println("Setting SLOPE2: " + value);
        }
        else if(param.equals(c_INTERCEPT3_PARAM_NAME))
        {
            m_intercept3 = value.toDouble();
            Serial.println("Setting INTERCEPT3: " + value);
        }
        else if(param.equals(c_SLOPE3_PARAM_NAME))
        {
            m_slope3 = value.toDouble();
            Serial.println("Setting SLOPE3: " + value);
        }
        else if(param.equals(c_INTERCEPT4_PARAM_NAME))
        {
            m_intercept4 = value.toDouble();
            Serial.println("Setting INTERCEPT4: " + value);
        }
        else if(param.equals(c_SLOPE4_PARAM_NAME))
        {
            m_slope4 = value.toDouble();
            Serial.println("Setting SLOPE4: " + value);
        }
        else if(param.equals(c_GASINDEX_PARAM_NAME))
        {
            m_selectedGas = value.toDouble();
            Serial.println("Setting Gas_Index: " + value);
        }
        else
        {
            for (auto& gas : m_gases)
            {
                if (gas.getName().equals(param))
                {
                    double tc = value.toDouble();
                    gas.setThermalConductivity(tc);
                    Serial.println("Setting TC for gas: " + gas.getName() + " " + String(tc));
                    break;
                }
            }
        }
    }


};

