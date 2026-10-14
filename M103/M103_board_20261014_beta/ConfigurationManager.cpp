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


    // Helper: EEPROM bytes 0xFF on a blank board produce a signaling NaN when
    // read as a double. Passing NaN to String()/dtostrf() overflows the small
    // internal char buffer and smashes the stack. Sanitise every double read
    // back to the -1 sentinel before using it.
    #define SAFE_EEPROM_DOUBLE(addr) \
        ([&]() -> double { double _v = EEPROM.readDouble(addr); return isnan(_v) ? -1.0 : _v; }())

    // NOTE: cal factors are forwarded with 9 decimal places. The default
    // String(double) keeps only 2, which truncated small slopes (e.g.
    // 0.004061 -> "0.00" -> 0.0) and zeroed the calibration on every boot.
	// INTERCEPT
	double intercept = SAFE_EEPROM_DOUBLE(EEPROM_GAS_INTERCEPT_OFFSET);
	if(intercept != -1)
		notifyParamChanged(c_INTERCEPT_PARAM_NAME, String(intercept, 9));
	Serial.println("config: intercept: " + String(intercept, 9));

    double intercept2 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_INTERCEPT2_OFFSET);
    if(intercept2 != -1)
        notifyParamChanged(c_INTERCEPT2_PARAM_NAME, String(intercept2, 9));
    Serial.println("config: intercept2: " + String(intercept2, 9));

    double intercept3 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_INTERCEPT3_OFFSET);
    if(intercept3 != -1)
        notifyParamChanged(c_INTERCEPT3_PARAM_NAME, String(intercept3, 9));
    Serial.println("config: intercept3: " + String(intercept3, 9));

    double intercept4 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_INTERCEPT4_OFFSET);
    if(intercept4 != -1)
        notifyParamChanged(c_INTERCEPT4_PARAM_NAME, String(intercept4, 9));
    Serial.println("config: intercept4: " + String(intercept4, 9));

	// Slope
    double slope = SAFE_EEPROM_DOUBLE(EEPROM_GAS_SLOPE_OFFSET);
    if(slope != -1)
        notifyParamChanged(c_SLOPE_PARAM_NAME, String(slope, 9));
    Serial.println("config: slope: " + String(slope, 9));

    double slope2 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_SLOPE2_OFFSET);
    if(slope2 != -1)
        notifyParamChanged(c_SLOPE2_PARAM_NAME, String(slope2, 9));
    Serial.println("config: slope2: " + String(slope2, 9));

    double slope3 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_SLOPE3_OFFSET);
    if(slope3 != -1)
        notifyParamChanged(c_SLOPE3_PARAM_NAME, String(slope3, 9));
    Serial.println("config: slope3: " + String(slope3, 9));

    double slope4 = SAFE_EEPROM_DOUBLE(EEPROM_GAS_SLOPE4_OFFSET);
    if(slope4 != -1)
        notifyParamChanged(c_SLOPE4_PARAM_NAME, String(slope4, 9));
    Serial.println("config: slope4: " + String(slope4, 9));


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

	// This is WebServer changing params so notify others too!
	notifyParamChanged(param, value);
}



void ConfigurationManager:: saveInterceptToEEPROM(double intercept, bool doCommit)
{
	if(m_loadAllInProgress)
		return;
	Serial.println("EEPROM saveInterceptToEEPROM: " + String(intercept));
	size_t written = EEPROM.writeDouble(EEPROM_GAS_INTERCEPT_OFFSET, intercept);
	Serial.println("EEPROM written: " + String(written));
	if(doCommit && !EEPROM.commit())
		Serial.println("EEPROM commit ERROR!!!");
}

void ConfigurationManager:: saveSlopeToEEPROM(double slope, bool doCommit)
{
    if(m_loadAllInProgress)
        return;
    Serial.println("EEPROM saveSlopeToEEPROM: " + String(slope));
    size_t written = EEPROM.writeDouble(EEPROM_GAS_SLOPE_OFFSET, slope);
    Serial.println("EEPROM written: " + String(written));
    if(doCommit && !EEPROM.commit())
        Serial.println("EEPROM commit ERROR!!!");
}



 void ConfigurationManager::saveRangeToEEPROM(int range, bool doCommit)
 {

     Serial.println("EEPROM saveRangeToEEPROM: " + String(range));
     EEPROM.writeInt(72, range);
     EEPROM.commit();
 }

 void ConfigurationManager::saveAlarmToEEPROM(int alarm, bool doCommit)
 {

     Serial.println("EEPROM saveAlarmToEEPROM: " + String(alarm));
     EEPROM.writeInt(76, alarm);
     EEPROM.commit();
 }

 void ConfigurationManager::saveLowalarmToEEPROM(int lowalarm, bool doCommit)
 {

     Serial.println("EEPROM saveLowAlarmToEEPROM: " + String(lowalarm));
     EEPROM.writeInt(92, lowalarm);
     EEPROM.commit();
 }



 void ConfigurationManager::saveCalvalueToEEPROM(int calvalue, bool doCommit)
 {

     Serial.println("EEPROM saveCalvalueToEEPROM: " + String(calvalue));
     EEPROM.writeInt(80, calvalue);
     EEPROM.commit();

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
