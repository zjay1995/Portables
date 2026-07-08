#pragma once
#include <Arduino.h>

class SleepTimer;
class GasManager;
class ParamChangeListener;


class ParamChangeListener
{
public:

	virtual void onParamChange(String param, String value) = 0;

};


class ConfigurationManager : public ParamChangeListener
{

    // Must cover the highest used address: time backup at 240-244 (see
    // inc/TimeSync.h). The old computed value (236) silently dropped every
    // write at >= 236, so the EEPROM time backup never persisted.
    const uint16_t EEPROM_SIZE = 512;
	
    const uint8_t EEPROM_GAS_INTERCEPT_OFFSET = 8;
    const uint8_t EEPROM_GAS_SLOPE_OFFSET = 16;

    const uint8_t EEPROM_GAS_INTERCEPT2_OFFSET = 24;
    const uint8_t EEPROM_GAS_SLOPE2_OFFSET = 32;

    const uint8_t EEPROM_GAS_INTERCEPT3_OFFSET = 40;
    const uint8_t EEPROM_GAS_SLOPE3_OFFSET = 48;

    const uint8_t EEPROM_GAS_INTERCEPT4_OFFSET = 56;
    const uint8_t EEPROM_GAS_SLOPE4_OFFSET = 64;

    const uint8_t EEPROM_RANGE_OFFSET = 72;
    const uint8_t EEPROM_ALARM_OFFSET = 76;
    const uint8_t EEPROM_Calvalue_OFFSET = 80;
    const uint8_t EEPROM_LOWALARM_OFFSET = 92;
    const uint8_t EEPROM_LOGGINGSET_OFFSET = 96;
    const uint8_t EEPROM_LOGGINGTIME_OFFSET = 200;

    bool m_loadAllInProgress = false;

	std::vector<ParamChangeListener*> m_paramChangeListeners;

public:

	ConfigurationManager();

	void init();

	void loadFromEEPROM();

    void saveRangeToEEPROM(int range, bool doCommit = true);
    void saveAlarmToEEPROM(int alarm, bool doCommit = true);
    void saveLowalarmToEEPROM(int lowalarm, bool doCommit = true);

    void saveCalvalueToEEPROM(int calvalue, bool doCommit = true);

	void saveInterceptToEEPROM(double intercept, bool doCommit = true);
    void saveSlopeToEEPROM(double slope, bool doCommit = true);

	void setupEEPROM();
	
	void clearEEPROM();

	void onParamChange(String param, String value);


	void addParamChangeListener(ParamChangeListener* l);
	void notifyParamChanged(String param, String value);

};
