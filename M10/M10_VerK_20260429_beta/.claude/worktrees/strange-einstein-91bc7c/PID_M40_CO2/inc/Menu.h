#pragma once

#include "GasManager.h"
#include "MenuRenderer.h"
#include "SleepTimer.h"
#include "RangeSet.h"
#include "AlarmSet.h"
#include "LowAlarmSet.h"

#include "HourSet.h"
#include "MinuteSet.h"

#include "CalvalueSet.h"
#include "OutputSet.h"
#include "LoggingSet.h"
#include "LoggingTime.h"
#include "DataLogger.h"
#include "TimeSync.h"
#include "DataSource.h"


class Menu
{
protected:

    String m_menuName;
    String m_parentMenuName;

    MenuRenderer* m_menuRenderer;

public:

    Menu(String name, String parentName, MenuRenderer* renderer) : m_menuName(name),
                                                                   m_parentMenuName(parentName),
                                                                   m_menuRenderer(renderer)
    {

    }

    virtual ~Menu() = default;


    String getName() { return m_menuName; }
    String getParentName() { return m_parentMenuName; }

    virtual void action() = 0;

    virtual void render()
    {
        m_menuRenderer->render(this);
    }

};

class GasMenuItem : public Menu
{
    GasManager* m_gasManager;
    int m_gasIndex;

public:

    GasMenuItem(String name, String parentName, int gasIndex, GasManager* gasManager, MenuRenderer* renderer) : Menu(name, parentName, renderer),
                                                                                                                m_gasManager(gasManager),
                                                                                                                m_gasIndex(gasIndex)
    {

    }

    void action()
    {
        m_gasManager->selectGasByIndex(m_gasIndex);
    }

};

class RunMenuItem : public Menu
{
    GasManager* m_gasManager;
    DataSource* m_dataSource;  // DataSource for sensor readings
    Loggingset* m_loggingset;  // Logging configuration
    Range* m_range;            // Range object
    Outport* m_outport;        // Outport object

public:
    // Constructor with DataSource and Loggingset
    RunMenuItem(String name, String parentName, int menuIndex, GasManager* gasManager, DataSource* dataSource,
                Loggingset* loggingset, Range* range, Outport* outport, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_gasManager(gasManager),
              m_dataSource(dataSource),
              m_loggingset(loggingset),
              m_range(range),
              m_outport(outport)
    {

    }

    void action() override
    {
        // Check if the logging mode is Manual (loggingset == 2)
        if (m_loggingset->getSelectedLoggingset() == 2) {
            // Get the current log count from EEPROM
            int logCount = EEPROM.readUShort(210);

// Manually log data when the "S" button is pressed
            double currentVoltage = m_dataSource->getRawMiliVolts();
            double sensorValue = m_dataSource->getDoubleValue();  // For I2C; for UART, use getCalibratedValue()

// Retrieve range and outport values
            int rangeValue = m_range->getSelectedRange();  // Assuming `getSelectedRange()` exists
            int outportValue = m_outport->getSelectedOutport();  // Assuming `getSelectedOutport()` exists

// Create the log entry
            String logData = "Log #" + String(logCount) + ": " +
                             " " + String(sensorValue, 1) + " ppm, " +
                             " " + String(currentVoltage, 1) + " mV, " +
                             "Range: " + rangeValue + ", " +
                             "UserID: " + outportValue + "\n";

// Save to SPIFFS
            if (!SPIFFS.begin(true)) {
                Serial.println("An Error has occurred while mounting SPIFFS");
                return;
            }

// Open the log file for reading
            File logFile = SPIFFS.open("/log.txt", FILE_READ);
            if (!logFile) {
                Serial.println("Failed to open file for reading");
                return;
            }

// Read existing entries into a vector
            std::vector<String> logEntries;
            while (logFile.available()) {
                logEntries.push_back(logFile.readStringUntil('\n') + "\n");  // Ensure newline character is retained
            }
            logFile.close();

// Add the new entry to the end
            logEntries.push_back(logData);

// Ensure only the last 200 entries are kept
            if (logEntries.size() > 200) {
                logEntries.erase(logEntries.begin(), logEntries.begin() + (logEntries.size() - 200));
            }

// Write the updated entries back to the file
            logFile = SPIFFS.open("/log.txt", FILE_WRITE);  // Open file in write mode to overwrite
            if (!logFile) {
                Serial.println("Failed to open file for writing");
                return;
            }
            for (const String& entry : logEntries) {
                logFile.print(entry);
            }
            logFile.close();

// Increment the log count
            logCount++;

// Save the updated log count to EEPROM
            EEPROM.writeUShort(210, logCount);
            EEPROM.commit();  // Make sure data is written to EEPROM

// Display log info to Serial for debugging
            Serial.print("Log saved: ");
            Serial.print(logData);
            Serial.println("Log count saved to EEPROM: " + String(logCount));

        }  else {
            Serial.println("Logging not enabled in Manual mode.");
        }
    }
};

class SleepTimerMenuItem : public Menu
{
    SleepTimer* m_sleepTimer;

    int m_intervalIndex;

public:

    SleepTimerMenuItem(String name, String parentName, int intervalIndex, SleepTimer* sleepTimer, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_sleepTimer(sleepTimer),
              m_intervalIndex(intervalIndex)
    {

    }

    void action()
    {
        m_sleepTimer->selectIntervalByIndex(m_intervalIndex);
    }
};



class RangeMenuItem : public Menu
{
    Range* m_range;

    int m_rangeIndex;

public:

    RangeMenuItem(String name, String parentName, int rangeIndex, Range* range, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_range(range),
              m_rangeIndex(rangeIndex)
    {

    }

    void action()
    {
        m_range->selectRangeByIndex(m_rangeIndex);
    }

    void render()
    {
        m_menuRenderer->render(this);
    }

};

class AlarmMenuItem : public Menu
{
    Alarm* m_alarm;

    int m_alarmIndex;

public:

    AlarmMenuItem(String name, String parentName, int alarmIndex, Alarm* alarm, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_alarm(alarm),
              m_alarmIndex(alarmIndex)
    {

    }

    void action()
    {
        m_alarm->selectAlarmByIndex(m_alarmIndex);
    }
};

class LowalarmMenuItem : public Menu
{
    Lowalarm* m_lowalarm;

    int m_lowalarmIndex;

public:

    LowalarmMenuItem(String name, String parentName, int lowalarmIndex, Lowalarm* lowalarm, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_lowalarm(lowalarm),
              m_lowalarmIndex(lowalarmIndex)
    {

    }

    void action()
    {
        m_lowalarm->selectLowalarmByIndex(m_lowalarmIndex);
    }
};


class HourMenuItem : public Menu
{
    Hour* m_hour;

    int m_hourIndex;

public:

    HourMenuItem(String name, String parentName, int hourIndex, Hour* hour, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_hour(hour),
              m_hourIndex(hourIndex)
    {

    }

    void action()
    {
        m_hour->selectHourByIndex(m_hourIndex);
    }
};


class MinuteMenuItem : public Menu
{
    Minute* m_minute;

    int m_minuteIndex;

public:

    MinuteMenuItem(String name, String parentName, int minuteIndex, Minute* minute, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_minute(minute),
              m_minuteIndex(minuteIndex)
    {

    }

    void action()
    {
        m_minute->selectMinuteByIndex(m_minuteIndex);
    }
};


class CalvalueMenuItem : public Menu
{
    Calvalue* m_calvalue  ;

    int m_calvalueIndex;

public:

    CalvalueMenuItem(String name, String parentName, int calvalueIndex, Calvalue* calvalue, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_calvalue(calvalue),
              m_calvalueIndex(calvalueIndex)
    {

    }

    void action()
    {
        m_calvalue->selectCalvalueByIndex(m_calvalueIndex);
    }

};


class OutportMenuItem : public Menu
{
    Outport* m_outport  ;

    int m_outportIndex;

public:

    OutportMenuItem(String name, String parentName, int outportIndex, Outport* outport, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_outport(outport),
              m_outportIndex(outportIndex)
    {

    }

    void action()
    {
        m_outport->selectOutportByIndex(m_outportIndex);
    }

};

class LoggingsetMenuItem : public Menu
{
    Loggingset* m_loggingset  ;

    int m_loggingsetIndex;

public:

    LoggingsetMenuItem(String name, String parentName, int loggingsetIndex, Loggingset* loggingset, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_loggingset(loggingset),
              m_loggingsetIndex(loggingsetIndex)
    {

    }

    void action() {

        if (m_loggingsetIndex == 3) {
            // Open the log file in write mode, which will truncate (clear) it
            File logFile = SPIFFS.open("/log.txt", FILE_WRITE);
            if (!logFile) {
                Serial.println("Failed to open log.txt for clearing");
                return;
            }

            // Write an empty string to clear the file
            logFile.print("");
            logFile.close();

            Serial.println("log.txt has been cleared");
        }

        m_loggingset->selectLoggingsetByIndex(m_loggingsetIndex);
    }
};



//Logging Time
class LoggingtimeMenuItem : public Menu
{
    Loggingtime* m_loggingtime  ;

    int m_loggingtimeIndex;

public:

    LoggingtimeMenuItem(String name, String parentName, int loggingtimeIndex, Loggingtime* loggingtime, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_loggingtime(loggingtime),
              m_loggingtimeIndex(loggingtimeIndex)
    {

    }

    void action()
    {
        m_loggingtime->selectLoggingtimeByIndex(m_loggingtimeIndex);
    }

};




class DataLoggerFlashStoreMenuItem : public Menu
{
    DataLogger* m_dataLogger;

public:

    DataLoggerFlashStoreMenuItem(String name, String parentName, DataLogger* dataLogger, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_dataLogger(dataLogger)
    {

    }

    void action()
    {
        if( m_dataLogger->isFlashStoreSessionRunning() )
            m_dataLogger->stopFlashStoreSession();
        else
            m_dataLogger->startFlashStoreSession();
    }

};

class WiFiDumpMenuItem : public Menu
{
    DataLogger* m_dataLogger;

public:

    WiFiDumpMenuItem(String name, String parentName, DataLogger* dataLogger, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_dataLogger(dataLogger)
    {

    }

    void action()
    {
        if (m_dataLogger->isWiFiDumpRunning())
            m_dataLogger->stopWiFiDumpSession();
        else
            m_dataLogger->startWiFiDumpSession();
    }

};

class WiFiRealTimeDumpMenuItem : public Menu
{
    DataLogger* m_dataLogger;

public:

    WiFiRealTimeDumpMenuItem(String name, String parentName, DataLogger* dataLogger, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_dataLogger(dataLogger)
    {

    }

    void action()
    {
        if (m_dataLogger->isWiFiRealTimeDumpRunning())
            m_dataLogger->stopWiFiRealTimeDumpSession();
        else
            m_dataLogger->startWiFiRealTimeDumpSession();
    }

};

class NTPSyncMenuItem : public Menu
{

    TimeSync* m_timeSync;

public:

    NTPSyncMenuItem(String name, String parentName, TimeSync* timeSync, MenuRenderer* renderer)
            : Menu(name, parentName, renderer)
    {
        m_timeSync = timeSync;
    }

    void action()
    {
        if(!m_timeSync->isNTCSyncRunning())
            m_timeSync->startNTPSync();
        else
        {
            m_timeSync->stopNTPSync();
        }

    }

};




class InfoMenuItem : public Menu
{

    //TimeSync* m_timeSync;

public:

    InfoMenuItem(String name, String parentName, MenuRenderer* renderer)
            : Menu(name, parentName, renderer)
    {
        //m_timeSync = timeSync;
    }

    void action()
    {


    }

};





class CalMenuItemZero : public Menu
{

    DataSource* m_dataSource;
    GasManager* m_gasManager;

public:

    CalMenuItemZero(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_gasManager(gasManager),
              m_dataSource(dataSource)
    {

    }

    void action()
    {
        m_gasManager->calibrate(m_dataSource->getRawMiliVolts());
    }

};

class CalMenuItemCalGas : public Menu
{

    DataSource* m_dataSource;
    GasManager* m_gasManager;

public:

    CalMenuItemCalGas(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_gasManager(gasManager),
              m_dataSource(dataSource)
    {

    }

    void action()
    {
        m_gasManager->calibrate2(m_dataSource->getRawMiliVolts());
    }

};

class CalMenuItemCalRes : public Menu
{

    DataSource* m_dataSource;
    GasManager* m_gasManager;

public:

    CalMenuItemCalRes(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer)
            : Menu(name, parentName, renderer),
              m_gasManager(gasManager),
              m_dataSource(dataSource)
    {

    }

    void action()
    {

    }

};





class ShowTimeMenuItem : public Menu
{

public:

    ShowTimeMenuItem(String name, String parentName, MenuRenderer* renderer)
            : Menu(name, parentName, renderer)
    {

    }

    void action()
    {

    }

};

class CompositeMenu : public Menu
{
    std::vector<Menu*> m_menus;
    int m_currentIndex;

public:

    CompositeMenu(String name, String parentName, std::vector<Menu*> menus) : Menu(name, parentName, nullptr),
                                                                              m_menus(menus),
                                                                              m_currentIndex(0)
    {

    }

    Menu* getCurrentMenu()
    {
        return m_menus[ m_currentIndex ];
    }

    int getCurrentIndex() // Added method to get the current index
    {
        return m_currentIndex;
    }

    void moveToNext()
    {
        m_currentIndex = (m_currentIndex + 1) % m_menus.size();
        if(m_currentIndex == 5 & m_menuName == "Main Menu"){
            m_currentIndex = 0;
        }

        if(m_currentIndex == 6 & m_menuName == "Main Menu"){
            m_currentIndex = 7;
        }
        if(m_currentIndex == 8 & m_menuName == "Main Menu"){
            m_currentIndex = 9;
        }
        if(m_currentIndex == 10 & m_menuName == "Main Menu"){
            m_currentIndex = 11;
        }
        if(m_currentIndex == 12 & m_menuName == "Main Menu"){
            m_currentIndex = 0;
        }

        Serial.println(m_parentMenuName);
        Serial.println("moveToNext" + String(m_currentIndex) + " " + String(m_menus.size()) );
        Serial.println("moveToNext: " + m_menuName );
        Serial.flush();
    }

    void setCurrentMenu(int menuIndex) {
        m_currentIndex = menuIndex;
        Serial.println("moveToNext" + String(m_currentIndex) + " ");


        // Optional: Handle the case where the menuName is not found
        Serial.println("Menu not found: " + menuIndex);
    }

    void action() override
    {
        m_menus[m_currentIndex]->action();
        Serial.println(m_currentIndex);
        Serial.println(m_menuName);
        if(m_currentIndex != 0) {
            m_currentIndex = (m_currentIndex + 1) % m_menus.size();
        }
        if(m_menuName == "LIBRARY") {
            m_currentIndex = (m_currentIndex - 1) % m_menus.size();
        }else if(m_menuName == "Alarm"){
            m_currentIndex = (m_currentIndex - 1) % m_menus.size();
        }else if(m_menuName == "Low Alarm"){
            m_currentIndex = (m_currentIndex - 1) % m_menus.size();
        }else if(m_menuName == "Range") {
            m_currentIndex = (m_currentIndex - 1) % m_menus.size();
        }else if(m_menuName == "Set Hour") {
            m_currentIndex = 0;
        }else if(m_menuName == "Set Minute") {
            m_currentIndex = 0;
        }else if(m_menuName == "Calvalue") {
            m_currentIndex = (m_currentIndex - 1) % m_menus.size();
        }

        if(m_currentIndex == 5 & m_menuName == "Main Menu"){
            m_currentIndex = 0;
        }
        if(m_currentIndex == 11 & m_menuName == "Main Menu"){
            m_currentIndex = 0;
        }
        Serial.println("moveToNext" + String(m_currentIndex) + " " + String(m_menus.size()) );
        Serial.println("moveToNext: " + m_menuName );
        Serial.flush();
    }

    void render() override
    {

        m_menus[ m_currentIndex ]->render();
    }

    void print()
    {
        Serial.println("Menu name: " + m_menuName + " Parent name: " + m_parentMenuName );
        for(Menu* m : m_menus)
            Serial.println("Menu name: " + m->getName() + " Parent name: " + m->getParentName() );
        Serial.flush();
    }

};