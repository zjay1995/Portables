#pragma once
#include <FS.h>
#include "GasManager.h"
#include "UnitSet.h"
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

class Menu {
protected:
    String m_menuName;
    String m_parentMenuName;
    MenuRenderer* m_menuRenderer;
public:
    Menu(String name, String parentName, MenuRenderer* renderer) : m_menuName(name), m_parentMenuName(parentName), m_menuRenderer(renderer) {}
    virtual ~Menu() = default;
    String getName() { return m_menuName; }
    String getParentName() { return m_parentMenuName; }
    virtual void action() = 0;
    virtual void render() { m_menuRenderer->render(this); }
};

// ... [Keep GasMenuItem, RunMenuItem, SleepTimerMenuItem, RangeMenuItem, AlarmMenuItem, LowalarmMenuItem, HourMenuItem, MinuteMenuItem, CalvalueMenuItem, OutportMenuItem] ...
// (These standard menus are safe to keep as they were)

class GasMenuItem : public Menu {
    GasManager* m_gasManager;
    int m_gasIndex;
public:
    GasMenuItem(String name, String parentName, int gasIndex, GasManager* gasManager, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_gasManager(gasManager), m_gasIndex(gasIndex) {}
    void action() { m_gasManager->selectGasByIndex(m_gasIndex); }
};

class RunMenuItem : public Menu {
    GasManager* m_gasManager;
    DataSource* m_dataSource;
    Loggingset* m_loggingset;
    Range* m_range;
    Outport* m_outport;
public:
    RunMenuItem(String name, String parentName, int menuIndex, GasManager* gasManager, DataSource* dataSource, Loggingset* loggingset, Range* range, Outport* outport, MenuRenderer* renderer)
            : Menu(name, parentName, renderer), m_gasManager(gasManager), m_dataSource(dataSource), m_loggingset(loggingset), m_range(range), m_outport(outport) {}
    void action() override {
        // Manual logging mode: S on the run screen appends one entry to
        // /log.txt and bumps the counter shown in the footer ("LOG: M n").
        if (m_loggingset->getSelectedLoggingset() != 2) return;

        fs::File logFile = SPIFFS.open("/log.txt", FILE_APPEND);
        if (!logFile) {
            Serial.println("Manual log: failed to open /log.txt");
            return;
        }
        struct tm timeinfo;
        char ts[20] = "00000000_000000";
        if (getLocalTime(&timeinfo, 10))
            strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &timeinfo);
        logFile.printf("%s,MANUAL,%.1f\n", ts, m_dataSource->getDoubleValue());
        logFile.close();

        uint16_t count = EEPROM.readUShort(210);
        if (count == 0xFFFF) count = 0;  // blank EEPROM
        EEPROM.writeUShort(210, count + 1);
        EEPROM.commit();
    }
};

class SleepTimerMenuItem : public Menu {
    SleepTimer* m_sleepTimer;
    int m_intervalIndex;
public:
    SleepTimerMenuItem(String name, String parentName, int intervalIndex, SleepTimer* sleepTimer, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_sleepTimer(sleepTimer), m_intervalIndex(intervalIndex) {}
    void action() { m_sleepTimer->selectIntervalByIndex(m_intervalIndex); }
};

class RangeMenuItem : public Menu {
    Range* m_range;
    int m_rangeIndex;
public:
    RangeMenuItem(String name, String parentName, int rangeIndex, Range* range, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_range(range), m_rangeIndex(rangeIndex) {}
    void action() { m_range->selectRangeByIndex(m_rangeIndex); }
};

class AlarmMenuItem : public Menu {
    Alarm* m_alarm;
    int m_alarmIndex;
public:
    AlarmMenuItem(String name, String parentName, int alarmIndex, Alarm* alarm, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_alarm(alarm), m_alarmIndex(alarmIndex) {}
    void action() { m_alarm->selectAlarmByIndex(m_alarmIndex); }
};

class LowalarmMenuItem : public Menu {
    Lowalarm* m_lowalarm;
    int m_lowalarmIndex;
public:
    LowalarmMenuItem(String name, String parentName, int lowalarmIndex, Lowalarm* lowalarm, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_lowalarm(lowalarm), m_lowalarmIndex(lowalarmIndex) {}
    void action() { m_lowalarm->selectLowalarmByIndex(m_lowalarmIndex); }
};

class HourMenuItem : public Menu {
    Hour* m_hour;
    int m_hourIndex;
public:
    HourMenuItem(String name, String parentName, int hourIndex, Hour* hour, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_hour(hour), m_hourIndex(hourIndex) {}
    void action() { m_hour->selectHourByIndex(m_hourIndex); }
};

class MinuteMenuItem : public Menu {
    Minute* m_minute;
    int m_minuteIndex;
public:
    MinuteMenuItem(String name, String parentName, int minuteIndex, Minute* minute, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_minute(minute), m_minuteIndex(minuteIndex) {}
    void action() { m_minute->selectMinuteByIndex(m_minuteIndex); }
};

class CalvalueMenuItem : public Menu {
    Calvalue* m_calvalue;
    int m_calvalueIndex;
public:
    CalvalueMenuItem(String name, String parentName, int calvalueIndex, Calvalue* calvalue, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_calvalue(calvalue), m_calvalueIndex(calvalueIndex) {}
    void action() { m_calvalue->selectCalvalueByIndex(m_calvalueIndex); }
};

class OutportMenuItem : public Menu {
    Outport* m_outport;
    int m_outportIndex;
public:
    OutportMenuItem(String name, String parentName, int outportIndex, Outport* outport, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_outport(outport), m_outportIndex(outportIndex) {}
    void action() { m_outport->selectOutportByIndex(m_outportIndex); }
};

class LoggingsetMenuItem : public Menu {
    Loggingset* m_loggingset;
    int m_loggingsetIndex;
public:
    LoggingsetMenuItem(String name, String parentName, int loggingsetIndex, Loggingset* loggingset, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_loggingset(loggingset), m_loggingsetIndex(loggingsetIndex) {}
    void action() {
        if (m_loggingsetIndex == 3) {
            fs::File logFile = SPIFFS.open("/log.txt", FILE_WRITE);
            if (logFile) { logFile.print(""); logFile.close(); }
            EEPROM.writeUShort(210, 0);  // reset manual log counter
            EEPROM.commit();
        }
        m_loggingset->selectLoggingsetByIndex(m_loggingsetIndex);
    }
};

class LoggingtimeMenuItem : public Menu {
    Loggingtime* m_loggingtime;
    int m_loggingtimeIndex;
public:
    LoggingtimeMenuItem(String name, String parentName, int loggingtimeIndex, Loggingtime* loggingtime, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_loggingtime(loggingtime), m_loggingtimeIndex(loggingtimeIndex) {}
    void action() { m_loggingtime->selectLoggingtimeByIndex(m_loggingtimeIndex); }
};

// REMOVED: WiFiDumpMenuItem, WiFiRealTimeDumpMenuItem, NTPSyncMenuItem,
// DataLoggerFlashStoreMenuItem (flash-session feature deleted from DataLogger)

class ShowTimeMenuItem : public Menu {
public:
    ShowTimeMenuItem(String name, String parentName, MenuRenderer* renderer) : Menu(name, parentName, renderer) {}
    void action() {}
};

class UnitMenuItem : public Menu {
    UnitSet* m_unitset;
    int m_unitIndex;
public:
    UnitMenuItem(String name, String parentName, int unitIndex, UnitSet* unitset, MenuRenderer* renderer)
        : Menu(name, parentName, renderer), m_unitset(unitset), m_unitIndex(unitIndex) {}
    void action() { m_unitset->selectUnitByIndex(m_unitIndex); }
};

class CalMenuItemZero : public Menu {
    DataSource* m_dataSource;
    GasManager* m_gasManager;
public:
    CalMenuItemZero(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_gasManager(gasManager), m_dataSource(dataSource) {}
    void action() { m_gasManager->calibrate(m_dataSource->getRawMiliVolts()); }
};

class CalMenuItemCalGas : public Menu {
    DataSource* m_dataSource;
    GasManager* m_gasManager;
public:
    CalMenuItemCalGas(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_gasManager(gasManager), m_dataSource(dataSource) {}
    void action() { m_gasManager->calibrate2(m_dataSource->getRawMiliVolts()); }
};

class CalMenuItemCalRes : public Menu {
    DataSource* m_dataSource;
    GasManager* m_gasManager;
public:
    CalMenuItemCalRes(String name, DataSource* dataSource, GasManager* gasManager, String parentName, MenuRenderer* renderer) : Menu(name, parentName, renderer), m_gasManager(gasManager), m_dataSource(dataSource) {}
    void action() {}
};

class CompositeMenu : public Menu {
    std::vector<Menu*> m_menus;
    int m_currentIndex;
    int m_navLimit;  // 0 = unlimited (wraps over full list)
public:
    CompositeMenu(String name, String parentName, std::vector<Menu*> menus) : Menu(name, parentName, nullptr), m_menus(menus), m_currentIndex(0), m_navLimit(0) {}
    Menu* getCurrentMenu() { return m_menus[ m_currentIndex ]; }
    int getCurrentIndex() { return m_currentIndex; }
    void setNavLimit(int n) { m_navLimit = n; }
    void moveToNext() {
        int limit = (m_navLimit > 0) ? m_navLimit : (int)m_menus.size();
        m_currentIndex = (m_currentIndex + 1) % limit;
    }
    void setCurrentMenu(int menuIndex) { m_currentIndex = menuIndex; }
    void action() override { m_menus[m_currentIndex]->action(); }
    void render() override { m_menus[ m_currentIndex ]->render(); }
    void print() {
        for(Menu* m : m_menus) Serial.println("Menu: " + m->getName());
    }
};
