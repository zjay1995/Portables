#include "inc/DataLogger.h"
#include "inc/DataSource.h"
#include "inc/GasManager.h"
#include <EEPROM.h>

portMUX_TYPE DataLogger::s_mtxLogger = portMUX_INITIALIZER_UNLOCKED;

void DataLogger::init(DataSource* dataSource, GasManager* gasManager)
{
  m_dataSource = dataSource;
  m_gasManager = gasManager;

  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  checkIfGracefulShutdown();
}

void DataLogger::checkIfGracefulShutdown()
{
  // Logic to check if previous session ended correctly (simplified)
}

bool DataLogger::startFlashStoreSession()
{
  if(m_sessionRunning) return false;

  m_sessionRunning = true;
  m_fileIndex = 0;
  m_currentSessionTimestamp = getCurrentDateTimeStr();
  // Create logic here if needed
  return true;
}

bool DataLogger::stopFlashStoreSession()
{
  if(!m_sessionRunning) return false;
  m_sessionRunning = false;
  if(m_file) m_file.close();
  return true;
}

bool DataLogger::isFlashStoreSessionRunning() const
{
  return m_sessionRunning;
}

void DataLogger::handleTick()
{
  if(m_sessionRunning && shouldExecTick()) {
        // Simple log logic
        if(!m_file) {
             // Open file logic if needed
        }
        // Write data...
    }
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
