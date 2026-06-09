#include "inc/MenuRenderer.h"
#include "inc/Menu.h"
#include "inc/UnitSet.h"
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
void drawCenteredString(TFT_eSPI* tft, String text, int y, int font, int size = 1) {
  tft->setTextDatum(TC_DATUM); // Top Center alignment
  tft->setTextFont(font);
  tft->setTextSize(size);
  tft->setTextPadding(tft->width()); // Clear full width
  tft->drawString(text, tft->width() / 2, y);
  tft->setTextSize(1);
  tft->setTextPadding(0);
}

// =================================================================================
// INDUSTRIAL PANEL HELPERS
// =================================================================================
// Industrial color scheme
#define PANEL_BORDER   TFT_DARKGREY
#define PANEL_HEADER   0x10A2          // dark blue/navy header band
#define PANEL_BG       TFT_BLACK
#define LABEL_COLOR    TFT_LIGHTGREY
#define VALUE_COLOR    TFT_WHITE
#define READING_COLOR  TFT_YELLOW
#define OK_COLOR       TFT_GREEN
#define ALERT_COLOR    TFT_RED

// Draw a bordered panel with a header band containing a title.
// Returns the inner content y-start (just below the header).
// STATIC: call once per repaint; do not call every frame (causes flicker).
static int drawPanel(TFT_eSPI* tft, int x, int y, int w, int h, const String& title) {
  const int headerH = 18;
  tft->drawRect(x, y, w, h, PANEL_BORDER);
  tft->drawRect(x + 1, y + 1, w - 2, h - 2, PANEL_BORDER);
  tft->fillRect(x + 2, y + 2, w - 4, headerH, PANEL_HEADER);
  tft->setTextColor(TFT_WHITE, PANEL_HEADER);
  tft->setTextFont(2);
  tft->setTextDatum(ML_DATUM);
  tft->drawString(title, x + 6, y + 2 + headerH / 2);
  return y + 2 + headerH + 2;
}

// Draw a cell label (small, static text drawn once).
static void drawCellLabel(TFT_eSPI* tft, int x, int y, const String& label) {
  tft->setTextColor(LABEL_COLOR, PANEL_BG);
  tft->setTextFont(2);
  tft->setTextDatum(TL_DATUM);
  tft->drawString(label, x + 4, y);
}

// Draw a cell value (updated each frame, padded clears stale text).
static void drawCellValue(TFT_eSPI* tft, int x, int y, int w,
                          const String& value, uint16_t valueColor) {
  tft->setTextColor(valueColor, PANEL_BG);
  tft->setTextFont(4);
  tft->setTextDatum(TL_DATUM);
  tft->setTextPadding(w - 6);
  tft->drawString(value, x + 4, y + 16);
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
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);

  // Footer
  m_display->drawLine(0, m_display->height() - 35, m_display->width(), m_display->height() - 35, TFT_WHITE);
  m_display->setTextFont(4);
  m_display->setTextDatum(BL_DATUM); m_display->drawString("MENU", 5, m_display->height() - 4);
  m_display->setTextDatum(BC_DATUM); m_display->drawString("SET", m_display->width() / 2, m_display->height() - 4);
  m_display->setTextDatum(BR_DATUM); m_display->drawString("VALUE", m_display->width() - 5, m_display->height() - 4);
}

// =================================================================================
// RUN MENU RENDERER (Industrial panel layout, 320x480 portrait)
// Static-once / dynamic-every-frame split to eliminate flicker.
// =================================================================================
void TFTRunMenuRenderer::render(Menu* menu) {
  const int W = m_display->width();    // 320
  const int H = m_display->height();   // 480
  const int halfW = (W - 8) / 2;

  // Layout: enlarged top/bottom bars, env panel removed, extra space in reading.
  const int barH          = 38;
  const int readingPanelY = barH + 4;                            // 42
  const int readingPanelH = 222;
  const int almPanelY     = readingPanelY + readingPanelH + 4;  // 268
  const int almPanelH     = 90;
  const int stPanelY      = almPanelY + almPanelH + 4;          // 362
  const int stPanelH      = 82;
  const int footerY       = stPanelY + stPanelH + 4;            // 448
  const int footerH       = H - footerY;                        // 32

  const int readingInnerY = readingPanelY + 2 + 18 + 2;  // 64
  const int almInnerY     = almPanelY     + 2 + 18 + 2;  // 290
  const int stInnerY      = stPanelY      + 2 + 18 + 2;  // 384

  // Battery icon: sits right side of status bar, percentage text to its left
  const int battW = 32, battH = 14;
  const int battX = W - battW - 4;               // 284
  const int battY = (barH - battH) / 2;          // 12

  // ---- Data Collection ----
  int range       = m_range->getSelectedRange();
  int alarmVal    = m_alarm->getSelectedAlarm();
  int lowalarmVal = m_lowalarm->getSelectedLowalarm();
  int loggingset  = m_loggingset->getSelectedLoggingset();
  bool isPpb      = m_unitset->isPpb();
  Gas& selectedGas = m_gasManager->getSelectedGas();

  double rawPpm     = m_dataSource->getDoubleValue();
  double displayVal = isPpb ? rawPpm * 1000.0 : rawPpm;
  const char* unitLabel = isPpb ? "ppb" : "ppm";

  int v_b           = m_dataSource->getRawMiliVolts_battery();
  uint16_t sensorMv = m_dataSource->getRawMiliVolts();

  bool triggerHigh = (alarmVal    != 0 && rawPpm > alarmVal);
  bool triggerLow  = (lowalarmVal != 0 && rawPpm > lowalarmVal);
  bool overRange   = (range == 10000 && rawPpm > 10000) || (range == 5000 && rawPpm > 5000);

  // ============================================================
  // STATIC LAYER — drawn once per repaint
  // ============================================================
  if (!m_staticDrawn) {
    // Top status bar
    m_display->fillRect(0, 0, W, barH, PANEL_HEADER);
    m_display->drawFastHLine(0, barH, W, PANEL_BORDER);

    // Model name (center, font 2 keeps it compact so time/battery have room)
    m_display->setTextColor(TFT_WHITE, PANEL_HEADER);
    m_display->setTextFont(2);
    m_display->setTextDatum(MC_DATUM);
    m_display->drawString("PID  M103", W / 2, barH / 2);

    // Battery icon outline + nub
    m_display->drawRect(battX, battY, battW, battH, TFT_WHITE);
    m_display->fillRect(battX + battW, battY + 4, 3, battH - 8, TFT_WHITE);

    // Reading panel
    String gasTitle = selectedGas.getName();
    if (gasTitle == "100" || gasTitle == "1000") gasTitle = "PID";
    drawPanel(m_display, 4, readingPanelY, W - 8, readingPanelH, gasTitle + "  READING");

    // Alarm panel + labels + divider
    drawPanel(m_display, 4, almPanelY, W - 8, almPanelH, "ALARM SETPOINTS");
    drawCellLabel(m_display, 6, almInnerY, "HIGH");
    drawCellLabel(m_display, 4 + halfW, almInnerY, "PEL");
    m_display->drawFastVLine(4 + halfW - 1, almInnerY, almPanelH - 22, PANEL_BORDER);

    // Status panel + labels + divider
    drawPanel(m_display, 4, stPanelY, W - 8, stPanelH, "STATUS");
    drawCellLabel(m_display, 6, stInnerY, "RANGE");
    drawCellLabel(m_display, 4 + halfW, stInnerY, "SIGNAL");
    m_display->drawFastVLine(4 + halfW - 1, stInnerY, stPanelH - 22, PANEL_BORDER);

    // Footer bar
    m_display->fillRect(0, footerY, W, footerH, PANEL_HEADER);
    m_display->drawFastHLine(0, footerY, W, PANEL_BORDER);

    m_staticDrawn = true;
  }

  // ============================================================
  // DYNAMIC LAYER
  // ============================================================

  // Time (top-left, font 4 for readability)
  struct tm timeinfo;
  getLocalTime(&timeinfo, 10);
  char timeString[16] = { 0 };
  strftime(timeString, 16, "%H:%M", &timeinfo);
  m_display->setTextColor(TFT_WHITE, PANEL_HEADER);
  m_display->setTextFont(4);
  m_display->setTextDatum(ML_DATUM);
  m_display->setTextPadding(90);
  m_display->drawString(timeString, 6, barH / 2);
  m_display->setTextPadding(0);

  // Battery fill + percent (font 4, text left of icon)
  float voltage = v_b / 1000.0;
  int percent = constrain((int)round((voltage - 3.4) / (4.05 - 3.4) * 100), 0, 100);
  m_display->fillRect(battX + 2, battY + 2, battW - 4, battH - 4, PANEL_HEADER);
  uint16_t battColor = percent < 20 ? ALERT_COLOR : (percent < 50 ? READING_COLOR : OK_COLOR);
  int fillW = ((battW - 4) * percent) / 100;
  if (fillW > 0) m_display->fillRect(battX + 2, battY + 2, fillW, battH - 4, battColor);
  m_display->setTextColor(TFT_WHITE, PANEL_HEADER);
  m_display->setTextFont(4);
  m_display->setTextDatum(MR_DATUM);
  m_display->setTextPadding(46);
  m_display->drawString(String(percent) + "%", battX - 4, barH / 2);
  m_display->setTextPadding(0);

  // Big reading number
  uint16_t bigColor = (triggerHigh || overRange) ? ALERT_COLOR : READING_COLOR;
  m_display->setTextColor(bigColor, PANEL_BG);
  m_display->setTextDatum(MC_DATUM);
  m_display->setTextPadding(W - 16);
  if (overRange) {
    m_display->setTextFont(4);
    m_display->setTextSize(3);
    m_display->drawString("OVER", W / 2, readingInnerY + 80);
    m_display->setTextSize(1);
  } else {
    m_display->setTextFont(6);
    String readingStr = (isnan(displayVal) || isinf(displayVal))
                        ? "0.0"
                        : String(displayVal, isPpb ? 0 : 1);
    m_display->drawString(readingStr, W / 2, readingInnerY + 80);
  }
  m_display->setTextPadding(0);

  // Unit label (dynamic so ppm↔ppb switch takes effect immediately)
  m_display->setTextColor(LABEL_COLOR, PANEL_BG);
  m_display->setTextFont(4);
  m_display->setTextDatum(MC_DATUM);
  m_display->setTextPadding(100);
  m_display->drawString(unitLabel, W / 2, readingInnerY + 170);
  m_display->setTextPadding(0);

  // Alarm status chip (in reading panel header area)
  String alarmTag = "NORMAL";
  uint16_t chipColor = OK_COLOR;
  if (triggerHigh)     { alarmTag = "HIGH"; chipColor = ALERT_COLOR; }
  else if (triggerLow) { alarmTag = "PEL";  chipColor = READING_COLOR; }
  int chipW = 70, chipH = 18, chipX = W - 4 - chipW - 6, chipY = readingPanelY + 4;
  m_display->fillRect(chipX, chipY, chipW, chipH, chipColor);
  m_display->setTextColor(TFT_BLACK, chipColor);
  m_display->setTextFont(2);
  m_display->setTextDatum(MC_DATUM);
  m_display->drawString(alarmTag, chipX + chipW / 2, chipY + chipH / 2);

  // Alarm setpoint values
  String hiStr  = (alarmVal == 0)    ? "OFF" : (String(alarmVal)    + " ppm");
  String pelStr = (lowalarmVal == 0) ? "OFF" : (String(lowalarmVal) + " ppm");
  drawCellValue(m_display, 6,         almInnerY, halfW - 4, hiStr,  triggerHigh ? ALERT_COLOR   : VALUE_COLOR);
  drawCellValue(m_display, 4 + halfW, almInnerY, halfW - 4, pelStr, triggerLow  ? READING_COLOR : VALUE_COLOR);

  // Status values
  drawCellValue(m_display, 6,         stInnerY, halfW - 4, String(range) + " ppm",   VALUE_COLOR);
  drawCellValue(m_display, 4 + halfW, stInnerY, halfW - 4, String(sensorMv) + " mV", VALUE_COLOR);

  // Footer indicators (font 4 for larger readability)
  bool anyAlarm = (alarmVal != 0 || lowalarmVal != 0);
  m_display->setTextFont(4);
  m_display->setTextDatum(ML_DATUM);
  m_display->setTextPadding(W / 2 - 6);
  m_display->setTextColor(anyAlarm ? OK_COLOR : LABEL_COLOR, PANEL_HEADER);
  m_display->drawString(anyAlarm ? "ALARM: ON" : "ALARM: OFF", 6, footerY + footerH / 2);

  String logTxt = "LOG: --";
  uint16_t logColor = LABEL_COLOR;
  if (loggingset == 1) { logTxt = "LOG: AUTO"; logColor = OK_COLOR; }
  else if (loggingset == 2) {
    int logCount = EEPROM.readUShort(210);
    logTxt = "LOG: M " + String(logCount);
    logColor = OK_COLOR;
  }
  m_display->setTextDatum(MR_DATUM);
  m_display->setTextPadding(W / 2 - 6);
  m_display->setTextColor(logColor, PANEL_HEADER);
  m_display->drawString(logTxt, W - 6, footerY + footerH / 2);
  m_display->setTextPadding(0);
}

// =================================================================================
// GENERIC MENUS (Standard Footer Updated & Vertically Centered)
// =================================================================================

void drawStandardFooter(TFT_eSPI* tft) {
  tft->drawLine(0, tft->height() - 35, tft->width(), tft->height() - 35, TFT_WHITE);
  tft->setTextFont(4);
  tft->setTextDatum(BL_DATUM); tft->drawString("MENU",  5, tft->height() - 4);
  tft->setTextDatum(BC_DATUM); tft->drawString("SET",   tft->width() / 2, tft->height() - 4);
  tft->setTextDatum(BR_DATUM); tft->drawString("OPTIONS", tft->width() - 5, tft->height() - 4);
}

void TFTSleepTimerMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Auto Power-Off", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTRangeMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Range", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTAlarmMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Alarm : High", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTLowalarmMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Alarm : PEL", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTHourMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Time : Hour", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTMinuteMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Time : Minute", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTCalvalueMenuRenderer::render(Menu* menu) {
  const int W = m_display->width();
  const int H = m_display->height();
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Cal Gas Value", 5, 4);
  m_display->drawLine(0, 35, W, 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), H / 2 - 26, 4, 2);
  m_display->drawLine(0, H - 35, W, H - 35, TFT_WHITE);
  m_display->setTextFont(4);
  m_display->setTextDatum(BL_DATUM); m_display->drawString("ABORT",   5, H - 4);
  m_display->setTextDatum(BC_DATUM); m_display->drawString("SET",     W / 2, H - 4);
  m_display->setTextDatum(BR_DATUM); m_display->drawString("OPTIONS", W - 5, H - 4);
}

void TFTOutportMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Set User ID", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTLoggingsetMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Logging Set", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTLoggingtimeMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Logging Interval", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
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

// Track sample-to-sample drift to flag stability for the operator.
// chan: 0 = Zero screen, 1 = Span screen — separate histories so leftover
// zero-gas samples can't mark the span reading stable (and vice versa).
// Samples are taken every 500 ms to match the ADS1115 cache refresh (2 Hz);
// sampling every render frame just re-read the same cached value 8 times
// and reported STABLE immediately. 8 samples = a 4 s observation window.
// If the screen hasn't rendered for >2 s (operator left and came back),
// the history restarts.
static bool isStableMv(uint16_t mv, int chan) {
  static uint16_t history[2][8];
  static int idx[2] = {0, 0};
  static bool primed[2] = {false, false};
  static unsigned long lastSample[2] = {0, 0};

  unsigned long now = millis();
  if (lastSample[chan] != 0 && now - lastSample[chan] > 2000) {
    idx[chan] = 0;
    primed[chan] = false;
  }
  if (lastSample[chan] == 0 || now - lastSample[chan] >= 500) {
    lastSample[chan] = now;
    history[chan][idx[chan]] = mv;
    idx[chan] = (idx[chan] + 1) % 8;
    if (idx[chan] == 0) primed[chan] = true;
  }
  if (!primed[chan]) return false;

  uint16_t lo = history[chan][0], hi = history[chan][0];
  for (int i = 1; i < 8; i++) {
    if (history[chan][i] < lo) lo = history[chan][i];
    if (history[chan][i] > hi) hi = history[chan][i];
  }
  return (hi - lo) <= 2; // within 2 mV peak-to-peak
}

void TFTZEROMenuRenderer::render(Menu* menu) {
  Gas& selectedGas = m_gasManager->getSelectedGas();
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "STEP 1 / 2  -  ZERO", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, "Apply Zero Gas", 50, 4);
  drawCenteredString(m_display, "(clean air)", 80, 2);

  uint16_t mv = m_dataSource->getRawMiliVolts();
  m_display->setTextColor(READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, String(mv) + " mV", m_display->height() / 2 - 50, 4, 3);

  bool stable = isStableMv(mv, 0);
  m_display->setTextColor(stable ? OK_COLOR : READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, stable ? "[ STABLE ]" : "[ STABILIZING... ]", m_display->height() / 2 + 60, 4);

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("Press S to Capture Zero", m_display->width() / 2, m_display->height() - 5);
}

void TFTCalGasMenuRenderer::render(Menu* menu) {
  int calvalue = m_calvalue->getSelectedCalvalue();
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "STEP 2 / 2  -  SPAN", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, "Apply Cal Gas", 50, 4);
  m_display->setTextColor(READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, String(calvalue) + " ppm", 82, 4, 2);

  uint16_t mv = m_dataSource->getRawMiliVolts();
  m_display->setTextColor(READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, String(mv) + " mV", m_display->height() / 2 + 30, 4, 3);

  bool stable = isStableMv(mv, 1);
  m_display->setTextColor(stable ? OK_COLOR : READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, stable ? "[ STABLE ]" : "[ STABILIZING... ]", m_display->height() - 80, 4);

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("Press S to Capture Span", m_display->width() / 2, m_display->height() - 5);
}

void TFTCalResMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Cal Result", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);

  if (m_gasManager->wasCalibrationSuccessful()) {
    m_display->setTextColor(TFT_GREEN, TFT_BLACK);
    drawCenteredString(m_display, "Success", m_display->height() / 2 - 50, 4, 3);
  } else {
    m_display->setTextColor(TFT_RED, TFT_BLACK);
    drawCenteredString(m_display, "Failed", m_display->height() / 2 - 50, 4, 3);
  }

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(4);
  m_display->setTextDatum(BC_DATUM);
  m_display->drawString("OK", m_display->width() / 2, m_display->height() - 5);
}

void TFTUnitMenuRenderer::render(Menu* menu) {
  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "Display Unit", 5, 4);
  m_display->drawLine(0, 35, m_display->width(), 35, TFT_WHITE);
  drawCenteredString(m_display, menu->getName(), m_display->height() / 2 - 26, 4, 2);
  drawStandardFooter(m_display);
}

void TFTCalInfoMenuRenderer::render(Menu* menu) {
  const int W = m_display->width();
  const int H = m_display->height();

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredString(m_display, "CAL FACTORS", 5, 4);
  m_display->drawLine(0, 35, W, 35, TFT_WHITE);

  // --- Cal date ---
  char calDateStr[32] = { 0 };
  EEPROM.readString(EEPROM_CAL_TIMESTAMP_OFFSET, calDateStr, 31);
  String dateStr = (calDateStr[0] != '\0') ? String(calDateStr) : "Not calibrated";

  m_display->setTextColor(LABEL_COLOR, TFT_BLACK);
  m_display->setTextFont(2);
  m_display->setTextDatum(TC_DATUM);
  m_display->drawString("Last Calibration:", W / 2, 48);

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  m_display->setTextFont(2);
  m_display->setTextDatum(TC_DATUM);
  m_display->setTextPadding(W - 8);
  m_display->drawString(dateStr, W / 2, 66);
  m_display->setTextPadding(0);

  m_display->drawLine(0, 92, W, 92, TFT_DARKGREY);

  // --- Zero (intercept → mV) ---
  double intercept = m_gasManager->getSelectedIntercept();
  double zeroMv    = intercept * 1000.0;
  String zeroStr   = String(zeroMv, 1) + " mV";

  m_display->setTextColor(LABEL_COLOR, TFT_BLACK);
  m_display->setTextFont(2);
  m_display->setTextDatum(TC_DATUM);
  m_display->drawString("ZERO", W / 2, 102);

  m_display->setTextColor(READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, zeroStr, 120, 4, 2);

  m_display->drawLine(0, 210, W, 210, TFT_DARKGREY);

  // --- Slope ---
  double slope    = m_gasManager->getSelectedSlope();
  String slopeStr = String(slope, 6);

  m_display->setTextColor(LABEL_COLOR, TFT_BLACK);
  m_display->setTextFont(2);
  m_display->setTextDatum(TC_DATUM);
  m_display->drawString("SLOPE", W / 2, 220);

  m_display->setTextColor(READING_COLOR, TFT_BLACK);
  drawCenteredString(m_display, slopeStr, 238, 4, 2);

  // --- Gas name ---
  String gasName = m_gasManager->getSelectedGas().getName();
  if (gasName == "100" || gasName == "1000") gasName = "PID";
  m_display->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  m_display->setTextFont(2);
  m_display->setTextDatum(TC_DATUM);
  m_display->setTextPadding(W - 8);
  m_display->drawString("Gas: " + gasName, W / 2, H - 45);
  m_display->setTextPadding(0);

  m_display->setTextColor(TFT_WHITE, TFT_BLACK);
  drawStandardFooter(m_display);
}
