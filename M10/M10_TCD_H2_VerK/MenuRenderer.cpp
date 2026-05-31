#include "inc/MenuRenderer.h"
#include "inc/Menu.h"
#include "inc/SleepTimer.h"
#include "inc/DataSource.h"
#include "inc/RangeSet.h"
#include "inc/AlarmSet.h"
#include "inc/LowAlarmSet.h"
#include "inc/HourSet.h"
#include "inc/MinuteSet.h"
#include "inc/CalvalueSet.h"
#include "inc/OutputSet.h"
#include "inc/LoggingSet.h"
#include "inc/LoggingTime.h"

#include <Adafruit_ADS1015.h>
#include "SSD1306.h"
#include <Arduino.h>
#include <U8g2lib.h>



SSD1306GasMenuRenderer::SSD1306GasMenuRenderer(SSD1306Wire* display) : SSD1306MenuRenderer(display)

{


}

void SSD1306GasMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Gas Selection");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->setFont(ArialMT_Plain_10);
  m_display->display();
}


SSD1306RunMenuRenderer::SSD1306RunMenuRenderer(SSD1306Wire* display, DataSource* dataSource, GasManager* gasManager, Alarm* alarm, Lowalarm* lowalarm, Range* range, Calvalue* calvalue, Outport* outport, Loggingset* loggingset, Loggingtime* loggingtime)
  : SSD1306MenuRenderer(display),
    m_dataSource(dataSource),
    m_gasManager(gasManager),
    m_alarm(alarm),
    m_lowalarm(lowalarm),
    m_range(range),
    m_calvalue(calvalue),
    m_outport(outport),
    m_loggingset(loggingset),
    m_loggingtime(loggingtime)
{
}
void SSD1306RunMenuRenderer::render(Menu* menu)
{
  digitalWrite(ALARM_PIN, LOW);  // Turn the alarm off

  const float multiplier = 0.125F; // GAIN 1
  float range    = m_range->getSelectedRange();
  float alarm    = m_alarm->getSelectedAlarm();
  float lowalarm = m_lowalarm->getSelectedLowalarm();
  int   outport  = m_outport->getSelectedOutport();
  float calvalue = m_calvalue->getSelectedCalvalue();
  int64_t startMicros = esp_timer_get_time();
  int v_b = m_dataSource->getRawMiliVolts_battery();
  int loggingset = m_loggingset->getSelectedLoggingset();
  int loggingtime = m_loggingtime->getSelectedLoggingtime();
  Gas& selectedGas = m_gasManager->getSelectedGas();
  int RH = m_dataSource->getHumidity();
  int T = m_dataSource->getTemperature();
  int userID = m_outport->getSelectedOutport();

  // Static variables to track alarm duration
  static unsigned long alarmStartTime = 0;
  static bool wasAlarmActive = false;

  // Map logging intervals (in milliseconds)
  unsigned long loggingIntervals[] = {
    1000,       // 1 sec
    10000,      // 10 sec
    60000,      // 1 minute
    600000,     // 10 minutes
    3600000     // 1 hour
  };

  // Define the connection type: 0 for I2C, 1 for UART
  int connectionType = 0; // 0 for I2C, 1 for UART
  int n_EC = 1; // Number of sensors (this can be adjusted)

  static unsigned long lastLogTime = 0;  // Store the last time a log was written
  unsigned long currentMillis = millis();  // Get the current time in milliseconds


  static const unsigned char battery_bits[] = {
    0xFE, 0x7F,  // ####### #######
    0x01, 0x80,  // #             #
    0xFD, 0xBF,  // # ####### #####
    0xFD, 0xBF,  // # ####### #####
    0xFD, 0xBF,  // # ####### #####
    0xFD, 0xBF,  // # ####### #####
    0x01, 0x80,  // #             #
    0xFE, 0x7F   // ####### #######
  };

  static bool displayOn = true;  // Toggle for blinking
  static unsigned long lastBlinkTime = 0;
  const unsigned long blinkInterval = 500;  // Blink interval in milliseconds

  // 1EC format
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->setFont(ArialMT_Plain_10);

  // Date & Time
  struct tm timeinfo = {};
  bool timeOK = getLocalTime(&timeinfo, 100);
  char dateString[30] = { 0 };
  char timeString[30] = { 0 };
  if (timeOK) {
    strftime(dateString, 30, "%b %d %y", &timeinfo);
    strftime(timeString, 30, "%H:%M", &timeinfo);
  } else {
    strcpy(dateString, "----");
    strcpy(timeString, "--:--");
  }
  m_display->drawString(0, 0, String(timeString));
  float voltage = v_b / 1000.0;
  int rawPercent = (int)round((voltage - 3.4) / (4.05 - 3.4) * 100);
  rawPercent = constrain(rawPercent, 0, 100);

  // 16-sample rolling average — smooths INA219 noise on a steady battery.
  static int battBuf[16];
  static int battIdx = 0;
  static bool battInit = false;
  if (!battInit) {
    for (int i = 0; i < 16; i++) battBuf[i] = rawPercent;
    battInit = true;
  }
  battBuf[battIdx] = rawPercent;
  battIdx = (battIdx + 1) % 16;
  int battSum = 0;
  for (int i = 0; i < 16; i++) battSum += battBuf[i];
  int avgPct = battSum / 16;                     // averaged, 1 % resolution

  // Hysteresis on top of the 5 % quantization: averaging alone still flips at
  // the round-to-5 boundary (e.g. 87.4 vs 87.6). Only switch buckets when the
  // averaged value moves at least 3 % past the currently-displayed bucket,
  // giving a 1 % dead-band that fully eliminates adjacent-step blinking.
  static int displayedPct = -1;
  if (displayedPct < 0) {
    displayedPct = (int)round(avgPct / 5.0) * 5;
  } else if (avgPct >= displayedPct + 3) {
    displayedPct = (int)round(avgPct / 5.0) * 5;
  } else if (avgPct <= displayedPct - 3) {
    displayedPct = (int)round(avgPct / 5.0) * 5;
  }
  int percent = constrain(displayedPct, 0, 100);

  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 0, "B " + String(percent) + "%");

  // Display sensor data based on connection type
  double sensorValue = 0;
  if (connectionType == 0) {
    sensorValue = m_dataSource->getDoubleValue();  // I2C
  } else {
    // sensorValue = m_dataSource->getCalibratedValue(0);  // UART (if needed)
  }

  // Sensor type header (center top)
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "TCD");
  m_display->drawLine(0, 14, 128, 14);
  m_display->drawString(113, 33, "ppm");  // Unit for H2 concentration
  // Display the current log number if in manual logging mode
  if (loggingset == 2) {
    int logCount = EEPROM.readUShort(210);
    m_display->drawString(105, 51, "M " + String(logCount));  // Manual log count display
  }

  // ---------------------------------------------------------
  // UPDATED ALARM LOGIC (High & Low + 30s Timeout)
  // ---------------------------------------------------------

  // Check triggers
  bool triggerHigh = (alarm != 0 && sensorValue > alarm);
  bool triggerLow  = (lowalarm != 0 && sensorValue > lowalarm);
  bool isAlarmActive = triggerHigh || triggerLow;


  if (isAlarmActive) {
    // Initialize start time if this is a new alarm
    if (!wasAlarmActive) {
      alarmStartTime = millis();
      wasAlarmActive = true;
    }

    // Blink Timer
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime > blinkInterval) {
      displayOn = !displayOn;  // Toggle state
      lastBlinkTime = currentTime;
    }

    m_display->setFont(ArialMT_Plain_24);

    if (displayOn) {
      // PHASE 1: SHOW "ALARM" TEXT
      m_display->drawString(64, 18, "ALARM");

      // Buzzer Logic (Beeps only during this phase, creating a pulse)
      // Stops beeping after 30 seconds
      if (currentTime - alarmStartTime < 30000) {
        digitalWrite(ALARM_PIN, HIGH);
      }
    } else {
      // PHASE 2: SHOW SENSOR READING (The Interchange)
      // This ensures the user sees the value even during an alarm
      m_display->setFont(ArialMT_Plain_10);
      m_display->setTextAlignment(TEXT_ALIGN_LEFT);
      m_display->drawString(2, 25, "H2");
      m_display->setFont(ArialMT_Plain_24);
      m_display->setTextAlignment(TEXT_ALIGN_CENTER);
      if (sensorValue >= 11000.0f) {
        m_display->setFont(ArialMT_Plain_10);
        m_display->drawString(64, 20, "Above Maximum");
        m_display->drawString(64, 32, "Concentration");
      } else if (sensorValue > range) {
        m_display->drawString(64, 18, "OVR");
      } else {
        m_display->drawString(64, 18, String((long)sensorValue));
      }
    }

  } else {
    // NO ALARM - Normal Display
    wasAlarmActive = false;

    m_display->setFont(ArialMT_Plain_10);
    m_display->setTextAlignment(TEXT_ALIGN_LEFT);
    m_display->drawString(2, 25, "H2");
    m_display->setFont(ArialMT_Plain_24);
    m_display->setTextAlignment(TEXT_ALIGN_CENTER);

    if (sensorValue >= 11000.0f) {
      m_display->setFont(ArialMT_Plain_10);
      m_display->drawString(64, 20, "Above Maximum");
      m_display->drawString(64, 32, "Concentration");
    } else if (sensorValue > range) {
      m_display->drawString(64, 18, "OVR");
    } else {
      m_display->drawString(64, 18, String((long)sensorValue));
    }
  }
  // ---------------------------------------------------------

  m_display->setFont(ArialMT_Plain_10);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);   // reset from CENTER used by value display
  // Bottom bar
  m_display->drawLine(0, 49, 128, 49);
  if (alarm != 0 || lowalarm != 0) {
    m_display->drawString(2, 51, "Alarm ON");
  } else {
    m_display->drawString(2, 51, "Alarm OFF");
  }
  m_display->drawString(68, 51, String(m_dataSource->getRawMiliVolts()) + "mV");

  // Automatic logging mode
  if (loggingset == 1)  {
    if (currentMillis - lastLogTime >= loggingIntervals[loggingtime]) {

      struct tm logTimeinfo = {};
      bool logTimeOK = getLocalTime(&logTimeinfo, 100);
      char dateString[30] = { 0 };
      char timeString[30] = { 0 };
      if (logTimeOK) {
        strftime(dateString, 30, "%b %d %y", &logTimeinfo); // Format: e.g., "Oct 06 2024"
        strftime(timeString, 30, "%H:%M:%S", &logTimeinfo); // Format: e.g., "14:30:45"
      } else {
        strcpy(dateString, "----");
        strcpy(timeString, "--:--:--");
      }

      // Create the log entry
      String logData = String(dateString) + " " + String(timeString) + " , " +
                       selectedGas.getName() + ", " +
                       String((long)m_dataSource->getDoubleValue()) + " ppm, " +
                       String(m_dataSource->getRawMiliVolts()) + "mV, Range: " +
                       String((long)range) + " ppm, userID: " + String(userID) + "\n";

      // Save to SPIFFS
      if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
      }

      File logFile = SPIFFS.open("/log.txt", FILE_APPEND);  // Open file in append mode
      if (!logFile) {
        Serial.println("Failed to open file for writing");
        return;
      }

      logFile.print(logData);  // Write the data to the file
      logFile.close();  // Close the file

      // Update the last log time
      lastLogTime = currentMillis;
    }
    m_display->drawString(110, 51, "LOG-A");
  }

  m_display->display();
  delay(100);  // Adjust this delay based on your application's requirements


}

///////////////////////


SSD1306SleepTimerMenuRenderer::SSD1306SleepTimerMenuRenderer(SSD1306Wire* display, SleepTimer* sleepTimer) : SSD1306MenuRenderer(display),
  m_sleepTimer(sleepTimer)
{
}

void SSD1306SleepTimerMenuRenderer::render(Menu* menu)
{
  //int interval = m_sleepTimer->getSelectedInterval();

  //String intervalText = interval > 0 ? String(interval) : "CONTINUOUS";

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "TIMER SLEEP");
  m_display->drawLine(0, 24, 256, 24);
  m_display->drawString(64, 30 , menu->getName());
  m_display->display();
}

///////////////////////////

SSD1306RangeMenuRenderer::SSD1306RangeMenuRenderer(SSD1306Wire* display, Range* range) : SSD1306MenuRenderer(display),
  m_range(range)
{
}

void SSD1306RangeMenuRenderer::render(Menu* menu)
{
  float range = m_range->getSelectedRange();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Range");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(70, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->setFont(ArialMT_Plain_10);
  m_display->display();
}


///////////////////////////

SSD1306AlarmMenuRenderer::SSD1306AlarmMenuRenderer(SSD1306Wire* display, Alarm* alarm) : SSD1306MenuRenderer(display),
  m_alarm(alarm)
{
}

void SSD1306AlarmMenuRenderer::render(Menu* menu)
{
  float alarm = m_alarm->getSelectedAlarm();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Alarm : High");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}

///////////////////////////

SSD1306LowalarmMenuRenderer::SSD1306LowalarmMenuRenderer(SSD1306Wire* display, Lowalarm* lowalarm) : SSD1306MenuRenderer(display),
  m_lowalarm(lowalarm)
{
}

void SSD1306LowalarmMenuRenderer::render(Menu* menu)
{
  float lowalarm = m_lowalarm->getSelectedLowalarm();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Alarm : Low Flow");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}

///////////////////////////

SSD1306HourMenuRenderer::SSD1306HourMenuRenderer(SSD1306Wire* display, Hour* hour) : SSD1306MenuRenderer(display),
  m_hour(hour)
{
}

void SSD1306HourMenuRenderer::render(Menu* menu)
{
  int hour = m_hour->getSelectedHour();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Time : Hour");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_24);
  m_display->drawString(60, 20 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}

///////////////////////////

SSD1306MinuteMenuRenderer::SSD1306MinuteMenuRenderer(SSD1306Wire* display, Minute* minute) : SSD1306MenuRenderer(display),
  m_minute(minute)
{
}

void SSD1306MinuteMenuRenderer::render(Menu* menu)
{
  int minute = m_minute->getSelectedMinute();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Time : Minute");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_24);
  m_display->drawString(60, 20 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}


///////////////////////////////

SSD1306CalvalueMenuRenderer::SSD1306CalvalueMenuRenderer(SSD1306Wire* display, Calvalue* calvalue) : SSD1306MenuRenderer(display),
  m_calvalue(calvalue)
{
}

void SSD1306CalvalueMenuRenderer::render(Menu* menu)
{
  float calvalue = m_calvalue->getSelectedCalvalue();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(64, 0, "Ref H2 (ppm)");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(70, 26, menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  // Footer: SKIP (left) / S (center) / OPTION (right) — Cal Value lives
  // inside the cal sequence, so left action is "skip this step" rather
  // than "back to main menu".
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "SKIP");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "S");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->display();
}

///////////////////////////////


SSD1306OutportMenuRenderer::SSD1306OutportMenuRenderer(SSD1306Wire* display, Outport* outport) : SSD1306MenuRenderer(display),
  m_outport(outport)
{
}
void SSD1306OutportMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(64, 0, "Set User ID");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}


///////////////////////////////


SSD1306LoggingsetMenuRenderer::SSD1306LoggingsetMenuRenderer(SSD1306Wire* display, Loggingset* loggingset) : SSD1306MenuRenderer(display),
  m_loggingset(loggingset)
{
}
void SSD1306LoggingsetMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(64, 0, "Logging Set");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}


//////////////////////

SSD1306LoggingtimeMenuRenderer::SSD1306LoggingtimeMenuRenderer(SSD1306Wire* display, Loggingtime* loggingtime) : SSD1306MenuRenderer(display),
  m_loggingtime(loggingtime)
{
}
void SSD1306LoggingtimeMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(64, 0, "Logging Interval");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);

  m_display->display();
}



///////////////////////


SSD1306FlashLoggerMenuRenderer::SSD1306FlashLoggerMenuRenderer(SSD1306Wire* display, DataLogger* dataLogger) : SSD1306MenuRenderer(display),
  m_dataLogger(dataLogger)
{


}

void SSD1306FlashLoggerMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "DATA LOGGER");
  m_display->drawLine(0, 24, 256, 24);
  m_display->drawString(64, 30 , menu->getName());
  m_display->drawString(64, 40 , m_dataLogger->isFlashStoreSessionRunning() ? "Started" : "Idle");
  m_display->display();


}

///////////////////////////////





SSD1306CalStartMenuRenderer::SSD1306CalStartMenuRenderer(SSD1306Wire* display) : SSD1306MenuRenderer(display)
{

}
void SSD1306CalStartMenuRenderer::render(Menu* menu)
{ m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Calibration");
  m_display->drawLine(10, 16, 256, 16);
  m_display->drawString(64, 24, "<DOWN>: Start");
  m_display->drawString(64, 36, "<UP>: Finish");
  m_display->display();
}




SSD1306ZEROMenuRenderer::SSD1306ZEROMenuRenderer(SSD1306Wire* display, DataSource* dataSource, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_gasManager(gasManager)
{

}
void SSD1306ZEROMenuRenderer::render(Menu* menu)
{
  const float multiplier = 0.125F; //GAIN 1
  double sensor_val = m_dataSource->getDoubleValue();
  Gas& selectedGas = m_gasManager->getSelectedGas();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setFont(ArialMT_Plain_10);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Header
  m_display->drawString(64, 0, "Cal: Zero  [H2]");
  m_display->drawLine(0, 14, 128, 14);
  // Body (3 lines, y=17/27/37 — line 3 ends ~y=47 to clear the footer line)
  m_display->drawString(64, 17, "Insert zero gas");
  m_display->drawString(64, 27, String("Det: " + String(m_dataSource->getRawMiliVolts()) + "mV").c_str());
  m_display->drawString(64, 37, "Press S when stable");
  // Footer: DOWN skips this cal step; S commits the zero.
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "SKIP");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "S");
  m_display->display();
}

SSD1306CalGasMenuRenderer::SSD1306CalGasMenuRenderer(SSD1306Wire* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_calvalue(calvalue),
  m_gasManager(gasManager)
{

}
void SSD1306CalGasMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  float calvalue = m_calvalue->getSelectedCalvalue();
  m_display->setColor(WHITE);
  m_display->setFont(ArialMT_Plain_10);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Header
  m_display->drawString(64, 0, "Calibration - Span");
  m_display->drawLine(0, 14, 128, 14);
  // Body (3 lines, y=17/27/37 — line 3 ends ~y=47 to clear the footer line)
  m_display->drawString(64, 17, String("Cal gas: " + String((long)calvalue) + " ppm").c_str());
  m_display->drawString(64, 27, String("Det: " + String(m_dataSource->getRawMiliVolts()) + "mV").c_str());
  m_display->drawString(64, 37, "Apply cal gas");
  // Footer: DOWN skips this cal step; S commits the span calibration.
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "SKIP");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "S");
  m_display->display();
}


SSD1306BumpTestMenuRenderer::SSD1306BumpTestMenuRenderer(SSD1306Wire* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_calvalue(calvalue),
  m_gasManager(gasManager)
{

}
void SSD1306BumpTestMenuRenderer::render(Menu* menu)
{
  double sensor_val = m_dataSource->getDoubleValue();
  float  refPpm     = m_calvalue->getSelectedCalvalue();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setFont(ArialMT_Plain_10);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Header
  m_display->drawString(64, 0, "Bump Test  [H2]");
  m_display->drawLine(0, 14, 128, 14);
  // Body (3 lines, y=17/27/37 — line 3 ends ~y=47 to clear the footer line)
  m_display->drawString(64, 17, String("Ref: " + String((long)refPpm) + " ppm").c_str());
  m_display->drawString(64, 27, String("Read: " + String((long)sensor_val) + " ppm").c_str());
  m_display->drawString(64, 37, "Apply cal gas, press S");
  // Footer: standard settings-page layout (MENU / SET / OPTION) — Bump Test
  // lives in the UP-cycled settings section.
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "MENU");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "SET");
  m_display->setTextAlignment(TEXT_ALIGN_RIGHT);
  m_display->drawString(128, 51, "OPTION");
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->display();
}

SSD1306CalResMenuRenderer::SSD1306CalResMenuRenderer(SSD1306Wire* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_calvalue(calvalue),
  m_gasManager(gasManager)
{

}
void SSD1306CalResMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setFont(ArialMT_Plain_10);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Header
  m_display->drawString(64, 0, "Calibration - Result");
  m_display->drawLine(0, 14, 128, 14);

  // Body: 16-pt result + 10-pt cal date
  m_display->setFont(ArialMT_Plain_16);
  if (m_gasManager->wasCalibrationSuccessful()) {
    m_display->drawString(64, 17, "Success");
  } else {
    m_display->drawString(64, 17, "Failed");
  }
  m_display->setFont(ArialMT_Plain_10);

  // Read and display calibration date/time from EEPROM
  char calDateStr[32] = { 0 };
  EEPROM.readString(120, calDateStr, 31);
  if (calDateStr[0] != '\0') {
    m_display->drawString(64, 36, "Cal: " + String(calDateStr));
  }

  // Footer
  m_display->drawLine(0, 49, 128, 49);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 51, "SKIP");
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 51, "OK");
  m_display->display();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Info screen  —  SW version, last cal date, cal factors
// ─────────────────────────────────────────────────────────────────────────────

SSD1306InfoMenuRenderer::SSD1306InfoMenuRenderer(SSD1306Wire* display,
    GasManager*  gasManager,
    const char*  fwVersion)
  : SSD1306MenuRenderer(display),
    m_gasManager(gasManager),
    m_fwVersion(fwVersion)
{}

void SSD1306InfoMenuRenderer::render(Menu* menu)
{
  // Read calibration date from EEPROM (address 120, max 31 bytes ASCII)
  char calDate[32] = { 0 };
  EEPROM.readString(120, calDate, 31);
  bool hasDate = (calDate[0] != '\0' && (uint8_t)calDate[0] != 0xFF);

  m_display->clear();
  m_display->setFont(ArialMT_Plain_10);
  m_display->setColor(WHITE);

  // ── Header ──
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "DEVICE INFO");
  m_display->drawLine(0, 11, 128, 11);

  // ── SW version — split at last space so each part fits in 128 px ──
  // e.g. "M10-TCD-H2 VerK 05/15/2026" → "M10-TCD-H2 VerK" | "05/15/2026"
  String fwStr = String(m_fwVersion);
  int splitPos = fwStr.lastIndexOf(' ');
  if (splitPos > 0) {
    m_display->drawString(64, 13, fwStr.substring(0, splitPos));
    m_display->drawString(64, 23, fwStr.substring(splitPos + 1));
  } else {
    m_display->drawString(64, 13, fwStr);
  }

  // ── Cal date + calibration factors ──
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->drawString(0, 34, "Cal: " + String(hasDate ? calDate : "not calibrated"));
  m_display->drawString(0, 44, "Int: " + String(m_gasManager->getSelectedIntercept(), 5));
  m_display->drawString(0, 54, "Slp: " + String(m_gasManager->getSelectedSlope(),     5));

  m_display->display();
}
