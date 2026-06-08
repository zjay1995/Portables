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

    // The EEPROM layout uses explicit byte offsets (see below) that extend up
    // to the first-boot "seeded" sentinel at address 250 and the DataLogger
    // counters around 210-230. The old size-by-component formula evaluated to
    // 236, which left the sentinel (and those counters) out of bounds: on
    // ESP32, EEPROM.read() of an out-of-range address returns 0, so the
    // sentinel never read back as 0xA8 and the defaults re-seeded on EVERY
    // boot -- silently wiping any saved calibration. Size the region to cover
    // the whole explicit map with headroom.
    const uint16_t EEPROM_SIZE = 512;
	
    const uint8_t EEPROM_TIMER_OFFSET = 0;
    const uint8_t EEPROM_GAS_SELECT_OFFSET = 4;

    const uint8_t EEPROM_GAS_INTERCEPT_OFFSET = 8;
    const uint8_t EEPROM_GAS_SLOPE_OFFSET = 16;

    const uint8_t EEPROM_GAS_INTERCEPT2_OFFSET = 24;
    const uint8_t EEPROM_GAS_SLOPE2_OFFSET = 32;

    const uint8_t EEPROM_GAS_INTERCEPT3_OFFSET = 40;
    const uint8_t EEPROM_GAS_SLOPE3_OFFSET = 48;

    const uint8_t EEPROM_GAS_INTERCEPT4_OFFSET = 56;
    const uint8_t EEPROM_GAS_SLOPE4_OFFSET = 64;


    //const uint8_t EEPROM_GAS_AIR_TC_OFFSET = 28;
    //const uint8_t EEPROM_GAS_O2_TC_OFFSET = 36;
    //const uint8_t EEPROM_GAS_N2_TC_OFFSET = 44;
    //const uint8_t EEPROM_GAS_He_TC_OFFSET = 52;
    //const uint8_t EEPROM_GAS_H2_TC_OFFSET = 60;
    //const uint8_t EEPROM_GAS_ArCH4_TC_OFFSET = 68;
    const uint8_t EEPROM_DEVICE_ID_OFFSET = 84;			//64 chars
    const uint8_t EEPROM_WIFI_SSID_OFFSET = 148;		//32 chars
    const uint8_t EEPROM_WIFI_PASSWORD_OFFSET = 180;	//32 chars
    const uint8_t EEPROM_RANGE_OFFSET = 72;	//uint16_t
    const uint8_t EEPROM_Calvalue_OFFSET = 80;	//uint16_t
    const uint8_t EEPROM_ALARM_OFFSET = 76;	//uint16_t
    const uint8_t EEPROM_LOWALARM_OFFSET = 92;	//uint16_t
    const uint8_t EEPROM_LOGGINGSET_OFFSET = 96;	//uint16_t
    const uint8_t EEPROM_LOGGINGTIME_OFFSET = 200;	//uint16_t


    const uint8_t EEPROM_GAS_AIR_TC_OFFSET = 999;
    const uint8_t EEPROM_GAS_O2_TC_OFFSET = 999;
    const uint8_t EEPROM_GAS_N2_TC_OFFSET = 999;
    const uint8_t EEPROM_GAS_He_TC_OFFSET = 999;
    const uint8_t EEPROM_GAS_H2_TC_OFFSET = 999;
    const uint8_t EEPROM_GAS_ArCH4_TC_OFFSET = 999;



    bool m_loadAllInProgress = false;

	std::vector<ParamChangeListener*> m_paramChangeListeners;

	String m_wifiSsid;
	String m_wifiPassword;
	String m_deviceId;
	String m_wifiRtLogFreq;
	String m_flashLogFreq;
	String m_mqttServerUrl;

public:

	ConfigurationManager();
	
	void init();
	
	void loadFromEEPROM();

	void saveTimerIntervalToEEPROM(int interval, bool doCommit = true);

    void saveRangeToEEPROM(int range, bool doCommit = true);
    void saveAlarmToEEPROM(int alarm, bool doCommit = true);
    void saveLowalarmToEEPROM(int lowalarm, bool doCommit = true);

    void saveCalvalueToEEPROM(int calvalue, bool doCommit = true);

    void saveGasSelectedToEEPROM(int gasIndex, bool doCommit = true);

	void saveInterceptToEEPROM(double intercept, bool doCommit = true);
    void saveSlopeToEEPROM(double slope, bool doCommit = true);
    void saveGasThermalConductivity(String gasName, double tc, bool doCommit = true);
	void saveWifiSSID(String ssid, bool doCommit = true);
	void saveWifiPassword(String password, bool doCommit = true);

	void saveDeviceId(String id, bool doCommit = true);

	// Getters
	String getWifiSsid() const { return m_wifiSsid; }
	String getWifiPassword() const { return m_wifiPassword; }
	String getDeviceId() const { return m_deviceId; }



	void setupEEPROM();
	
	void clearEEPROM();

	void onParamChange(String param, String value);


	void addParamChangeListener(ParamChangeListener* l);
	void notifyParamChanged(String param, String value);

};
