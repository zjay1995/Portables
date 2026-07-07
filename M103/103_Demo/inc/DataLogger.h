#pragma once

#include <FS.h>
#include <SPIFFS.h>
#include <time.h>
#include <sys/time.h>

class GasManager;
class DataSource;
class Loggingset;
class Loggingtime;

class DataLogger
{
    int m_secondsBetweenDataPoints = 1;

    unsigned long m_lastTickMillis = 0;

    DataSource* m_dataSource;

    GasManager* m_gasManager;

    Loggingset*  m_loggingset  = nullptr;
    Loggingtime* m_loggingtime = nullptr;

    String getCurrentDateTimeStr();
    bool shouldExecTick();
    bool appendLogEntry(const char* tag);

public:

    DataLogger()=default;
    ~DataLogger()=default;

    void init(DataSource* dataSource, GasManager* gasManager);
    void setLoggingConfig(Loggingset* loggingset, Loggingtime* loggingtime);
    void handleTick();
};
