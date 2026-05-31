#include "inc/ConfigurationManager.h"
#include <EEPROM.h>

#include "inc/Globals.h"
#include "inc/SleepTimer.h"
#include "inc/GasManager.h"

ConfigurationManager::ConfigurationManager()
{
}

void ConfigurationManager::init()
{
    setupEEPROM();
}

void ConfigurationManager::loadFromEEPROM()
{
    m_loadAllInProgress = true;
    
    int timerInterval = EEPROM.readInt(EEPROM_TIMER_OFFSET);
    Serial.println("config: timer interval: " + String(timerInterval));
    
    int range = EEPROM.read(72);
    Serial.println("config: range: " + String(range));
    notifyParamChanged(c_RANGE_PARAM_NAME, String(range));

    int alarm = EEPROM.read(76);
    Serial.println("config: alarm: " + String(alarm));
    notifyParamChanged(c_ALARM_PARAM_NAME, String(alarm));

    int lowalarm = EEPROM.read(92);
    Serial.println("config: lowalarm: " + String(lowalarm));
    notifyParamChanged(c_LOWALARM_PARAM_NAME, String(lowalarm));

    int gasIndex = EEPROM.readInt(4);
    Serial.println("config: gas index: " + String(gasIndex));
    notifyParamChanged(c_GASINDEX_PARAM_NAME, String(gasIndex));

    // --- INTERCEPTS (Defaults to 0.0 for new boards) ---
    double intercept = EEPROM.readDouble(EEPROM_GAS_INTERCEPT_OFFSET);
    if (intercept == -1.0 || isnan(intercept)) intercept = 0.0;
    notifyParamChanged(c_INTERCEPT_PARAM_NAME, String(intercept, 6));
    Serial.println("config: intercept: " + String(intercept, 6));

    double intercept2 = EEPROM.readDouble(EEPROM_GAS_INTERCEPT2_OFFSET);
    if (intercept2 == -1.0 || isnan(intercept2)) intercept2 = 0.0;
    notifyParamChanged(c_INTERCEPT2_PARAM_NAME, String(intercept2, 6));
    Serial.println("config: intercept2: " + String(intercept2, 6));

    double intercept3 = EEPROM.readDouble(EEPROM_GAS_INTERCEPT3_OFFSET);
    if (intercept3 == -1.0 || isnan(intercept3)) intercept3 = 0.0;
    notifyParamChanged(c_INTERCEPT3_PARAM_NAME, String(intercept3, 6));
    Serial.println("config: intercept3: " + String(intercept3, 6));

    double intercept4 = EEPROM.readDouble(EEPROM_GAS_INTERCEPT4_OFFSET);
    if (intercept4 == -1.0 || isnan(intercept4)) intercept4 = 0.0;
    notifyParamChanged(c_INTERCEPT4_PARAM_NAME, String(intercept4, 6));
    Serial.println("config: intercept4: " + String(intercept4, 6));

    // --- SLOPES (Defaults to 1.0 for new boards to prevent div-by-zero) ---
    double slope = EEPROM.readDouble(EEPROM_GAS_SLOPE_OFFSET);
    if (slope == -1.0 || isnan(slope) || slope == 0.0) slope = 1.0;
    notifyParamChanged(c_SLOPE_PARAM_NAME, String(slope, 6));
    Serial.println("config: slope: " + String(slope, 6));

    double slope2 = EEPROM.readDouble(EEPROM_GAS_SLOPE2_OFFSET);
    if (slope2 == -1.0 || isnan(slope2) || slope2 == 0.0) slope2 = 1.0;
    notifyParamChanged(c_SLOPE2_PARAM_NAME, String(slope2, 6));
    Serial.println("config: slope2: " + String(slope2, 6));

    double slope3 = EEPROM.readDouble(EEPROM_GAS_SLOPE3_OFFSET);
    if (slope3 == -1.0 || isnan(slope3) || slope3 == 0.0) slope3 = 1.0;
    notifyParamChanged(c_SLOPE3_PARAM_NAME, String(slope3, 6));
    Serial.println("config: slope3: " + String(slope3, 6));

    double slope4 = EEPROM.readDouble(EEPROM_GAS_SLOPE4_OFFSET);
    if (slope4 == -1.0 || isnan(slope4) || slope4 == 0.0) slope4 = 1.0;
    notifyParamChanged(c_SLOPE4_PARAM_NAME, String(slope4, 6));
    Serial.println("config: slope4: " + String(slope4, 6));

    // --- Gas TCs ---
    double AirTC = EEPROM.readDouble(EEPROM_GAS_AIR_TC_OFFSET);
    if(AirTC != -1.0 && !isnan(AirTC)) notifyParamChanged(c_GAS_AIR_PARAM_NAME, String(AirTC));
    
    double O2TC = EEPROM.readDouble(EEPROM_GAS_O2_TC_OFFSET);
    if(O2TC != -1.0 && !isnan(O2TC)) notifyParamChanged(c_GAS_O2_PARAM_NAME, String(O2TC));
    
    double N2TC = EEPROM.readDouble(EEPROM_GAS_N2_TC_OFFSET);    
    if(N2TC != -1.0 && !isnan(N2TC)) notifyParamChanged(c_GAS_N2_PARAM_NAME, String(N2TC));
    
    double HeTC = EEPROM.readDouble(EEPROM_GAS_He_TC_OFFSET);
    if(HeTC != -1.0 && !isnan(HeTC)) notifyParamChanged(c_GAS_He_PARAM_NAME, String(HeTC));
    
    double H2TC = EEPROM.readDouble(EEPROM_GAS_H2_TC_OFFSET);        
    if(H2TC != -1.0 && !isnan(H2TC)) notifyParamChanged(c_GAS_H2_PARAM_NAME, String(H2TC));
    
    double ArCH4TC = EEPROM.readDouble(EEPROM_GAS_ArCH4_TC_OFFSET);        
    if(ArCH4TC != -1.0 && !isnan(ArCH4TC)) notifyParamChanged(c_GAS_ARCH4_PARAM_NAME, String(ArCH4TC));

    m_loadAllInProgress = false;
    return;     
}

void ConfigurationManager::addParamChangeListener(ParamChangeListener* l)
{
    m_paramChangeListeners.push_back(l);
}

void ConfigurationManager::notifyParamChanged(String param, String value)
{
    for (auto& l : m_paramChangeListeners)
        l->onParamChange(param, value);
}

void ConfigurationManager::onParamChange(String param, String value)
{
    if (param.equals(c_INTERCEPT_PARAM_NAME))
    {
        Serial.println("EEPROM save INTERCEPT: " + value);
        EEPROM.writeDouble(EEPROM_GAS_INTERCEPT_OFFSET, value.toDouble());
    }
    else if (param.equals(c_SLOPE_PARAM_NAME))
    {
        Serial.println("EEPROM save SLOPE: " + value);
        EEPROM.writeDouble(EEPROM_GAS_SLOPE_OFFSET, value.toDouble());
    }

    if (!EEPROM.commit())
        Serial.println("EEPROM commit ERROR!!!");

    notifyParamChanged(param, value);
}

void ConfigurationManager::saveInterceptToEEPROM(double intercept, bool doCommit)
{
    if(m_loadAllInProgress) return;
    Serial.println("EEPROM saveInterceptToEEPROM: " + String(intercept));
    EEPROM.writeDouble(EEPROM_GAS_INTERCEPT_OFFSET, intercept);
    if(doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::saveSlopeToEEPROM(double slope, bool doCommit)
{
    if(m_loadAllInProgress) return;
    Serial.println("EEPROM saveSlopeToEEPROM: " + String(slope));
    EEPROM.writeDouble(EEPROM_GAS_SLOPE_OFFSET, slope);
    if(doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::saveTimerIntervalToEEPROM(int interval, bool doCommit)
{   
    if(m_loadAllInProgress) return;
    Serial.println("EEPROM saveTimerIntervalToEEPROM: " + String(interval));
    EEPROM.put(EEPROM_TIMER_OFFSET, interval);
    if(doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::saveRangeToEEPROM(int range, bool doCommit)
{
    Serial.println("EEPROM saveRangeToEEPROM: " + String(range));
    EEPROM.writeInt(72, range);
    if(doCommit) EEPROM.commit();
}

void ConfigurationManager::saveAlarmToEEPROM(int alarm, bool doCommit)
{
    Serial.println("EEPROM saveAlarmToEEPROM: " + String(alarm));
    EEPROM.writeInt(76, alarm);
    if(doCommit) EEPROM.commit();
}

void ConfigurationManager::saveLowalarmToEEPROM(int lowalarm, bool doCommit)
{
    Serial.println("EEPROM saveLowAlarmToEEPROM: " + String(lowalarm));
    EEPROM.writeInt(92, lowalarm);
    if(doCommit) EEPROM.commit();
}

void ConfigurationManager::saveCalvalueToEEPROM(int calvalue, bool doCommit)
{
    Serial.println("EEPROM saveCalvalueToEEPROM: " + String(calvalue));
    EEPROM.writeInt(80, calvalue);
    if(doCommit) EEPROM.commit();
}

void ConfigurationManager::saveWifiSSID(String ssid, bool doCommit)
{
    if (m_loadAllInProgress) return;
    Serial.println("EEPROM saveWifiSSID: " + ssid);
    EEPROM.writeString(EEPROM_WIFI_SSID_OFFSET, ssid.c_str());
    if (doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::saveWifiPassword(String password, bool doCommit)
{
    if (m_loadAllInProgress) return;
    Serial.println("EEPROM saveWifiPassword: " + password);
    EEPROM.writeString(EEPROM_WIFI_PASSWORD_OFFSET, password.c_str());
    if (doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::saveDeviceId(String id, bool doCommit)
{
    if (m_loadAllInProgress) return;
    Serial.println("EEPROM saveDeviceId: " + id);
    EEPROM.writeString(EEPROM_DEVICE_ID_OFFSET, id.c_str());
    if (doCommit && !EEPROM.commit()) Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager::setupEEPROM()
{
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("EEPROM start failed");
        delay(60000);
        return;
    }
    Serial.println("EEPROM begin!!");
}

void ConfigurationManager::clearEEPROM()
{
    for (size_t i = 0; i < EEPROM_SIZE; ++i)
        EEPROM.writeByte(i, 0xFF);
    EEPROM.commit();
}