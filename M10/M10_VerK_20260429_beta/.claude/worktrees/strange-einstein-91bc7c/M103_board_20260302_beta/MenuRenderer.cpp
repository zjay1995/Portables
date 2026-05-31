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
#include "inc/GasManager.h"
#include "inc/DataLogger.h"
#include "inc/TimeSync.h"
#include <Arduino.h>
#include <EEPROM.h>

// Helper to center text with padding
void drawCenteredString(TFT_eSPI* tft, String text, int y, int font) {
  tft->setTextDatum(TC_DATUM); // Top Center alignment
  tft->setTextFont(font);
  tft->setTextPadding(tft->width()); // Clear full width
  tft->drawString(text, tft->width() / 2, y);
  tft->setTextPadding(0);
}

// =================================================================================
// GAS MENU RENDERER
// =================================================================================
void TFTGasMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);

  // Header
  drawCenteredString(m_display, "Range Selection", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);

  // Content (Vertically Centered)
  m_display->setTextSize(1);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);

  // Footer
  m_display->drawLine(0, m_display->height() - 35, m_display->width(), m_display->height() - 35, TFT_WHITE);
  m_display->setTextFont(4);
  m_display->setTextDatum(BL_DATUM); m_display->drawString("Next", 5, m_display->height() - 4);
  m_display->setTextDatum(BC_DATUM); m_display->drawString("Save", m_display->width() / 2, m_display->height() - 4);
  m_display->setTextDatum(BR_DATUM); m_display->drawString("Skip", m_display->width() - 5, m_display->height() - 4);
}

// =================================================================================
// RUN MENU RENDERER (Main Screen - Yellow & Large)
// =================================================================================
void TFTRunMenuRenderer::render(Menu* menu) {
  // --- Data Collection ---
  int range = m_range->getSelectedRange();
  int alarmVal = m_alarm->getSelectedAlarm();
  int lowalarmVal = m_lowalarm->getSelectedLowalarm();
  int loggingset = m_loggingset->getSelectedLoggingset();
  Gas& selectedGas = m_gasManager->getSelectedGas();

  double sensorValue = m_dataSource->getDoubleValue();
  int v_b = m_dataSource->getRawMiliVolts_battery();
  float tempVal = m_dataSource->getTemperature();
  float rhVal = m_dataSource->getHumidity();

  // --- HEADER (Time & Battery) ---
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextSize(1);
  m_display->setTextFont(4);

  struct tm timeinfo;
  getLocalTime(&timeinfo, 10);
  char timeString[30] = { 0 };
  strftime(timeString, 30, "%H:%M", &timeinfo);

  m_display->setTextDatum(TL_DATUM);
  m_display->setTextPadding(100);
  m_display->drawString(timeString, 2, 4);
  m_display->setTextPadding(0);

  float voltage = v_b / 1000.0;
  int percent = constrain(round((voltage - 3.4) / (4.05 - 3.4) * 100), 0, 100);
  m_display->setTextDatum(TR_DATUM);
  m_display->setTextPadding(120);
  m_display->drawString("B " + String(percent) + "%", m_display->width() - 2, 4);
  m_display->setTextPadding(0);

  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);

  // === DATA ROWS ===

  // ROW 1: Gas Name
  m_display->setTextColor(TFT_GREEN, TFT_BLACK);
  m_display->setTextFont(4);
  m_display->setTextDatum(TC_DATUM);
  m_display->setTextPadding(m_display->width());
  // y = 45
  //m_display->drawString("CO " + selectedGas.getName(), m_display->width() / 2, 45);
  m_display->drawString("PID", m_display->width() / 2, 45);
  m_display->setTextPadding(0);

  // ROW 2: PID Value (Yellow & Huge)
  bool triggerHigh = (alarmVal != 0 && sensorValue > alarmVal);
  bool triggerLow  = (lowalarmVal != 0 && sensorValue > lowalarmVal);

  if (triggerHigh || triggerLow) {
//    m_display->setTextColor(TFT_RED, TFT_BLACK);
//    m_display->setTextFont(4);
//    m_display->setTextSize(2);
//    m_display->setTextPadding(m_display->width());
//    m_display->drawString("ALARM", m_display->width() / 2, 90);
  } else {
    // YELLOW COLOR
    m_display->setTextColor(TFT_YELLOW, TFT_BLACK);
    m_display->setTextFont(4);
    m_display->setTextSize(3); // Huge Number

    m_display->setTextPadding(m_display->width());
    String valStr = String(sensorValue, 1);
    if ((range == 10000 && sensorValue > 10000) || (range == 5000 && sensorValue > 5000)) {
      valStr = "xxx";
    }
    // y = 80 (Centered in upper section)
    m_display->drawString(valStr, m_display->width() / 2, 80);
  }
  m_display->setTextSize(1);
  m_display->setTextPadding(0);

  // ROW 3: Units "ppm" (Yellow & Large)
  m_display->setTextColor(TFT_YELLOW, TFT_BLACK);
  m_display->setTextFont(4); // Changed from Font 2 to Font 4
  m_display->setTextDatum(TC_DATUM);
  m_display->drawString("ppm", m_display->width() / 2, 155);

  // ROW 4: Temperature (Yellow & Larger)
  m_display->setTextColor(TFT_YELLOW, TFT_BLACK);
  // Use Font 2 Size 2 (Height ~32px) for "Larger" look than standard Font 4 (26px)
  m_display->setTextFont(2);
  m_display->setTextSize(3);

  m_display->setTextDatum(TC_DATUM);
  m_display->setTextPadding(m_display->width());
  // y = 195
  String ppbStr = "PPB:   " + String(tempVal, 1) ;
  m_display->drawString(ppbStr, m_display->width() / 2, 205);
  m_display->setTextPadding(0);


  
  m_display->setTextDatum(TC_DATUM);
  m_display->setTextPadding(m_display->width());
  String tStr = "T:   " + String(tempVal, 1) + " C";
  m_display->drawString(tStr, m_display->width() / 2, 255);
  m_display->setTextPadding(0);

  // ROW 5: Humidity (Yellow & Larger)
  // y = 235
  m_display->setTextPadding(m_display->width());
  String rhStr = "RH:  " + String(rhVal, 1) + " %";
  m_display->drawString(rhStr, m_display->width() / 2, 310);
  m_display->setTextPadding(0);

  m_display->setTextSize(1); // Reset size

  // --- FOOTER ---
  m_display->drawLine(0, m_display->height() - 35, m_display->width(), m_display->height() - 35, TFT_WHITE);
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(4);

  // Alarm Status (Bottom Left)
  m_display->setTextDatum(BL_DATUM);
  m_display->setTextPadding(140);
  if (alarmVal != 0 || lowalarmVal != 0) {
    m_display->drawString("Alarm ON", 2, m_display->height() - 4);
  } else {
    m_display->drawString("Alarm OFF", 2, m_display->height() - 4);
  }

  // Sensor Voltage (Bottom Center + 20px Offset)
  m_display->setTextDatum(BC_DATUM);
  m_display->setTextPadding(110);
  // CHANGED: Added +20 to X position
  m_display->drawString(String(m_dataSource->getRawMiliVolts()) + "mV", (m_display->width() / 2) + 20, m_display->height() - 4);

  // Logging Status (Bottom Right)
  m_display->setTextDatum(BR_DATUM);
  m_display->setTextPadding(100);
  if (loggingset == 1) {
    m_display->drawString("LOG-A", m_display->width() - 2, m_display->height() - 4);
  } else if (loggingset == 2) {
    int logCount = EEPROM.readUShort(210);
    m_display->drawString("M " + String(logCount), m_display->width() - 2, m_display->height() - 4);
  }
  m_display->setTextPadding(0);
}

// =================================================================================
// GENERIC MENUS (Standard Footer Updated & Vertically Centered)
// =================================================================================

void drawStandardFooter(TFT_eSPI* tft) {
  tft->drawLine(0, tft->height() - 35, tft->width(), tft->height() - 35, TFT_WHITE);
  tft->setTextFont(4);
  tft->setTextDatum(BL_DATUM); tft->drawString("Next", 5, tft->height() - 4);
  tft->setTextDatum(BC_DATUM); tft->drawString("Save", tft->width() / 2, tft->height() - 4);
  tft->setTextDatum(BR_DATUM); tft->drawString("Skip", tft->width() - 5, tft->height() - 4);
}

void TFTSleepTimerMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "TIMER SLEEP", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
}

void TFTRangeMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Range", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTAlarmMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Alarm : High", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTLowalarmMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Alarm : PEL", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTHourMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Time : Hour", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTMinuteMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Time : Minute", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTCalvalueMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Cal Gas Value", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
}

void TFTOutportMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Set User ID", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTLoggingsetMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Logging Set", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTLoggingtimeMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Logging Interval", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 15, 4);
  drawStandardFooter(m_display);
}

void TFTFlashLoggerMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "DATA LOGGER", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 30, 2);
  drawCenteredString(m_display, m_dataLogger->isFlashStoreSessionRunning() ? "Started" : "Idle", m_display->height() / 2, 4);
}

void TFTShowTimeMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  char calDateStr[32] = { 0 };
  EEPROM.readString(120, calDateStr, 31);
  drawCenteredString(m_display, "Last Calibration:", 5, 4);
  drawCenteredString(m_display, String(calDateStr), m_display->height() / 2 - 15, 4);
}

void TFTCalStartMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Calibration", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, "<DOWN>: Start", m_display->height() / 2 - 30, 4);
  drawCenteredString(m_display, "<UP>: Finish", m_display->height() / 2 + 10, 4);
}

void TFTZEROMenuRenderer::render(Menu* menu) {
  Gas& selectedGas = m_gasManager->getSelectedGas();
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, selectedGas.getName() + "-Cal-Zero", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, "Place ZeroGas", 60, 2);

  m_display->setTextSize(2);
  drawCenteredString(m_display, String(m_dataSource->getRawMiliVolts()) + " mV", m_display->height() / 2, 4);
  m_display->setTextSize(1);

  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("Press S when Stable", m_display->width() / 2, m_display->height() - 5);
}

void TFTCalGasMenuRenderer::render(Menu* menu) {
  int calvalue = m_calvalue->getSelectedCalvalue();
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Cal Gas", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, "Gas: " + String(calvalue) + " ppm", 60, 2);

  m_display->setTextSize(2);
  drawCenteredString(m_display, String(m_dataSource->getRawMiliVolts()) + " mV", m_display->height() / 2, 4);
  m_display->setTextSize(1);

  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("Press S when Stable", m_display->width() / 2, m_display->height() - 5);
}

void TFTCalResMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Cal Result", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);

  if (m_gasManager->wasCalibrationSuccessful()) {
    m_display->setTextColor(TFT_GREEN, TFT_BLACK);
    drawCenteredString(m_display, "Success", m_display->height() / 2 - 15, 4);
  } else {
    m_display->setTextColor(TFT_RED, TFT_BLACK);
    drawCenteredString(m_display, "Failed", m_display->height() / 2 - 15, 4);
  }

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("OK", m_display->width() / 2, m_display->height() - 5);
}
