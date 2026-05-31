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
  m_display->drawString(12, 51, "Up");
  m_display->drawString(63, 51, "Enter");
  m_display->drawString(113, 51, "Next");
  m_display->drawLine(0, 49, 256, 49);

  m_display->setFont(ArialMT_Plain_10);
  m_display->display();
}



SSD1306RunMenuRenderer::SSD1306RunMenuRenderer(SSD1306Wire* display, DataSource* dataSource, GasManager* gasManager, Alarm* alarm, Lowalarm* lowalarm, Range* range, Calvalue* calvalue, Outport* outport) : SSD1306MenuRenderer(display),
  m_dataSource(dataSource),
  m_gasManager(gasManager),
  m_alarm(alarm),
  m_lowalarm(lowalarm),
  m_range(range),
  m_calvalue(calvalue),
  m_outport(outport)
{

}
//SimpleKalmanFilter simpleKalmanFilter(2, 2, 0.1);


void SSD1306RunMenuRenderer::render(Menu* menu)
{
  const float multiplier = 0.125F; // GAIN 1
  int range = m_range->getSelectedRange();
  int alarm = m_alarm->getSelectedAlarm();
  int lowalarm = m_lowalarm->getSelectedLowalarm();
  int outport = m_outport->getSelectedOutport();
  int calvalue = m_calvalue->getSelectedCalvalue();
  int64_t startMicros = esp_timer_get_time();
  int v_b = m_dataSource->getRawMiliVolts_battery();
  Gas& selectedGas = m_gasManager->getSelectedGas();
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

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_LEFT);
  m_display->setFont(ArialMT_Plain_10);

  // Date & Time
  struct tm timeinfo;
  getLocalTime(&timeinfo, 10);
  char dateString[30] = { 0 };
  char timeString[30] = { 0 };
  strftime(dateString, 30, "%b %d %y", &timeinfo);
  strftime(timeString, 30, "%H:%M", &timeinfo);
  m_display->drawString(0, 0, String(timeString));
    m_display->drawString(40, 0, "ALM H  Log");

  m_display->drawXbm(110 , 2, 16, 8, battery_bits);

  // Horizontal and vertical lines
  m_display->drawLine(0, 14, 256, 14);
  m_display->drawLine(0, 49, 256, 49);
  m_display->drawLine(42, 14, 42, 64);  // Divide left section (CH2O)
  m_display->drawLine(85, 14, 85, 64);  // Divide middle section (H2S)

  // Check if the reading is above the alarm level
  bool isAboveAlarm = m_dataSource->getDoubleValue() > alarm;

  // Left section (CH2O)
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(15, 28, String(v_b/12));
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(9, 15, "CH2O");

  // Middle section (H2S)
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(60, 28, String(v_b/21));
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(55, 15, "H2S");

  // Right section (NH3)
  m_display->setFont(ArialMT_Plain_16);
  m_display->drawString(100, 28, String(v_b/2));
  m_display->setFont(ArialMT_Plain_10);
  m_display->drawString(100, 15, "NH3");

  // Display alarm if above threshold
  if (isAboveAlarm && alarm != 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime > blinkInterval) {
      displayOn = !displayOn;  // Toggle the display state
      lastBlinkTime = currentTime;
    }
    if (displayOn) {
      m_display->setFont(ArialMT_Plain_24);
      m_display->drawString(60, 18, "ALARM");
    }
  }

  // Battery voltage display
  m_display->drawString(100, 51, "ppm");  // Unit for NH3
  m_display->drawString(9, 51, "ppm");   // Unit for CH2O
  m_display->drawString(55, 51, "ppm");   // Unit for H2S

  if (outport == 1) {
    Serial.print((String(m_dataSource->getDoubleValue(), 0) + ",ppm," + String(m_dataSource->getRawMiliVolts()) + "mV," + String(range) + "rg\n").c_str());
    m_display->drawString(114, 51, "USB");
  }

  m_display->display();
  delay(100);  // Consider adjusting this delay based on your application's requirements
}



///////////////////////////

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
  int alarm = m_alarm->getSelectedAlarm();
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
  double sensor_val = m_dataSource->getDoubleValue();

  m_display->clear();
  m_display->setColor(WHITE);
  m_display->setTextAlignment(TEXT_ALIGN_CENTER);
  m_display->drawString(64, 0, "Calibration - Zero");
  m_display->drawLine(0, 16, 256, 16);
  m_display->drawString(64, 21, "“Place ZeroGas on Probe");
  m_display->drawString(64, 34, String("Det: " + String(m_dataSource->getRawMiliVolts()) + "mV").c_str());

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
  m_display->drawString(64, 34, String("Det: " + String(m_dataSource->getRawMiliVolts()) + "mV").c_str());
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
