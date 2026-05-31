#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h> // Include INA219 library
#include <Adafruit_SHT4x.h>
#include <SSD1306.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <vector>
#include "BluetoothSerial.h"

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
#include "inc/image.h"
#include "inc/LoggingSet.h"
#include "inc/LoggingTime.h"
#include "inc/SensorConfig.h"  // active sensor profile — swap sensors here

using namespace std;

#define USE_SSD1306_DISPLAY

#define FW_VERSION  "M30 VerK 05/30/2026"
#define FW_BUILD    (__DATE__ " " __TIME__)

// ALARM_PIN defined in inc/Globals.h

#define MAX_SCCM 5000

// GasManager factory defaults — sourced from SensorConfig.h (sensor profile)
// Field calibration will overwrite intercept/slope values in EEPROM at runtime.
GasManager g_gasManager(SENSOR_INTERCEPT,  SENSOR_SLOPE,
                         0,                0,
                         SENSOR_INTERCEPT2, SENSOR_SLOPE2,
                         SENSOR_INTERCEPT3, SENSOR_SLOPE3,
                         SENSOR_INTERCEPT4, SENSOR_SLOPE4);

BluetoothSerial BT; // Create Bluetooth Serial object

CompositeMenu* g_mainMenu = nullptr;
BumpTestMenuItem* g_bumpTestItem = nullptr;  // result queried by S handler for banner

Adafruit_ADS1115 ads1115;
Adafruit_INA219 ina219; // Create INA219 object
Adafruit_SHT4x sht4x = Adafruit_SHT4x();

#ifdef USE_SSD1306_DISPLAY
SSD1306 display(0x3c, 21, 22);  // VerK board: SDA=21, SCL=22
#endif

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
DataLogger g_dataLogger;

volatile bool CALIBRATION_MODE = false;

// Sync the active sub-menu's cursor to the value saved in EEPROM so the
// display opens on the currently-selected option rather than always item 0.
// Index map: 0=Run 1=CalZero 2=Calvalue 3=CalGas 4=CalRes
//            5=Alarm 6=LowAlarm 7=LoggingSet 8=LoggingTime
//            9=Hour 10=Minute 11=Range 12=BumpTest 13=Info
void syncSubMenuToCurrent() {
  int mainIdx = g_mainMenu->getCurrentIndex();
  CompositeMenu* sub = (CompositeMenu*)g_mainMenu->getCurrentMenu();
  int savedIdx = 0;
  switch (mainIdx) {
    case 2:  savedIdx = EEPROM.read(80);  break;  // calvalue
    case 5:  savedIdx = EEPROM.read(76);  break;  // alarm
    case 6:  savedIdx = EEPROM.read(92);  break;  // lowalarm
    case 7:  savedIdx = EEPROM.read(96);  break;  // loggingset
    case 8:  savedIdx = EEPROM.read(200); break;  // loggingtime
    case 9:  savedIdx = 0; break;                 // hour — always start at 0
    case 10: savedIdx = 0; break;                 // minute — always start at 0
    case 11: savedIdx = EEPROM.read(72);  break;  // range
    default: return;  // run / cal pages / bump / info — no cursor to sync
  }
  sub->setCurrentMenu(savedIdx);
}

void syncTimeFromRTC() {
  RTC_PCF8563 rtc;
  if (rtc.begin()) {
    DateTime now = rtc.now();
    // Guard: PCF8563 returns garbage (e.g. year 2090) when it has lost power
    // or was never set. Only apply time if it looks valid.
    if (now.year() < 2020 || now.year() > 2040) {
      Serial.println("RTC: invalid time rejected (year=" + String(now.year()) + "). Set time via menu.");
      return;
    }
    uint32_t t = now.unixtime();
    struct timeval tv = { (time_t)t, 0 };
    settimeofday(&tv, NULL);
    Serial.println("RTC sync OK: " + String(now.year()) + "-" +
                   (now.month()  < 10 ? "0" : "") + String(now.month())  + "-" +
                   (now.day()    < 10 ? "0" : "") + String(now.day())    + " " +
                   (now.hour()   < 10 ? "0" : "") + String(now.hour())   + ":" +
                   (now.minute() < 10 ? "0" : "") + String(now.minute()));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Auto-sync RTC from firmware build timestamp on fresh upload (ported from M10)
//  EEPROM addr 152 holds the last-seen FW_BUILD string. Boot compares to the
//  current compile-time string; if they differ, set RTC to compile time + save
//  the string. Normal reboots match → RTC untouched (manual time preserved).
// ─────────────────────────────────────────────────────────────────────────────
#define EEPROM_BUILD_ADDR  152
#define EEPROM_BUILD_LEN   28

static DateTime parseBuildDateTime() {
  static const char* months[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
  };
  char mon[4] = {}; int day = 1, year = 2026, hour = 0, minute = 0, second = 0;
  sscanf(__DATE__, "%s %d %d", mon, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
  int month = 1;
  for (int i = 0; i < 12; i++) {
    if (strncmp(mon, months[i], 3) == 0) { month = i + 1; break; }
  }
  return DateTime(year, month, day, hour, minute, second);
}

static void autoSyncRTCFromBuild() {
  char storedBuild[EEPROM_BUILD_LEN] = {};
  EEPROM.readString(EEPROM_BUILD_ADDR, storedBuild, EEPROM_BUILD_LEN - 1);
  const char* currentBuild = FW_BUILD;

  if (strncmp(storedBuild, currentBuild, EEPROM_BUILD_LEN - 1) == 0) {
    Serial.println("Build unchanged — RTC auto-set skipped.");
    return;
  }

  Serial.println("New build detected! Setting RTC to compile time: " + String(currentBuild));

  RTC_PCF8563 rtc;
  if (!rtc.begin()) {
    Serial.println("RTC not found — cannot auto-set time.");
    return;
  }

  DateTime buildTime = parseBuildDateTime();
  rtc.adjust(buildTime);
  uint32_t t = buildTime.unixtime();
  struct timeval tv = { (time_t)t, 0 };
  settimeofday(&tv, NULL);

  EEPROM.writeString(EEPROM_BUILD_ADDR, currentBuild);
  EEPROM.commit();
}

unsigned long buttonPressStartTime = 0;
bool isButtonBeingPressed = false;


void setup() {

  Serial.begin(115200);
  Serial.print("Firmware: "); Serial.println(FW_VERSION);
  Serial.print("Build:    "); Serial.println(FW_BUILD);
  BT.begin(SENSOR_BT_NAME); // Bluetooth device name (from SensorConfig.h)
  BT.print("Firmware: "); BT.println(FW_VERSION);
  BT.print("Build:    "); BT.println(FW_BUILD);
  Serial.println("The device started, now you can pair it with bluetooth!");
  Wire.begin(21, 22);  // VerK board: SDA=21, SCL=22

  delay(500);
  // Other setup code...
  Serial.println("Initializing...");
  BT.println("Initializing...");

  // DEEP-SLEEP init
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);

  // Initialize sensors
  ads1115.begin();
  ads1115.setGain(SENSOR_ADS_GAIN); // from SensorConfig.h

  ina219.begin(); // Initialize INA219 sensor


  if (! sht4x.begin()) {
    Serial.println("Couldn't find SHT4x");
    //while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4x.readSerial(), HEX);

  // Initialize the display
#ifdef USE_SSD1306_DISPLAY
  // VerK board: SDA=21, SCL=22
  // If using the ThingPulse SSD1306 library:
  display.init();
#endif

  // Create DataSource with battery monitoring
  AnalogSourceInput* ads1115AnalogSourceInput = new ADS1115AnalogSourceInput(&ads1115, &ina219, &sht4x);
  DataSource* dataSource = new DataSource(&g_gasManager, ads1115AnalogSourceInput);


  // Gas Manager
  g_gasManager.setConfigurationManager(&g_configurationManager);

  // Gas list comes from the active sensor profile (SensorConfig.h).
  g_gasManager.addGas(Gas(SENSOR_GAS_NAME, 1.0));
  //
  /// Menus
  //

#ifdef USE_SSD1306_DISPLAY

  // Display (already init()'d above)
  display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_16);
  display.drawXbm(0, 0, 128, 60, image_arr);
  display.display();
  delay(750);
  display.setFont(ArialMT_Plain_10);

  // ----- SYSTEM CHECK -----
  {
    // Re-assert Wire pins after library begin() calls may have reset them.
    // VerK board: all peripherals on SDA=21 SCL=22.
    Wire.begin(21, 22);

    // Probe each I2C peripheral
    auto i2c_ok = [](uint8_t addr) -> bool {
      Wire.beginTransmission(addr);
      return (Wire.endTransmission() == 0);
    };

    bool chk_adc      = i2c_ok(0x48);  // ADS1115
    bool chk_ina      = i2c_ok(0x40);  // INA219
    bool chk_sht      = i2c_ok(0x44);  // SHT4x
    bool chk_rtc_hw   = i2c_ok(0x51);  // PCF8563 hardware present
    bool chk_rtc_time = false;
    if (chk_rtc_hw) {
      RTC_PCF8563 rtc;
      if (rtc.begin()) {
        DateTime now = rtc.now();
        chk_rtc_time = (now.year() >= 2020 && now.year() <= 2040);
      }
    }

    bool allOK = chk_adc && chk_ina && chk_sht && chk_rtc_hw;

    // Mirror results to Serial / BT
    Serial.println("-- SYSTEM CHECK --");
    Serial.println("  ADC1115 : " + String(chk_adc      ? "OK"    : "FAIL"));
    Serial.println("  INA219  : " + String(chk_ina      ? "OK"    : "FAIL"));
    Serial.println("  SHT4x   : " + String(chk_sht      ? "OK"    : "FAIL"));
    Serial.println("  RTC hw  : " + String(chk_rtc_hw   ? "OK"    : "FAIL"));
    Serial.println("  RTC time: " + String(chk_rtc_time ? "valid" : "needs set"));

    // Draw check screen
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64,  0, "SYSTEM CHECK");
    display.drawString(64, 10, FW_VERSION);
    display.drawLine(0, 21, 128, 21);

    // Helper: label at left, result at right
    auto drawItem = [&](int y, const char* label, bool ok) {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(2, y, label);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(126, y, ok ? "[ OK ]" : "[FAIL]");
    };

    drawItem(24, "ADC1115", chk_adc);
    drawItem(34, "INA219",  chk_ina);
    drawItem(44, "SHT4x",   chk_sht);

    // RTC: 3 states — hardware missing, time not set, OK
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(2, 54, "RTC");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    if      (!chk_rtc_hw)   display.drawString(126, 54, "[FAIL]");
    else if (!chk_rtc_time) display.drawString(126, 54, "[SET!]");
    else                    display.drawString(126, 54, "[ OK ]");

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.display();
    delay(allOK ? 1000 : 1500);
  }
  // ----- END SYSTEM CHECK -----

  display.clear();

  MenuRenderer* gasMenuRenderer = new SSD1306GasMenuRenderer(&display);
  MenuRenderer* runMenuRenderer = new SSD1306RunMenuRenderer(&display, dataSource, &g_gasManager, &g_alarm, &g_lowalarm, &g_range, &g_calvalue, &g_outport, &g_loggingset, &g_loggingtime);
  MenuRenderer* sleepTimerMenuRenderer = new SSD1306SleepTimerMenuRenderer(&display, &g_sleepTimer);
  MenuRenderer* rangeMenuRenderer = new SSD1306RangeMenuRenderer(&display, &g_range);
  MenuRenderer* alarmMenuRenderer = new SSD1306AlarmMenuRenderer(&display, &g_alarm);
  MenuRenderer* lowalarmMenuRenderer = new SSD1306LowalarmMenuRenderer(&display, &g_lowalarm);

  MenuRenderer* hourMenuRenderer = new SSD1306HourMenuRenderer(&display, &g_hour);
  MenuRenderer* minuteMenuRenderer = new SSD1306MinuteMenuRenderer(&display, &g_minute);
  MenuRenderer* calvalueMenuRenderer = new SSD1306CalvalueMenuRenderer(&display, &g_calvalue);
  MenuRenderer* outportMenuRenderer = new SSD1306OutportMenuRenderer(&display, &g_outport);
  MenuRenderer* loggingsetMenuRenderer = new SSD1306LoggingsetMenuRenderer(&display, &g_loggingset);
  MenuRenderer* loggingtimeMenuRenderer = new SSD1306LoggingtimeMenuRenderer(&display, &g_loggingtime);

  MenuRenderer* flashLoggerMenuRenderer = new SSD1306FlashLoggerMenuRenderer(&display, &g_dataLogger);
  MenuRenderer* CalStartMenuRenderer = new SSD1306CalStartMenuRenderer(&display);
  MenuRenderer* infoMenuRenderer     = new SSD1306InfoMenuRenderer(&display, &g_gasManager, FW_VERSION);

  MenuRenderer* ZEROMenuRenderer = new SSD1306ZEROMenuRenderer(&display, dataSource, &g_gasManager);
  MenuRenderer* CalGasMenuRenderer = new SSD1306CalGasMenuRenderer(&display, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* CalResMenuRenderer = new SSD1306CalResMenuRenderer(&display, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* BumpTestMenuRenderer = new SSD1306BumpTestMenuRenderer(&display, dataSource, &g_calvalue, &g_gasManager);

#endif

  // Run Menus
  vector<Menu*> runMenus;

  runMenus.push_back(new RunMenuItem(" ", "RUN", 0, &g_gasManager, dataSource, &g_loggingset, &g_range, &g_outport, runMenuRenderer));


  CompositeMenu* runMenu = new CompositeMenu("RUN", "Main Menu", runMenus);

  // Gas Library menu removed — active gas is selected at compile time via
  // SensorConfig.h. The GasManager still needs its m_gases populated so
  // getSelectedGas() works for renderers; that's done by the addGas() calls
  // earlier in setup() driven by the active sensor profile.

  // Timer Menus
  vector<Menu*> sleepTimerMenus;
  sleepTimerMenus.push_back(new SleepTimerMenuItem("5", "TIMER",  0, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("60", "TIMER", 1, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("120", "TIMER", 2, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("360", "TIMER", 3, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("CONTINUOUS", "TIMER", 4, &g_sleepTimer, sleepTimerMenuRenderer));

  CompositeMenu* timerMenu = new CompositeMenu("TIMER", "Main Menu" , sleepTimerMenus);


  // Helper: format a float using the sensor profile's display digits.
  // Strips trailing zeros for integer values so "100.0" shows as "100".
  auto fmtVal = [](float v) -> String {
    if (v == (int)v) return String((int)v);
    return String(v, SENSOR_RES_DIGITS);
  };

  // Range Menus — built from SENSOR_RANGE_VALUES in the active profile.
  vector<Menu*> rangeMenus;
  for (int i = 0; i < k_rangeCount; i++) {
    rangeMenus.push_back(new RangeMenuItem(
      fmtVal(k_rangeArr[i]) + " " + SENSOR_UNIT_LABEL,
      "Range", i, &g_range, rangeMenuRenderer));
  }
  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu", rangeMenus);

  // High Alarm Menus — built from SENSOR_ALARM_VALUES. First entry = 0 = Off.
  vector<Menu*> alarmMenus;
  for (int i = 0; i < k_alarmCount; i++) {
    if (k_alarmArr[i] == 0.0f) {
      alarmMenus.push_back(new AlarmMenuItem("Off", "High Alarm", i, &g_alarm, alarmMenuRenderer));
    } else {
      alarmMenus.push_back(new AlarmMenuItem(
        fmtVal(k_alarmArr[i]) + " " + SENSOR_UNIT_LABEL,
        "High Alarm", i, &g_alarm, alarmMenuRenderer));
    }
  }
  CompositeMenu* alarmMenu = new CompositeMenu("Alarm", "Main Menu", alarmMenus);

  // PEL Alarm Menus — built from SENSOR_PEL_VALUES. First entry = 0 = Off.
  vector<Menu*> lowalarmMenus;
  for (int i = 0; i < k_lowalarmCount; i++) {
    if (k_lowalarmArr[i] == 0.0f) {
      lowalarmMenus.push_back(new LowalarmMenuItem("Off", "PEL Alarm", i, &g_lowalarm, lowalarmMenuRenderer));
    } else {
      lowalarmMenus.push_back(new LowalarmMenuItem(
        fmtVal(k_lowalarmArr[i]) + " " + SENSOR_UNIT_LABEL,
        "PEL Alarm", i, &g_lowalarm, lowalarmMenuRenderer));
    }
  }
  CompositeMenu* lowalarmMenu = new CompositeMenu("PEL Alarm", "Main Menu", lowalarmMenus);


  // Hour Menus
  vector<Menu*> minuteMenus;
  for (int i = 0; i <= 59; i++) {
    String label = "Set Minute";
    minuteMenus.push_back(new MinuteMenuItem(String(i) , label, i, &g_minute, minuteMenuRenderer));
  }

  CompositeMenu* minuteMenu = new CompositeMenu("Set Minute", "Main Menu" , minuteMenus);

  // Minute Menus

  vector<Menu*> hourMenus;
  for (int i = 0; i <= 23; i++) {
    String label = "Set Hour";
    hourMenus.push_back(new HourMenuItem(String(i) , label, i, &g_hour, hourMenuRenderer));
  }

  CompositeMenu* hourMenu = new CompositeMenu("Set Hour", "Main Menu" , hourMenus);



  // Calvalue Menus — built from SENSOR_CALVALUE_VALUES in the active profile.
  vector<Menu*> calvalueMenus;
  for (int i = 0; i < k_calvalueCount; i++) {
    calvalueMenus.push_back(new CalvalueMenuItem(
      fmtVal(k_calvalueArr[i]) + " " + SENSOR_UNIT_LABEL,
      "Cal Gas", i, &g_calvalue, calvalueMenuRenderer));
  }
  CompositeMenu* calvalueMenu = new CompositeMenu("Calvalue", "Main Menu", calvalueMenus);

  // outport menus
  vector<Menu*> outportMenus;
  for (int i = 0; i <= 49; ++i) {
    String userName = "User " + String(i); // Use Arduino String class
    outportMenus.push_back(new OutportMenuItem(userName, " USER ID", i, &g_outport, outportMenuRenderer));
  }

  CompositeMenu* outportMenu = new CompositeMenu("USER ID", "Main Menu", outportMenus);




  // loggingset menus
  vector<Menu*> loggingsetMenus;
  loggingsetMenus.push_back(new LoggingsetMenuItem("Off", " Gas", 0, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Auto", " Gas", 1, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Manual", " Gas", 2, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Clear Log", " Gas", 3, &g_loggingset, loggingsetMenuRenderer));

  CompositeMenu* loggingsetMenu = new CompositeMenu("Loggingset", "Main Menu" , loggingsetMenus);

  // loggingtime menus
  vector<Menu*> loggingtimeMenus;
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 sec", " Gas", 0, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("10 sec", " Gas", 1, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 minute", " Gas", 2, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("10 minute", " Gas", 3, &g_loggingtime, loggingtimeMenuRenderer));
  loggingtimeMenus.push_back(new LoggingtimeMenuItem("1 hour", " Gas", 4, &g_loggingtime, loggingtimeMenuRenderer));

  CompositeMenu* loggingtimeMenu = new CompositeMenu("Loggingtime", "Main Menu" , loggingtimeMenus);

  // DataLogger Menus
  vector<Menu*> dataLoggerMenus;

  dataLoggerMenus.push_back(new DataLoggerFlashStoreMenuItem("FLASH LOGGER", "DATALOGGER",  &g_dataLogger, flashLoggerMenuRenderer));


  CompositeMenu* dataLoggerMenu = new CompositeMenu("DATALOGGER", "Main Menu" , dataLoggerMenus);

  // Cal Zero (first step of full cal sequence) — no standalone drift-zero
  // menu on M30. DOWN from Run lands here directly.
  vector<Menu*> calMenus;
  calMenus.push_back(new CalMenuItemZero("Zero", dataSource, &g_gasManager, "ZeroGas", ZEROMenuRenderer));
  CompositeMenu* calMenu = new CompositeMenu("CALIBRATION Zero", "Main Menu", calMenus);

  // Bump Test — quick pass/fail against current cal; no EEPROM writes.
  vector<Menu*> bumpTestMenus;
  g_bumpTestItem = new BumpTestMenuItem("Bump Test", dataSource, &g_gasManager, &g_calvalue, "BumpTest", BumpTestMenuRenderer);
  bumpTestMenus.push_back(g_bumpTestItem);
  CompositeMenu* bumpTestMenu = new CompositeMenu("BUMP TEST", "Main Menu", bumpTestMenus);

  // Cal gas menu
  vector<Menu*> calgasMenus;
  calgasMenus.push_back(new CalMenuItemCalGas("Cal Gas", dataSource, &g_gasManager, "CalGas", CalGasMenuRenderer));
  CompositeMenu* calgasMenu = new CompositeMenu("CALIBRATION Calgas", "Main Menu", calgasMenus);

  // Cal res menu
  vector<Menu*> calresMenus;
  calresMenus.push_back(new CalMenuItemCalRes("Cal Res", dataSource, &g_gasManager, "CalRes", CalResMenuRenderer));
  CompositeMenu* calresMenu = new CompositeMenu("Cal Result", "Main Menu", calresMenus);


  ////////////////////////////////////
  vector<Menu*> horizontalMenus;

  // Final index map (Library + ZeroOnly removed):
  //  0=Run        1=CalZero     2=Calvalue   3=CalGas      4=CalRes
  //  5=Alarm      6=LowAlarm    7=LoggingSet 8=LoggingTime
  //  9=Hour      10=Minute     11=Range     12=BumpTest   13=Info
  horizontalMenus.push_back(runMenu);         // 0

  horizontalMenus.push_back(calMenu);         // 1  Cal Zero (DOWN from Run lands here)
  horizontalMenus.push_back(calvalueMenu);    // 2  Cal Value
  horizontalMenus.push_back(calgasMenu);      // 3  Cal Gas (S returns to Run)
  horizontalMenus.push_back(calresMenu);      // 4  Cal Result (skipped — banner only)

  // libraryMenu omitted — single compile-time gas, no selection needed.
  // zeroOnlyMenu omitted on M30 — DOWN from Run enters the full cal sequence.

  horizontalMenus.push_back(alarmMenu);       // 5
  horizontalMenus.push_back(lowalarmMenu);    // 6
  horizontalMenus.push_back(loggingsetMenu);  // 7
  horizontalMenus.push_back(loggingtimeMenu); // 8 (skipped from main RIGHT cycle)

  horizontalMenus.push_back(hourMenu);        // 9
  horizontalMenus.push_back(minuteMenu);      // 10
  horizontalMenus.push_back(rangeMenu);       // 11
  horizontalMenus.push_back(bumpTestMenu);    // 12 Bump Test (S returns to Run)
  //horizontalMenus.push_back(outportMenu);

  // Info screen — last in sequence
  vector<Menu*> infoMenus;
  infoMenus.push_back(new InfoMenuItem("INFO", "Main Menu", infoMenuRenderer));
  CompositeMenu* infoMenu = new CompositeMenu("INFO", "Main Menu", infoMenus);
  horizontalMenus.push_back(infoMenu);        // 13

  Serial.println("horizontal menu " + String(horizontalMenus.size()));
  CompositeMenu* verticalMenu = new CompositeMenu("Main Menu", "", horizontalMenus);

  g_mainMenu = verticalMenu;

  g_mainMenu->print();

  setupButtons();

  //g_sleepTimer.init(&g_configurationManager);
  //g_range.init(&g_gasManager);

  g_dataLogger.init(dataSource, &g_gasManager);

  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_gasManager);
  //g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_range);

  g_configurationManager.init();

  // ── First-boot EEPROM seeding (ported from M10) ───────────────────────
  // Brand-new ESP32 has 0xFF in every Flash byte. readDouble(0xFF…) returns
  // a signaling NaN; String(NaN, 10) overflows dtostrf → boot loop.
  // Sentinel at addr 250 == 0xA8 marks "seeded"; if missing, write safe
  // defaults BEFORE loadFromEEPROM() reads anything.
  if (EEPROM.read(250) != 0xA8) {
    Serial.println("Blank EEPROM detected — seeding defaults from sensor profile");
    EEPROM.writeDouble(8,  SENSOR_INTERCEPT);
    EEPROM.writeDouble(16, SENSOR_SLOPE);
    EEPROM.writeDouble(24, SENSOR_INTERCEPT2);
    EEPROM.writeDouble(32, SENSOR_SLOPE2);
    EEPROM.writeDouble(40, SENSOR_INTERCEPT3);
    EEPROM.writeDouble(48, SENSOR_SLOPE3);
    EEPROM.writeDouble(56, SENSOR_INTERCEPT4);
    EEPROM.writeDouble(64, SENSOR_SLOPE4);
    EEPROM.writeInt(4,  0);   // gas index
    EEPROM.write(72,    0);   // range
    EEPROM.write(76,    0);   // high alarm (Off)
    EEPROM.write(80,    0);   // cal value
    EEPROM.write(92,    0);   // low alarm  (Off)
    EEPROM.write(96,    0);   // logging set (Off)
    EEPROM.write(200,   0);   // logging time
    EEPROM.writeString(120, "not calibrated");
    EEPROM.write(250,   0xA8);
    EEPROM.commit();
  }

  g_configurationManager.loadFromEEPROM();

  autoSyncRTCFromBuild();   // ported from M10: sets RTC to compile time on fresh upload
  syncTimeFromRTC();
  int range = EEPROM.read(72);
  g_range.selectRangeByIndex(range);
  int gasindex = EEPROM.read(4);

  g_gasManager.selectGasByIndex(gasindex);
  int alarm = EEPROM.read(76);
  g_alarm.selectAlarmByIndex(alarm);
  int lowalarm = EEPROM.read(92);
  g_lowalarm.selectLowalarmByIndex(lowalarm);
  int loggingset = EEPROM.read(96);
  g_loggingset.selectLoggingsetByIndex(loggingset);
  //g_mainMenu->setCurrentMenu(13);
}

static void showSavedBanner(const char* msg) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 24, msg);
  display.display();
  delay(700);
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_10);
}

void setupButtons()
{

  Keyboard* keyboard = new Keyboard();

  keyboard->addOnDownPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS DOWN");

    // M30: ZeroOnly removed — DOWN from Run enters the full cal sequence
    // directly at Cal Zero (idx 1). Elsewhere DOWN cycles sub-menu options.
    int cur = g_mainMenu->getCurrentIndex();
    if (cur == 0) {
      g_mainMenu->setCurrentMenu(1);   // Cal Zero (cal flow entry)
    } else {
      ((CompositeMenu*)g_mainMenu->getCurrentMenu())->moveToNext();
    }

  });
  keyboard->addOnSPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS S");

    int idx = g_mainMenu->getCurrentIndex();
    g_mainMenu->action();  // execute save + advance screen

    if (idx == 0) {
      // Run screen — log action only, no banner
      syncTimeFromRTC();
      return;
    }

    // Context-aware banner — index map (Library + ZeroOnly removed):
    //  0=Run 1=CalZero 2=Calvalue 3=CalGas 4=CalRes
    //  5=Alarm 6=LowAlarm 7=LoggingSet 8=LoggingTime
    //  9=Hour 10=Minute 11=Range 12=BumpTest 13=Info
    if (idx == 1)                    showSavedBanner("ZERO SET");   // Cal Zero (in full flow)
    else if (idx == 3) {
      showSavedBanner("CAL COMPLETE");
      g_mainMenu->setCurrentMenu(0);   // Return to Run after span
      syncTimeFromRTC();
      return;
    }
    else if (idx == 4)               showSavedBanner("CAL DONE");   // CalRes (rarely seen)
    else if (idx == 12) {
      bool ok = g_bumpTestItem && g_bumpTestItem->wasBumpTestOK();
      showSavedBanner(ok ? "BUMP OK" : "RECALIBRATE");
      g_mainMenu->setCurrentMenu(0);
      syncTimeFromRTC();
      return;
    }
    else if (idx == 9 || idx == 10)  showSavedBanner("TIME SAVED"); // Hour / Minute
    else                             showSavedBanner("SAVED");

    syncTimeFromRTC();

  });
  keyboard->addOnRightPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS RIGHT (main menu next)");

    // RIGHT acts as the demos' UP — cycles the main menu and syncs each
    // sub-menu's cursor to its EEPROM-saved value.
    g_mainMenu->print();
    g_mainMenu->moveToNext();
    syncSubMenuToCurrent();

  });
  keyboard->addOnOnPressedFctor([] {


    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();

    Serial.println("SleepTimer deep_sleep!");
    digitalWrite(25, LOW);
    Serial.flush();

    Wire.beginTransmission(0x3C);
    Wire.write(0x80);
    Wire.write(SSD1306_DISPLAYOFF);
    Wire.endTransmission(); //           m_display -> display();

    delay(1000);
    esp_deep_sleep_start();


    //    g_sleepTimer.resetIdleCounter();
    //    Serial.println("PRESS Mode");
    //
    //    if (g_mainMenu->getCurrentIndex() == 0) {
    //      g_mainMenu->setCurrentMenu(11);
    //    } else if (g_mainMenu->getCurrentIndex() == 11) {
    //      g_mainMenu->setCurrentMenu(0);
    //    }

  });

  keyboard->addOnCalibrationComboPressedFctor([] {

    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();

    Serial.println("SleepTimer deep_sleep!");
    digitalWrite(25, LOW);
    Serial.flush();

    Wire.beginTransmission(0x3C);
    Wire.write(0x80);
    Wire.write(SSD1306_DISPLAYOFF);
    Wire.endTransmission(); //           m_display -> display();

    delay(1000);
    esp_deep_sleep_start();
    return;
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

  File logFile = SPIFFS.open("/log.txt", FILE_READ);
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
  // Open the log file in write mode, which will truncate (clear) it
  File logFile = SPIFFS.open("/log.txt", FILE_WRITE);
  if (!logFile) {
    Serial.println("Failed to open log.txt for clearing");
    return;
  }

  // Write an empty string to clear the file
  logFile.print("");
  logFile.close();

  Serial.println("log.txt has been cleared");
}


void loop()
{

  ButtonPressDetector::handleTick();

  g_sleepTimer.handleTick();
  g_dataLogger.handleTick();

  g_mainMenu->render();
  listenForCode();

  delay(10);
}
