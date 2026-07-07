#include "inc/DataLogger.h"
#include "inc/DataSource.h"
#include "inc/GasManager.h"
#include "inc/LoggingSet.h"
#include "inc/LoggingTime.h"
#include <EEPROM.h>

void DataLogger::init(DataSource* dataSource, GasManager* gasManager)
{
  m_dataSource = dataSource;
  m_gasManager = gasManager;

  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
}

void DataLogger::setLoggingConfig(Loggingset* loggingset, Loggingtime* loggingtime)
{
  m_loggingset = loggingset;
  m_loggingtime = loggingtime;
}

// Append one CSV line ("timestamp,TAG,ppm") to /log.txt. The file is retrieved
// over Bluetooth/Serial with the PID73* command (see listenForCode()).
bool DataLogger::appendLogEntry(const char* tag)
{
  if (!m_dataSource) return false;
  fs::File f = SPIFFS.open("/log.txt", FILE_APPEND);
  if (!f) {
    Serial.println("DataLogger: failed to open /log.txt for append");
    return false;
  }
  f.printf("%s,%s,%.1f\n", getCurrentDateTimeStr().c_str(), tag,
           m_dataSource->getDoubleValue());
  f.close();
  return true;
}

void DataLogger::handleTick()
{
  // Auto mode: log at the configured interval while on any screen.
  if (!m_loggingset || m_loggingset->getSelectedLoggingset() != 1) return;

  static const int c_intervalSeconds[] = { 1, 10, 60, 600, 3600 };
  int idx = m_loggingtime ? m_loggingtime->getSelectedLoggingtime() : 0;
  if (idx < 0 || idx > 4) idx = 0;
  m_secondsBetweenDataPoints = c_intervalSeconds[idx];

  if (shouldExecTick())
    appendLogEntry("AUTO");
}

bool DataLogger::shouldExecTick()
{
    unsigned long now = millis();
    if (now - m_lastTickMillis >= m_secondsBetweenDataPoints * 1000) {
        m_lastTickMillis = now;
        return true;
    }
    return false;
}

String DataLogger::getCurrentDateTimeStr() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "00000000_000000";
    }
    char str[20];
    strftime(str, 20, "%Y%m%d_%H%M%S", &timeinfo);
    return String(str);
}

// REMOVED: MQTT and WiFi Dump implementations
