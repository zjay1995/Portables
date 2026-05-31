#pragma once

#include <TFT_eSPI.h>

class Menu;
class GasManager;
class DataSource;
class SleepTimer;
class Range;
class Alarm;
class Lowalarm;
class Calvalue;
class Hour;
class Minute;
class Outport;
class Loggingset;
class Loggingtime;
class DataLogger;
class TimeSync;

class MenuRenderer {
public:
    virtual void render(Menu* menu) = 0;
};

// --- Base Class for TFT ---
class TFTMenuRenderer : public MenuRenderer {
protected:
    TFT_eSPI* m_display;
public:
    TFTMenuRenderer(TFT_eSPI* display) : m_display(display) {}
    virtual void render(Menu* menu) = 0;
};

// --- Derived Classes ---

class TFTGasMenuRenderer : public TFTMenuRenderer {
public:
    TFTGasMenuRenderer(TFT_eSPI* display) : TFTMenuRenderer(display) {}
    void render(Menu* menu);
};

class TFTRunMenuRenderer : public TFTMenuRenderer {
    DataSource* m_dataSource;
    GasManager* m_gasManager;
    Range* m_range;
    Alarm* m_alarm;
    Lowalarm* m_lowalarm;
    Calvalue* m_calvalue;
    Outport* m_outport;
    Loggingset* m_loggingset;
    Loggingtime* m_loggingtime;
public:
    TFTRunMenuRenderer(TFT_eSPI* display, DataSource* dataSource, GasManager* gasManager, Alarm* alarm, Lowalarm* lowalarm, Range* range, Calvalue* calvalue, Outport* outport, Loggingset* loggingset, Loggingtime* loggingtime)
            : TFTMenuRenderer(display), m_dataSource(dataSource), m_gasManager(gasManager), m_range(range), m_alarm(alarm), m_lowalarm(lowalarm), m_calvalue(calvalue), m_outport(outport), m_loggingset(loggingset), m_loggingtime(loggingtime) {}
    void render(Menu* menu);
};

class TFTSleepTimerMenuRenderer : public TFTMenuRenderer {
    SleepTimer* m_sleepTimer;
public:
    TFTSleepTimerMenuRenderer(TFT_eSPI* display, SleepTimer* sleepTimer) : TFTMenuRenderer(display), m_sleepTimer(sleepTimer) {}
    void render(Menu* menu);
};

class TFTRangeMenuRenderer : public TFTMenuRenderer {
    Range* m_range;
public:
    TFTRangeMenuRenderer(TFT_eSPI* display, Range* range) : TFTMenuRenderer(display), m_range(range) {}
    void render(Menu* menu);
};

class TFTAlarmMenuRenderer : public TFTMenuRenderer {
    Alarm* m_alarm;
public:
    TFTAlarmMenuRenderer(TFT_eSPI* display, Alarm* alarm) : TFTMenuRenderer(display), m_alarm(alarm) {}
    void render(Menu* menu);
};

class TFTLowalarmMenuRenderer : public TFTMenuRenderer {
    Lowalarm* m_lowalarm;
public:
    TFTLowalarmMenuRenderer(TFT_eSPI* display, Lowalarm* lowalarm) : TFTMenuRenderer(display), m_lowalarm(lowalarm) {}
    void render(Menu* menu);
};

class TFTHourMenuRenderer : public TFTMenuRenderer {
    Hour* m_hour;
public:
    TFTHourMenuRenderer(TFT_eSPI* display, Hour* hour) : TFTMenuRenderer(display), m_hour(hour) {}
    void render(Menu* menu);
};

class TFTMinuteMenuRenderer : public TFTMenuRenderer {
    Minute* m_minute;
public:
    TFTMinuteMenuRenderer(TFT_eSPI* display, Minute* minute) : TFTMenuRenderer(display), m_minute(minute) {}
    void render(Menu* menu);
};

class TFTCalvalueMenuRenderer : public TFTMenuRenderer {
    Calvalue* m_calvalue;
public:
    TFTCalvalueMenuRenderer(TFT_eSPI* display, Calvalue* calvalue) : TFTMenuRenderer(display), m_calvalue(calvalue) {}
    void render(Menu* menu);
};

class TFTOutportMenuRenderer : public TFTMenuRenderer {
    Outport* m_outport;
public:
    TFTOutportMenuRenderer(TFT_eSPI* display, Outport* outport) : TFTMenuRenderer(display), m_outport(outport) {}
    void render(Menu* menu);
};

class TFTLoggingsetMenuRenderer : public TFTMenuRenderer {
    Loggingset* m_loggingset;
public:
    TFTLoggingsetMenuRenderer(TFT_eSPI* display, Loggingset* loggingset) : TFTMenuRenderer(display), m_loggingset(loggingset) {}
    void render(Menu* menu);
};

class TFTLoggingtimeMenuRenderer : public TFTMenuRenderer {
    Loggingtime* m_loggingtime;
public:
    TFTLoggingtimeMenuRenderer(TFT_eSPI* display, Loggingtime* loggingtime) : TFTMenuRenderer(display), m_loggingtime(loggingtime) {}
    void render(Menu* menu);
};

class TFTFlashLoggerMenuRenderer : public TFTMenuRenderer {
    DataLogger* m_dataLogger;
public:
    TFTFlashLoggerMenuRenderer(TFT_eSPI* display, DataLogger* dataLogger) : TFTMenuRenderer(display), m_dataLogger(dataLogger) {}
    void render(Menu* menu);
};

class TFTShowTimeMenuRenderer : public TFTMenuRenderer {
public:
    TFTShowTimeMenuRenderer(TFT_eSPI* display) : TFTMenuRenderer(display) {}
    void render(Menu* menu);
};

class TFTCalStartMenuRenderer : public TFTMenuRenderer {
public:
    TFTCalStartMenuRenderer(TFT_eSPI* display) : TFTMenuRenderer(display) {}
    void render(Menu* menu);
};

class TFTZEROMenuRenderer : public TFTMenuRenderer {
    DataSource* m_dataSource;
    GasManager* m_gasManager;
public:
    TFTZEROMenuRenderer(TFT_eSPI* display, DataSource* dataSource, GasManager* gasManager) : TFTMenuRenderer(display), m_dataSource(dataSource), m_gasManager(gasManager) {}
    void render(Menu* menu);
};

class TFTCalGasMenuRenderer : public TFTMenuRenderer {
    DataSource* m_dataSource;
    Calvalue* m_calvalue;
    GasManager* m_gasManager;
public:
    TFTCalGasMenuRenderer(TFT_eSPI* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : TFTMenuRenderer(display), m_dataSource(dataSource), m_calvalue(calvalue), m_gasManager(gasManager) {}
    void render(Menu* menu);
};

class TFTCalResMenuRenderer : public TFTMenuRenderer {
    DataSource* m_dataSource;
    Calvalue* m_calvalue;
    GasManager* m_gasManager;
public:
    TFTCalResMenuRenderer(TFT_eSPI* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : TFTMenuRenderer(display), m_dataSource(dataSource), m_calvalue(calvalue), m_gasManager(gasManager) {}
    void render(Menu* menu);
};