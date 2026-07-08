#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SHT4x.h>
#include <TFT_eSPI.h> 
#include <Wire.h>
#include <vector>
#include "BluetoothSerial.h"

#include "inc/pins.h"
#include "inc/TimeSync.h"
#include "inc/GasManager.h"
#include "inc/Button.h"
#include "inc/Menu.h"
#include "inc/MenuRenderer.h"
#include "inc/SleepTimer.h"
#include "inc/RangeSet.h"
#include "inc/AlarmSet.h"
#include "inc/LowAlarmSet.h"
#include "inc/HourSet.h"
#include "inc/MinuteSet.h"
#include "inc/CalvalueSet.h"
#include "inc/OutputSet.h"
#include "inc/Globals.h"
#include "inc/DataLogger.h"
#include "inc/DataSource.h"
#include "inc/LoggingSet.h"
#include "inc/LoggingTime.h"
#include "inc/UnitSet.h"

using namespace std;

// Firmware name/version are defined in Globals.cpp (c_FIRMWARE_NAME /
// c_FIRMWARE_VERSION) so display renderers can show them too.
// Update the date there when releasing a new build.

GasManager g_gasManager(-2.054456771, 1, 0, 0, 0, 0, 0, 0, 0, 0);
BluetoothSerial BT;
CompositeMenu* g_mainMenu = nullptr;
TFTRunMenuRenderer* g_runRenderer = nullptr;  // for invalidate() after fillScreen
DataSource* g_dataSource = nullptr;           // for alarm evaluation in loop()

Adafruit_ADS1115 ads1115;
Adafruit_INA219 ina219;
Adafruit_SHT4x sht4x = Adafruit_SHT4x();

// Initialize TFT
TFT_eSPI tft = TFT_eSPI(); 

SleepTimer g_sleepTimer;
Range g_range;
Alarm g_alarm;
Lowalarm g_lowalarm;
Hour g_hour;
Minute g_minute;
Calvalue g_calvalue;
Outport g_outport;
Loggingset g_loggingset;
Loggingtime g_loggingtime;
UnitSet g_unitset;
DataLogger g_dataLogger;
TimeSync g_timeSync;

void setup() {
  Serial.begin(115200);
  Serial.println(String("PID M103 ") + c_FIRMWARE_VERSION);
  Wire.begin(23, 22); 

  BT.begin("ESP32 PID M103");
  BT.println("Bluetooth connection");
  Serial.println("The device started, now you can pair it with bluetooth!");

  delay(500);

  // DEEP-SLEEP init
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);

  // ALARM relay/buzzer drive (idle low).
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  // PID UV lamp -- enable on boot. Active-high per schematic LAMPOUT net.
  pinMode(LAMPOUT_PIN, OUTPUT);
  digitalWrite(LAMPOUT_PIN, HIGH);

  // High-voltage status feedback (input-only pin, GPIO 36 / SENSOR_VP).
  pinMode(HVON_STATUS_PIN, INPUT);

  // Wake from deep sleep on the power-switch button (formerly TPAD_G), IO 12.
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);

  // Initialize I2C-bus sensors and capture pass/fail for system check display.
  // ADS1115: begin() returns void — probe address 0x48 directly via I2C.
  // Re-assert Wire.begin(23, 22) after every sensor lib begin(): the old
  // Adafruit begin() calls Wire.begin() with no args, which can reset the bus
  // to the default 21/22 pins and make every following probe fail.
  ads1115.begin();
  // GAIN_ONE = ±4.096 V full scale, 0.125 mV/bit — matches the 0.125F
  // multiplier in AnalogSourceInput.h. The previous GAIN_TWOTHIRDS setting
  // (0.1875 mV/bit) made every displayed/captured mV value read 33% low.
  ads1115.setGain(GAIN_ONE);
  Wire.begin(23, 22);
  Wire.beginTransmission(0x48);
  bool sc_ads = (Wire.endTransmission() == 0);

  bool sc_ina = ina219.begin();
  if (!sc_ina) Serial.println("INA219 not found -- falling back to ADC on IO 34 for battery");
  Wire.begin(23, 22);

  // Battery voltage fallback via IO 34 resistor divider.
  analogSetPinAttenuation(VBATT_ADC_PIN, ADC_11db);

  bool sc_sht = sht4x.begin();
  if (!sc_sht) Serial.println("Couldn't find SHT4x");
  Wire.begin(23, 22);

  AnalogSourceInput* ads1115AnalogSourceInput = new ADS1115AnalogSourceInput(&ads1115, &ina219, &sht4x);
  DataSource* dataSource = new DataSource(&g_gasManager, ads1115AnalogSourceInput);
  g_dataSource = dataSource;

  g_gasManager.setConfigurationManager(&g_configurationManager);
  g_gasManager.addGas(Gas("100", 1.0));
  g_gasManager.addGas(Gas("1000", 1.0));

  // --- TFT INIT ---
  tft.init();
  tft.setRotation(0);  // 0 = portrait
  tft.fillScreen(TFT_BLACK);

  // Startup splash: PID Analyzers logo + firmware version, held 2 seconds.
  // The logo is drawn as styled text + chromatogram peak using TFT primitives.
  // To swap in a real bitmap later: convert pid_analyzers_logo.png to an
  // RGB565 C-array, store as logo[] with LOGO_W/LOGO_H, then replace the
  // block below with: tft.pushImage((sw - LOGO_W)/2, 60, LOGO_W, LOGO_H, logo);
  {
    const int sw = tft.width();
    const int sh = tft.height();
    const uint16_t LOGO_RED = tft.color565(204, 27, 38);  // approx brand red

    // ----- "PID" word -----
    // Note: TFT_eSPI fonts 6, 7, 8 are digits-only (1234567890:.-) and won't
    // render letters. Use font 4 with setTextSize() for large letter text.
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(LOGO_RED, TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextSize(3);                 // ~78 px tall
    tft.drawString("PID", sw / 2, 130);
    tft.setTextSize(1);

    // ----- separator line -----
    tft.drawFastHLine(20, 170, sw - 40, LOGO_RED);
    tft.drawFastHLine(20, 171, sw - 40, LOGO_RED);

    // ----- "ANALYZERS" word (no large bold available; use font 4 size 2) -----
    tft.setTextFont(4);
    tft.setTextSize(2);
    tft.drawString("ANALYZERS", sw / 2, 210);
    tft.setTextSize(1);

    // ----- subtitle + version -----
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("Single Gas PID Analyzer", sw / 2, sh / 2 + 80);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    tft.drawString(c_FIRMWARE_NAME, sw / 2, sh / 2 + 110);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(4);
    tft.drawString(c_FIRMWARE_VERSION, sw / 2, sh / 2 + 145);

    delay(2000);
    tft.fillScreen(TFT_BLACK);
  }

  // --- SYSTEM CHECK SCREEN ---
  {
    // RTC: probe PCF8563 at I2C address 0x51
    Wire.begin(23, 22);
    Wire.beginTransmission(0x51);
    bool sc_rtc = (Wire.endTransmission() == 0);

    // SPIFFS
    bool sc_spiffs = SPIFFS.begin(true);

    struct SysItem { const char* label; bool ok; };
    SysItem items[] = {
      { "RTC  PCF8563", sc_rtc    },
      { "ADC  ADS1115", sc_ads    },
      { "BAT  INA219 ", sc_ina    },
      { "RHT  SHT4x  ", sc_sht    },
      { "FLASH SPIFFS ", sc_spiffs },
    };
    const int N = sizeof(items) / sizeof(items[0]);

    bool anyFail = false;
    for (int i = 0; i < N; i++) if (!items[i].ok) anyFail = true;

    const int W = tft.width();
    const int H = tft.height();

    // Title
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    tft.drawString("SYSTEM CHECK", W / 2, 10);
    tft.drawFastHLine(10, 38, W - 20, TFT_WHITE);

    // Each component row
    int rowY = 60;
    for (int i = 0; i < N; i++) {
      // Label (left)
      tft.setTextDatum(ML_DATUM);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextFont(4);
      tft.drawString(items[i].label, 14, rowY);

      // Status badge (right)
      if (items[i].ok) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("[ OK ]", W - 10, rowY);
      } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setTextDatum(MR_DATUM);
        tft.drawString("[FAIL]", W - 10, rowY);
      }
      rowY += 58;
    }

    // Summary line
    tft.drawFastHLine(10, H - 70, W - 20, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    if (anyFail) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString("FAULT DETECTED", W / 2, H - 45);
    } else {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("ALL SYSTEMS OK", W / 2, H - 45);
    }

    delay(anyFail ? 4000 : 2000);
    tft.fillScreen(TFT_BLACK);
  }

  // Use TFT Renderers
  MenuRenderer* gasMenuRenderer = new TFTGasMenuRenderer(&tft);
  TFTRunMenuRenderer* runMenuRenderer = new TFTRunMenuRenderer(&tft, dataSource, &g_gasManager, &g_alarm, &g_lowalarm, &g_range, &g_calvalue, &g_outport, &g_loggingset, &g_loggingtime, &g_unitset);
  g_runRenderer = runMenuRenderer;
  MenuRenderer* sleepTimerMenuRenderer = new TFTSleepTimerMenuRenderer(&tft, &g_sleepTimer);
  MenuRenderer* rangeMenuRenderer = new TFTRangeMenuRenderer(&tft, &g_range);
  MenuRenderer* alarmMenuRenderer = new TFTAlarmMenuRenderer(&tft, &g_alarm);
  MenuRenderer* lowalarmMenuRenderer = new TFTLowalarmMenuRenderer(&tft, &g_lowalarm);
  MenuRenderer* hourMenuRenderer = new TFTHourMenuRenderer(&tft, &g_hour);
  MenuRenderer* minuteMenuRenderer = new TFTMinuteMenuRenderer(&tft, &g_minute);
  MenuRenderer* calvalueMenuRenderer = new TFTCalvalueMenuRenderer(&tft, &g_calvalue);
  MenuRenderer* loggingsetMenuRenderer = new TFTLoggingsetMenuRenderer(&tft, &g_loggingset);
  MenuRenderer* loggingtimeMenuRenderer = new TFTLoggingtimeMenuRenderer(&tft, &g_loggingtime);
  MenuRenderer* ZEROMenuRenderer = new TFTZEROMenuRenderer(&tft, dataSource, &g_gasManager);
  MenuRenderer* CalGasMenuRenderer = new TFTCalGasMenuRenderer(&tft, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* CalResMenuRenderer = new TFTCalResMenuRenderer(&tft, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* calInfoMenuRenderer = new TFTCalInfoMenuRenderer(&tft, &g_gasManager);

  // --- MENU BUILDING ---
  vector<Menu*> runMenus;
  runMenus.push_back(new RunMenuItem(" ", "RUN", 0, &g_gasManager, dataSource, &g_loggingset, &g_range, &g_outport, runMenuRenderer));
  CompositeMenu* runMenu = new CompositeMenu("RUN", "Main Menu", runMenus);

  vector<Menu*> gasMenus;
  gasMenus.push_back(new GasMenuItem("100", "LIBRARY",  0, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("1000", "LIBRARY", 1, &g_gasManager, gasMenuRenderer));
  CompositeMenu* libraryMenu = new CompositeMenu("LIBRARY", "Main Menu" , gasMenus);

  vector<Menu*> sleepTimerMenus;
  sleepTimerMenus.push_back(new SleepTimerMenuItem("5 min", "TIMER",  0, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("1 hour", "TIMER", 1, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("2 hour", "TIMER", 2, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("6 hour", "TIMER", 3, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("Continuous", "TIMER", 4, &g_sleepTimer, sleepTimerMenuRenderer));
  CompositeMenu* timerMenu = new CompositeMenu("TIMER", "Main Menu" , sleepTimerMenus);

  vector<Menu*> rangeMenus;
  rangeMenus.push_back(new RangeMenuItem("5000 ppm", "Range", 0, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("10000 ppm", "Range", 1, &g_range, rangeMenuRenderer));
  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu", rangeMenus);

  vector<Menu*> alarmMenus;
  int highalarmid = 0;
  std::array<int, 56> m_alarmArray{{0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 410, 420, 430, 440, 450, 460, 470, 480, 490, 500, 600, 700, 800, 900, 1000}};
  for (int ppm : m_alarmArray) {
    if (ppm == 0) alarmMenus.push_back(new AlarmMenuItem("Off", "High Alarm", highalarmid, &g_alarm, alarmMenuRenderer));
    else alarmMenus.push_back(new AlarmMenuItem(String(ppm) + " ppm", "High Alarm", highalarmid, &g_alarm, alarmMenuRenderer));
    highalarmid++;
  }
  CompositeMenu* alarmMenu = new CompositeMenu("Alarm", "Main Menu", alarmMenus);

  vector<Menu*> lowalarmMenus;
  String label = "PEL Alarm";
  for (int i = 0; i <= 60; i++) {
    if (i == 0) lowalarmMenus.push_back(new LowalarmMenuItem("Off", label, i, &g_lowalarm, lowalarmMenuRenderer));
    else lowalarmMenus.push_back(new LowalarmMenuItem(String(i) + " ppm", label, i, &g_lowalarm, lowalarmMenuRenderer));
  }
  CompositeMenu* lowalarmMenu = new CompositeMenu("PEL Alarm", "Main Menu" , lowalarmMenus);

  vector<Menu*> minuteMenus;
  for (int i = 0; i <= 59; i++) {
    minuteMenus.push_back(new MinuteMenuItem(String(i) , "Set Minute", i, &g_minute, minuteMenuRenderer));
  }
  CompositeMenu* minuteMenu = new CompositeMenu("Set Minute", "Main Menu" , minuteMenus);

  vector<Menu*> hourMenus;
  for (int i = 0; i <= 23; i++) {
    hourMenus.push_back(new HourMenuItem(String(i) , "Set Hour", i, &g_hour, hourMenuRenderer));
  }
  CompositeMenu* hourMenu = new CompositeMenu("Set Hour", "Main Menu" , hourMenus);

  vector<Menu*> calvalueMenus;
  int calid = 0;
  for (int i = 10; i <= 100; i += 10) calvalueMenus.push_back(new CalvalueMenuItem(String(i) + " ppm", "Cal Gas", calid++, &g_calvalue, calvalueMenuRenderer));
  for (int i = 200; i <= 500; i += 100) calvalueMenus.push_back(new CalvalueMenuItem(String(i) + " ppm", "Cal Gas", calid++, &g_calvalue, calvalueMenuRenderer));
  CompositeMenu* calvalueMenu = new CompositeMenu("Calvalue", "Main Menu" , calvalueMenus);

  vector<Menu*> loggingsetMenus;
  loggingsetMenus.push_back(new LoggingsetMenuItem("Off", " Gas", 0, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Auto", " Gas", 1, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Manual", " Gas", 2, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Clear Log", " Gas", 3, &g_loggingset, loggingsetMenuRenderer));
  CompositeMenu* loggingsetMenu = new CompositeMenu("Loggingset", "Main Menu" , loggingsetMenus);

  vector<Menu*> loggingtimeMenus;
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 sec", " Gas", 0, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("10 sec", " Gas", 1, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 minute", " Gas", 2, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("10 minute", " Gas", 3, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 hour", " Gas", 4, &g_loggingtime, loggingtimeMenuRenderer));
  CompositeMenu* loggingtimeMenu = new CompositeMenu("Loggingtime", "Main Menu" , loggingtimeMenus);

  MenuRenderer* unitMenuRenderer = new TFTUnitMenuRenderer(&tft);
  vector<Menu*> unitMenus;
  unitMenus.push_back(new UnitMenuItem("ppm",       "Unit", 0, &g_unitset, unitMenuRenderer));
  unitMenus.push_back(new UnitMenuItem("ppb",       "Unit", 1, &g_unitset, unitMenuRenderer));
  unitMenus.push_back(new UnitMenuItem("1117 Lamp", "Unit", 2, &g_unitset, unitMenuRenderer));
  CompositeMenu* unitMenu = new CompositeMenu("Unit", "Main Menu", unitMenus);

  vector<Menu*> calInfoMenus;
  calInfoMenus.push_back(new ShowTimeMenuItem("Cal Info", "CAL INFO", calInfoMenuRenderer));
  CompositeMenu* calInfoMenu = new CompositeMenu("CAL INFO", "Main Menu", calInfoMenus);

  vector<Menu*> calMenus;
  calMenus.push_back(new CalMenuItemZero("Zero", dataSource, &g_gasManager, "ZeroGas", ZEROMenuRenderer));
  CompositeMenu* calMenu = new CompositeMenu("CALIBRATION Zero", "Main Menu", calMenus);

  vector<Menu*> calgasMenus;
  calgasMenus.push_back(new CalMenuItemCalGas("Cal Gas", dataSource, &g_gasManager, "CalGas", CalGasMenuRenderer));
  CompositeMenu* calgasMenu = new CompositeMenu("CALIBRATION Calgas", "Main Menu", calgasMenus);

  vector<Menu*> calresMenus;
  calresMenus.push_back(new CalMenuItemCalRes("Cal Res", dataSource, &g_gasManager, "CalRes", CalResMenuRenderer));
  CompositeMenu* calresMenu = new CompositeMenu("Cal Result", "Main Menu", calresMenus);

  // --- FINAL MENU STRUCTURE ---
  // Indices 0-11: normal scrollable settings (DOWN cycles through these).
  // Indices 12-15: calibration sequence, only reachable via RIGHT from run screen.
  vector<Menu*> horizontalMenus;
  horizontalMenus.push_back(runMenu);        // 0
  horizontalMenus.push_back(libraryMenu);    // 1
  horizontalMenus.push_back(rangeMenu);      // 2  Range (5000 / 10000 ppm)
  horizontalMenus.push_back(alarmMenu);      // 3
  horizontalMenus.push_back(lowalarmMenu);   // 4
  horizontalMenus.push_back(loggingsetMenu); // 5
  horizontalMenus.push_back(loggingtimeMenu);// 6
  horizontalMenus.push_back(hourMenu);       // 7
  horizontalMenus.push_back(minuteMenu);     // 8
  horizontalMenus.push_back(unitMenu);       // 9  Display unit (ppm / ppb / 1117)
  horizontalMenus.push_back(timerMenu);      // 10 Auto power-off idle time
  horizontalMenus.push_back(calInfoMenu);    // 11 Cal factors & date (view only)
  horizontalMenus.push_back(calMenu);        // 12 Cal Step 1: Zero
  horizontalMenus.push_back(calvalueMenu);   // 13 Cal Step 2: set span gas value
  horizontalMenus.push_back(calgasMenu);     // 14 Cal Step 3: capture span
  horizontalMenus.push_back(calresMenu);     // 15 Cal result

  Serial.println("horizontal menu " + String(horizontalMenus.size()));
  CompositeMenu* verticalMenu = new CompositeMenu("Main Menu", "", horizontalMenus);
  g_mainMenu = verticalMenu;
  g_mainMenu->setNavLimit(12);  // DOWN only cycles indices 0-11; cal zone (12-15) is right-button only
  g_mainMenu->print();

  setupButtons();
  g_dataLogger.init(dataSource, &g_gasManager);

  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_gasManager);

  g_configurationManager.init();
  g_configurationManager.loadFromEEPROM();
  Wire.begin(23, 22);  // re-assert custom pins — ads/ina/sht begin() may have reset Wire
  g_timeSync.initTimeFromRTC();

  // EEPROM.read() returns uint8_t (0-255). On a blank/erased board every byte
  // is 0xFF (255). Clamp each value to its valid range before using it so a
  // fresh board boots cleanly with safe defaults instead of crashing.
  int range = EEPROM.read(72);
  if (range > 1) range = 0;                  // valid: 0 (5000 ppm) or 1 (10000 ppm)
  g_range.selectRangeByIndex(range);

  int gasindex = EEPROM.read(4);
  if (gasindex >= g_gasManager.getGasCount()) gasindex = 0;  // blank EEPROM → 255
  g_gasManager.selectGasByIndex(gasindex);

  int alarm = EEPROM.read(76);
  if (alarm >= 56) alarm = 0;                // 56 entries in alarm table
  g_alarm.selectAlarmByIndex(alarm);

  int lowalarm = EEPROM.read(92);
  if (lowalarm > 60) lowalarm = 0;           // 0-60 ppm PEL entries
  g_lowalarm.selectLowalarmByIndex(lowalarm);

  int loggingset = EEPROM.read(96);
  if (loggingset > 3) loggingset = 0;        // 0=Off,1=Auto,2=Manual,3=Clear
  g_loggingset.selectLoggingsetByIndex(loggingset);

  int unit = EEPROM.read(100);
  if (unit > 2) unit = 0;                    // 0=ppm, 1=ppb, 2=1117 lamp
  g_unitset.selectUnitByIndex(unit);

  int calvalue = EEPROM.read(80);
  if (calvalue >= 14) calvalue = 0;          // 14 entries in cal gas table
  g_calvalue.selectCalvalueByIndex(calvalue);

  int loggingtime = EEPROM.read(200);
  if (loggingtime > 4) loggingtime = 0;      // 1s/10s/1m/10m/1h
  g_loggingtime.selectLoggingtimeByIndex(loggingtime);

  // Manual log counter (ushort @ 210): blank EEPROM reads 0xFFFF.
  if (EEPROM.readUShort(210) == 0xFFFF) {
    EEPROM.writeUShort(210, 0);
    EEPROM.commit();
  }

  int sleepIdx = EEPROM.read(0);
  if (sleepIdx > 4) sleepIdx = 2;            // factory default: 2 hour auto-off
  g_sleepTimer.selectIntervalByIndex(sleepIdx);

  g_dataLogger.setLoggingConfig(&g_loggingset, &g_loggingtime);
}

static void showSavedBanner(const char* msg) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(2);
  tft.drawString(msg, tft.width() / 2, tft.height() / 2);
  tft.setTextSize(1);
  delay(700);
  tft.fillScreen(TFT_BLACK);
}

void setupButtons() {
  Keyboard* keyboard = new Keyboard();

  // DOWN: advance through regular settings (0-11). In cal zone, escape back to run.
  keyboard->addOnDownPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    tft.fillScreen(TFT_BLACK);
    if (g_runRenderer) g_runRenderer->invalidate();
    int idx = g_mainMenu->getCurrentIndex();
    if (idx >= 12) {
      g_mainMenu->setCurrentMenu(0);  // escape cal sequence
    } else {
      g_mainMenu->moveToNext();
    }
  });

  // S: save current value, flash confirmation, then advance to next screen.
  keyboard->addOnSPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    int idx = g_mainMenu->getCurrentIndex();
    g_mainMenu->action();
    g_timeSync.initTimeFromRTC();

    if (idx == 0 || idx == 11) {
      // Run screen: S triggers logging only. CalInfo: view-only, no banner.
      tft.fillScreen(TFT_BLACK);
      if (g_runRenderer) g_runRenderer->invalidate();
      if (idx == 11) g_mainMenu->moveToNext();  // calInfo → back to run
      return;
    }

    showSavedBanner(idx == 14 ? "CAL COMPLETE" : idx == 15 ? "CAL FINISHED" : "VALUE SAVED");
    if (g_runRenderer) g_runRenderer->invalidate();

    if (idx >= 12 && idx < 15) {
      g_mainMenu->setCurrentMenu(idx + 1);  // next cal step
    } else if (idx == 15) {
      g_mainMenu->setCurrentMenu(0);        // cal done → back to run
    } else {
      g_mainMenu->moveToNext();             // next regular setting (wraps to run)
    }
  });

  // RIGHT: from run → enter cal sequence. Elsewhere → cycle values within submenu.
  keyboard->addOnRightPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    tft.fillScreen(TFT_BLACK);
    if (g_runRenderer) g_runRenderer->invalidate();
    if (g_mainMenu->getCurrentIndex() == 0) {
      g_mainMenu->setCurrentMenu(12);  // jump straight to cal Zero (calInfo is menu-only)
    } else {
      ((CompositeMenu*)g_mainMenu->getCurrentMenu())->moveToNext();
    }
  });

  keyboard->addOnOnPressedFctor([] {
    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();
    Serial.println("SleepTimer deep_sleep!");
    digitalWrite(25, LOW);
    Serial.flush();
    delay(1000);
    esp_deep_sleep_start();
  });
  keyboard->addOnCalibrationComboPressedFctor([] {
    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();
    Serial.println("SleepTimer deep_sleep!");
    digitalWrite(25, LOW);
    Serial.flush();
    delay(1000);
    esp_deep_sleep_start();
  });
}

void listenForCode() {
  static String inputString = "";
  while (Serial.available() || BT.available()) {
    char inChar;
    if (Serial.available()) inChar = (char)Serial.read();
    else if (BT.available()) inChar = (char)BT.read();
    inputString += inChar;
    if (inChar == '\n') {
      inputString.trim();
      if (inputString == "PID73*") {
        spillLogContents();
      } else if (inputString == "CLEARLOG") {
        clearLogFile();
      }
      inputString = "";
    }
  }
}

void spillLogContents() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    BT.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  fs::File logFile = SPIFFS.open("/log.txt", FILE_READ);
  if (!logFile) {
    Serial.println("Failed to open log.txt");
    BT.println("Failed to open log.txt");
    return;
  }
  Serial.println("----- Start of log.txt -----");
  BT.println("----- Start of log.txt -----");
  while (logFile.available()) {
    String line = logFile.readStringUntil('\n');
    Serial.println(line);
    BT.println(line);
  }
  Serial.println("----- End of log.txt -----");
  BT.println("----- End of log.txt -----");
  logFile.close();
}

void clearLogFile() {
  fs::File logFile = SPIFFS.open("/log.txt", FILE_WRITE);
  if (!logFile) {
    Serial.println("Failed to open log.txt for clearing");
    return;
  }
  logFile.print("");
  logFile.close();
  EEPROM.writeUShort(210, 0);  // reset manual log counter shown on run screen
  EEPROM.commit();
  Serial.println("log.txt has been cleared");
}

// Drive the alarm relay/buzzer (IO 15, active-high) from the live reading and
// the saved setpoints. Runs every loop so the alarm tracks the gas level even
// when the operator is on a settings screen, and clears when the level drops.
static void handleAlarmOutput() {
  if (!g_dataSource) return;
  double ppm   = g_dataSource->getDoubleValue();
  int highSet  = g_alarm.getSelectedAlarm();
  int lowSet   = g_lowalarm.getSelectedLowalarm();
  bool active  = (highSet != 0 && ppm > highSet) || (lowSet != 0 && ppm > lowSet);
  digitalWrite(ALARM_PIN, active ? HIGH : LOW);
}

void loop() {
  ButtonPressDetector::handleTick();
  g_sleepTimer.handleTick();
  g_dataLogger.handleTick();
  handleAlarmOutput();
  g_mainMenu->render();
  listenForCode();
  delay(10);
}

