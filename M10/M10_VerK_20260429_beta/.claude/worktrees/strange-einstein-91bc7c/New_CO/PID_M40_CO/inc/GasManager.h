#pragma once

#include <vector>
#include <array>
#include <Arduino.h>
#include "ConfigurationManager.h"
#include "Globals.h"
#include <EEPROM.h>
#include "CalvalueSet.h"
#include "AlarmSet.h"
#include <RTClib.h>

#define EEPROM_CAL_TIMESTAMP_OFFSET 120
#define MAX_GAS_SUPPORTED 4

// Structure to hold calibration data for a single gas
struct GasParams {
    double intercept;
    double slope;
};

class Gas
{
    String m_name;
    double m_thermalConductivity;

public:
    Gas(String name, double thermalConductivity = 1.0)
            : m_name(name), m_thermalConductivity(thermalConductivity) {}

    String getName() { return m_name; }
    double getThermalConductivity() const { return m_thermalConductivity; }
    void setThermalConductivity(double tc) { m_thermalConductivity = tc; }
};

class GasManager : public ParamChangeListener
{
    std::vector<Gas> m_gases;
    std::array<GasParams, MAX_GAS_SUPPORTED> m_gasParams; // Array replacing individual variables

    int m_selectedGas = 0; // Default to 0, overwritten by EEPROM in constructor or init

    // Temporary variables for the active calibration session
    double m_zero = 0.0;
    double m_calgas = 0.0;
    bool calSuccess = false;

    ConfigurationManager* m_configurationManager;

public:
    // Constructor initializes the array from the passed arguments.
    // Note: ideally this should load from EEPROM, but we keep the signature compatible with your current setup.
    GasManager(double intercept1, double slope1,
               double zero, double calgas,
               double intercept2, double slope2,
               double intercept3, double slope3,
               double intercept4, double slope4)
    {
        m_gasParams[0] = {intercept1, slope1};
        m_gasParams[1] = {intercept2, slope2};
        m_gasParams[2] = {intercept3, slope3};
        m_gasParams[3] = {intercept4, slope4};

        m_zero = zero;
        m_calgas = calgas;

        // Initialize selected gas from EEPROM immediately
        m_selectedGas = EEPROM.readInt(4);
        if (m_selectedGas < 0 || m_selectedGas >= MAX_GAS_SUPPORTED) m_selectedGas = 0;
    }

    ~GasManager() {}

    // --- Getters (Unified) ---
    // Returns the currently selected gas params
    double getIntercept() const { return m_gasParams[m_selectedGas].intercept; }
    double getSlope() const { return m_gasParams[m_selectedGas].slope; }

    // Access specific gas params by index (replacing getIntercept2, etc.)
    double getIntercept(int index) const {
        if(index >= 0 && index < MAX_GAS_SUPPORTED) return m_gasParams[index].intercept;
        return 0.0;
    }
    double getSlope(int index) const {
        if(index >= 0 && index < MAX_GAS_SUPPORTED) return m_gasParams[index].slope;
        return 1.0;
    }

    double getZero() const { return m_zero; }
    double getCalgas() const { return m_calgas; }

    // --- Setters (Unified) ---
    void setIntercept(int index, double val) {
        if(index >= 0 && index < MAX_GAS_SUPPORTED) m_gasParams[index].intercept = val;
    }
    void setSlope(int index, double val) {
        if(index >= 0 && index < MAX_GAS_SUPPORTED) m_gasParams[index].slope = val;
    }

    void setZero(double z) { m_zero = z; }
    void setCalgas(double c) { m_calgas = c; }

    bool wasCalibrationSuccessful() const { return calSuccess; }

    void setConfigurationManager(ConfigurationManager* c) { m_configurationManager = c; }
    void addGas(Gas g) { m_gases.push_back(g); }

    // --- Core Logic ---

    double calculateSLM(double voltage) {
        // No more if/else chain. Uses the currently selected gas parameters.
        double intercept = m_gasParams[m_selectedGas].intercept;
        double slope = m_gasParams[m_selectedGas].slope;

        // Avoid division by zero if slope is corrupted
        if (slope == 0) slope = 1.0;

        double val = ((voltage - intercept) / slope) * 1000.0;
        return val < 0 ? 0 : val;
    }

    void selectGasByIndex(int index) {
        Serial.println("selectGasByIndex: " + String(index));
        if (index >= 0 && index < m_gases.size() && index < MAX_GAS_SUPPORTED) {
            m_selectedGas = index;
            EEPROM.writeInt(4, index);
            EEPROM.commit();
            Serial.println("Gas index saved.");
        }
    }

    void selectPreviousGas() {
        int newIndex = m_selectedGas - 1;
        if (newIndex < 0) newIndex = m_gases.size() - 1;
        selectGasByIndex(newIndex);
    }

    Gas& getSelectedGas() { return m_gases[m_selectedGas]; }
    std::vector<Gas>& getAllGases() { return m_gases; }
    int getSelectedGasIndex() { return m_selectedGas; } // Added helper

    // --- Calibration Logic ---

    // Calibrate Zero (Intercept)
    void calibrate(double zero) {
        m_zero = zero / 1000.0; // Convert to Volts? (Assuming input is mV)
        delay(20);

        // Update RAM
        m_gasParams[m_selectedGas].intercept = m_zero;

        // Calculate EEPROM address dynamically
        // Gas 0: 8, Gas 1: 24, Gas 2: 40... (Base 8 + 16*index)
        int address = 8 + (m_selectedGas * 16);

        Serial.print("Calibrating Zero for gas "); Serial.println(m_selectedGas);
        EEPROM.put(address, m_gasParams[m_selectedGas].intercept);
        EEPROM.commit();

        Serial.print("Intercept Saved: "); Serial.println(m_gasParams[m_selectedGas].intercept);
    }

    // Calibrate Span (Slope)
    void calibrate2(double calRawMv) {
        m_calgas = calRawMv / 1000.0; // Current sensor voltage in Volts

        int calValInt = Calvalue().getSelectedCalvalue(); // e.g. 100 ppm
        double targetConc = calValInt / 1000.0; // Scaled target? (Check unit logic)

        calSuccess = false;

        Serial.print("Calibrating Span for gas "); Serial.println(m_selectedGas);
        Serial.print("Target: "); Serial.println(targetConc);
        Serial.print("Read(V): "); Serial.println(m_calgas);

        if (targetConc == 0) {
            Serial.println("Error: Calibration target cannot be 0");
            return;
        }

        // Formula: Slope = (Volts_Span - Volts_Zero) / Concentration
        double newSlope = (m_calgas - m_zero) / targetConc;

        // Update RAM
        m_gasParams[m_selectedGas].slope = newSlope;

        // Calculate EEPROM address for Slope
        // Gas 0: 16, Gas 1: 32... (Base 16 + 16*index)
        int address = 16 + (m_selectedGas * 16);

        EEPROM.put(address, m_gasParams[m_selectedGas].slope);
        EEPROM.commit();
        calSuccess = true;

        Serial.print("Slope Saved: "); Serial.println(newSlope);

        saveCalibrationTimestamp();
    }

    void saveCalibrationTimestamp() {
        RTC_PCF8563 rtc;
        if (!rtc.begin()) return;

        DateTime now = rtc.now();
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
                 now.year(), now.month(), now.day(), now.hour(), now.minute());

        Serial.print("Saving Cal Date: "); Serial.println(buf);
        EEPROM.writeString(EEPROM_CAL_TIMESTAMP_OFFSET, String(buf));
        EEPROM.commit();
    }

    // --- Parameter Change Listener ---

    void onParamChange(String param, String value) override {
        Serial.println("onGasParamChange: " + param + "=" + value);
        double val = value.toDouble();

        // Map parameter names to indices and types
        // This keeps compatibility with Globals.h constants
        if      (param.equals(c_INTERCEPT_PARAM_NAME))  setIntercept(0, val);
        else if (param.equals(c_SLOPE_PARAM_NAME))      setSlope(0, val);
        else if (param.equals(c_INTERCEPT2_PARAM_NAME)) setIntercept(1, val);
        else if (param.equals(c_SLOPE2_PARAM_NAME))     setSlope(1, val);
        else if (param.equals(c_INTERCEPT3_PARAM_NAME)) setIntercept(2, val);
        else if (param.equals(c_SLOPE3_PARAM_NAME))     setSlope(2, val);
        else if (param.equals(c_INTERCEPT4_PARAM_NAME)) setIntercept(3, val);
        else if (param.equals(c_SLOPE4_PARAM_NAME))     setSlope(3, val);
        else if (param.equals(c_GASINDEX_PARAM_NAME)) {
            m_selectedGas = (int)val;
            Serial.println("Gas Index Updated: " + value);
        }
        else {
            // Check for gas name changes
            for (auto& gas : m_gases) {
                if (gas.getName().equals(param)) {
                    gas.setThermalConductivity(val);
                    Serial.println("TC updated for: " + gas.getName());
                    break;
                }
            }
        }
    }

    // Debugging Helper
    void printinfo(double currentVoltageMv) {
        if (m_selectedGas >= m_gases.size()) return;

        Serial.println("--- Gas Info ---");
        Serial.print("Gas: "); Serial.println(m_gases[m_selectedGas].getName());
        Serial.print("Intercept: "); Serial.println(getIntercept());
        Serial.print("Slope: "); Serial.println(getSlope());

        double gasLevel = calculateSLM(currentVoltageMv / 1000.0);
        Serial.print("Calc Level (Volts="); Serial.print(currentVoltageMv/1000.0);
        Serial.print("): "); Serial.println(gasLevel);
        Serial.println("----------------");
    }
};