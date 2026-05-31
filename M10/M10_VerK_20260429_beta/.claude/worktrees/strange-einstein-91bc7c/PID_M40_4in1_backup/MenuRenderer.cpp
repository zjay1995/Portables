// MenuRenderer.cpp
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
#include <Adafruit_ADS1015.h>
#include "SSD1306.h"
#include <Arduino.h>
#include <U8g2lib.h>

// Constructor for SSD1306GasMenuRenderer
SSD1306GasMenuRenderer::SSD1306GasMenuRenderer(SSD1306Wire* display) : SSD1306MenuRenderer(display) {}

void SSD1306GasMenuRenderer::render(Menu* menu) {
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Gas Selection");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24, menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Next");
  m_display->drawLine(0, 49, 256, 49);
  m_display->setFont(ArialMT_Plain_10);
  m_display->display();
}

// Constructor for SSD1306RunMenuRenderer
SSD1306RunMenuRenderer::SSD1306RunMenuRenderer(
  SSD1306Wire* display,
  DataSource* dataSource,
  GasManager* gasManager,
  Alarm* alarm,
  Lowalarm* lowalarm,
  Range* range,
  Calvalue* calvalue,
  Outport* outport
) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_gasManager(gasManager),
  m_alarm(alarm),
  m_lowalarm(lowalarm),
  m_range(range),
  m_calvalue(calvalue),
  m_outport(outport) {}


void SSD1306RunMenuRenderer::render(Menu* menu) {
    const float multiplier = 0.125F; // GAIN 1
    int range = m_range->getSelectedRange();
    int alarm = m_alarm->getSelectedAlarm(0);
    int lowalarm = m_lowalarm->getSelectedLowalarm();
    int outport = m_outport->getSelectedOutport();
    int calvalue = m_calvalue->getSelectedCalvalue();
    int64_t startMicros = esp_timer_get_time();
    int v_b = 1;
    Gas& selectedGas = m_gasManager->getSelectedGas();

    // Read all sensor values
    m_dataSource->readAllValues();

    // Battery icon bits
    static const unsigned char battery_bits[] = {
        0xFF, 0x7F,  // ####### #######
        0x01, 0x80,  // #             #
        0xFD, 0x87,  // # ####### #####
        0xFD, 0x87,  // # ####### #####
        0xFD, 0x87,  // # ####### #####
        0xFD, 0x87,  // # ####### #####
        0x01, 0x80,  // #             #
        0xFF, 0x7F   // ####### #######
    };

    // Blinking toggle
    static bool displayOn = true;
    static unsigned long lastBlinkTime = 0;
    const unsigned long blinkInterval = 1000; // Blink interval in milliseconds

    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime > blinkInterval) {
        displayOn = !displayOn;
        lastBlinkTime = currentTime;
    }

    bool highAlarmEnabled = true; // Boolean to enable or disable high alarm
    bool lowAlarmEnabled = false;  // Boolean to enable or disable low alarm

    m_display->clear();
    m_display->setColor(WHITE);
    m_display->setTextAlignment(TEXT_ALIGN_LEFT);
    m_display->setFont(ArialMT_Plain_10);

    // Display date & time
    struct tm timeinfo;
    getLocalTime(&timeinfo, 10);
    char dateString[30] = { 0 };
    char timeString[30] = { 0 };
    strftime(dateString, 30, "%b %d %y", &timeinfo);
    strftime(timeString, 30, "%H:%M", &timeinfo);
    m_display->drawString(0, 0, String(timeString));
    m_display->drawXbm(110, 3, 16, 8, battery_bits);

    m_display->setTextAlignment(TEXT_ALIGN_CENTER);
    m_display->drawString(64, 0, "ALM H LOG");
    m_display->drawLine(0, 14, 128, 14);
    m_display->drawLine(0, 39, 128, 39);
    m_display->drawLine(64, 14, 64, 64);

    // Get gas concentration values
    float o2_value = m_dataSource->getCalibratedValue(2) / 10.0;
    int co_value = m_dataSource->getCalibratedValue(0);
    int h2s_value = m_dataSource->getCalibratedValue(1);
    float ch4_value = m_dataSource->getCalibratedValue(3);

    // Alarm thresholds
    const float o2_high_alarm_threshold = 23.5;
    const float o2_low_alarm_threshold = 19.5;
    const int co_high_alarm_threshold = 200;
    const int co_low_alarm_threshold = 50;
    const int h2s_high_alarm_threshold = 20;
    const int h2s_low_alarm_threshold = 10;
    const float ch4_high_alarm_threshold = 50.0;
    const float ch4_low_alarm_threshold = 20.0;

    // Display gas concentration values with high and low alarm blinking
    m_display->setFont(ArialMT_Plain_10);
    
    // O2
    if (displayOn && ((highAlarmEnabled && o2_value > o2_high_alarm_threshold) || (lowAlarmEnabled && o2_value < o2_low_alarm_threshold))) {
        m_display->setFont(ArialMT_Plain_16);
        if (o2_value > o2_high_alarm_threshold) {
            m_display->drawString(30, 18, "HIGH!");
        } else if (o2_value < o2_low_alarm_threshold) {
            m_display->drawString(30, 18, "LOW!");
        }
    } else {
        m_display->setFont(ArialMT_Plain_10);
        m_display->drawString(9, 17, "O2"); // Unit
        m_display->drawString(12, 27, "%VOL"); // Unit
        m_display->setFont(ArialMT_Plain_16);
        m_display->drawString(44, 18, String(o2_value, 1));
    }

    // CO
    if (displayOn && ((highAlarmEnabled && co_value > co_high_alarm_threshold) || (lowAlarmEnabled && co_value < co_low_alarm_threshold))) {
        m_display->setFont(ArialMT_Plain_16);
        if (co_value > co_high_alarm_threshold) {
            m_display->drawString(96, 18, "HIGH!");
        } else if (co_value < co_low_alarm_threshold) {
            m_display->drawString(96, 18, "LOW!");
        }
    } else {
        m_display->setFont(ArialMT_Plain_10);
        m_display->drawString(75, 17, "CO"); // Unit
        m_display->drawString(75, 27, "ppm"); // Unit
        m_display->setFont(ArialMT_Plain_16);
        m_display->drawString(107, 18, String(co_value));
    }

    // H2S
    if (displayOn && ((highAlarmEnabled && h2s_value > h2s_high_alarm_threshold) || (lowAlarmEnabled && h2s_value < h2s_low_alarm_threshold))) {
        m_display->setFont(ArialMT_Plain_16);
        if (h2s_value > h2s_high_alarm_threshold) {
            m_display->drawString(30, 43, "HIGH!");
        } else if (h2s_value < h2s_low_alarm_threshold) {
            m_display->drawString(30, 43, "LOW!");
        }
    } else {
        m_display->setFont(ArialMT_Plain_10);
        m_display->drawString(10, 42, "H2S"); // Unit
        m_display->drawString(10, 52, "ppm"); // Unit
        m_display->setFont(ArialMT_Plain_16);
        m_display->drawString(44, 44, String(h2s_value));
    }

    // CH4
    if (displayOn && ((highAlarmEnabled && ch4_value > ch4_high_alarm_threshold) || (lowAlarmEnabled && ch4_value < ch4_low_alarm_threshold))) {
        m_display->setFont(ArialMT_Plain_16);
        if (ch4_value > ch4_high_alarm_threshold) {
            m_display->drawString(96, 43, "HIGH!");
        } else if (ch4_value < ch4_low_alarm_threshold) {
            m_display->drawString(96, 43, "LOW!");
        }
    } else {
        m_display->setFont(ArialMT_Plain_10);
        m_display->drawString(78, 42, "CH4"); // Unit
        m_display->drawString(79, 53, "%LEL"); // Unit
        m_display->setFont(ArialMT_Plain_16);
        m_display->drawString(107, 44, String(ch4_value, 1));
    }

    m_display->display();
    delay(100); // Adjust this delay based on your application's requirements
}
///////////////////

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
  int range = m_range->getSelectedRange();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Range");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(70, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

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
  int alarm = m_alarm->getSelectedAlarm(0);
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Alarm : High");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

  m_display->display();
}

///////////////////////////

SSD1306LowalarmMenuRenderer::SSD1306LowalarmMenuRenderer(SSD1306Wire* display, Lowalarm* lowalarm) : SSD1306MenuRenderer(display),
  m_lowalarm(lowalarm)
{
}

void SSD1306LowalarmMenuRenderer::render(Menu* menu)
{
  int lowalarm = m_lowalarm->getSelectedLowalarm();
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Alarm : Low");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

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
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

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
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

  m_display->display();
}


///////////////////////////////

SSD1306CalvalueMenuRenderer::SSD1306CalvalueMenuRenderer(SSD1306Wire* display, Calvalue* calvalue) : SSD1306MenuRenderer(display),
  m_calvalue(calvalue)
{
}

void SSD1306CalvalueMenuRenderer::render(Menu* menu)
{
  int calvalue = m_calvalue->getSelectedCalvalue();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(64, 0, "Cal Gas Value");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(70, 28 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
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
  m_display->drawString(64, 0, "Data Logging");
  m_display->drawLine(0, 16, 256, 16);
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 24 , menu->getName());
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

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
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Down");
  m_display->drawLine(0, 49, 256, 49);

  m_display->display();
}





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

SSD1306WiFiDumpMenuRenderer::SSD1306WiFiDumpMenuRenderer(SSD1306Wire* display, DataLogger* dataLogger) : SSD1306MenuRenderer(display),
  m_dataLogger(dataLogger)
{


}

void SSD1306WiFiDumpMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "WIFI DATA DUMP");
  m_display->drawLine(0, 24, 256, 24);
  m_display->drawString(64, 30, menu->getName());
  m_display->drawString(64, 40, m_dataLogger->isWiFiDumpRunning() ? "Started" : "Idle");
  m_display->display();


}

SSD1306WiFiRealTimeDumpMenuRenderer::SSD1306WiFiRealTimeDumpMenuRenderer(SSD1306Wire* display, DataLogger* dataLogger) : SSD1306MenuRenderer(display),
  m_dataLogger(dataLogger)
{


}

void SSD1306WiFiRealTimeDumpMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "WIFI REAL-TIME DUMP");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 30, menu->getName());
  m_display->drawString(64, 40, m_dataLogger->isWiFiDumpRunning() ? "Started" : "Idle");
  m_display->display();


}

SSD1306NTPSyncMenuRenderer::SSD1306NTPSyncMenuRenderer(SSD1306Wire* display, TimeSync* timeSync) : SSD1306MenuRenderer(display),
  m_timeSync(timeSync)
{

}

void SSD1306NTPSyncMenuRenderer::render(Menu* menu)
{
  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Time Sync");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 20, menu->getName());
  m_display->drawString(64, 30, m_timeSync->isNTCSyncRunning() == true ? "In Progress!" : "Idle");
  m_display->drawString(64, 40, "Press S to Start");
  m_display->display();

}

SSD1306ShowTimeMenuRenderer::SSD1306ShowTimeMenuRenderer(SSD1306Wire* display) : SSD1306MenuRenderer(display)
{

}

void SSD1306ShowTimeMenuRenderer::render(Menu* menu)
{
  int64_t startMicros = esp_timer_get_time();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);

  struct tm timeinfo;
  getLocalTime(&timeinfo, 10);

  int64_t passed = esp_timer_get_time() - startMicros;

  Serial.println("render time: " + String((uint32_t)(passed / 1000)));

  char dateString[30] = { 0 };
  char timeString[30] = { 0 };
  strftime(dateString, 30, "%b %d %y", &timeinfo);
  strftime(timeString, 30, "%H:%M:%S", &timeinfo);

  m_display->drawString(64, 0, "Current DateTime");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 28, String(dateString));
  m_display->drawString(64, 45, String(timeString));
  m_display->display();



}







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
  double sensor_val = m_dataSource->getCalibratedValue(1);
  m_display->setFont(ArialMT_Plain_10);

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Calibration - Zero");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 21, "“Place ZeroGas on Probe");
  m_display->drawString(64, 34, String("Det: " + String(m_dataSource->getCalibratedValue(1)) + "mV").c_str());

  m_display->drawString(64, 45, "Press S when Stable");
  m_display->display();
}

SSD1306CalGasMenuRenderer::SSD1306CalGasMenuRenderer(SSD1306Wire* display, DataSource* dataSource, Calvalue* calvalue, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_calvalue(calvalue),
  m_gasManager(gasManager)
{

}
void SSD1306CalGasMenuRenderer::render(Menu* menu)
{ m_display->clear();
  int calvalue = m_calvalue->getSelectedCalvalue();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Calibration - Cal Gas");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 22, String("Cal gas: " + String(calvalue) + " ppm").c_str());
  m_display->drawString(64, 34, String("Det: " + String(m_dataSource->getCalibratedValue(1)) + "mV").c_str());
  m_display->drawString(64, 45, "Press S when Stable");

  m_display->display();
}


SSD1306CalResMenuRenderer::SSD1306CalResMenuRenderer(SSD1306Wire* display, GasManager* gasManager) : SSD1306MenuRenderer(display),
  m_gasManager(gasManager)

{

}
void SSD1306CalResMenuRenderer::render(Menu* menu)
{ m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Calibration - Result");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 24, "Calibration Finished");
  m_display->drawString(64, 40, "<DOWN>: Restart");
  m_display->drawString(64, 48, "<UP>: Finish");
  m_display->display();
}
