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

using namespace std;

#define USE_SSD1306_DISPLAY

#define FW_VERSION  "M10-TCD-H2 VerK 05/22/2026"
#define FW_BUILD    (__DATE__ " " __TIME__)

// ALARM_PIN defined in inc/Globals.h

// GasManager(intercept, slope, zero, calgas)
// TCD direct H2 measurement — no GCF correction.
// Slope defaults to 1 to avoid divide-by-zero before EEPROM loads.
GasManager g_gasManager(-2.054456771, 1, 0, 0);

BluetoothSerial BT; // Create Bluetooth Serial object

CompositeMenu* g_mainMenu = nullptr;
BumpTestMenuItem* g_bumpTestItem = nullptr;  // result queried by S handler for banner

Adafruit_ADS1115 ads1115;
Adafruit_INA219 ina219; // Create INA219 object
Adafruit_SHT4x sht4x = Adafruit_SHT4x();

#ifdef USE_SSD1306_DISPLAY
SSD1306 display(0x3c, 21, 22);
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
void syncSubMenuToCurrent() {
  int mainIdx = g_mainMenu->getCurrentIndex();
  CompositeMenu* sub = (CompositeMenu*)g_mainMenu->getCurrentMenu();
  int savedIdx = 0;
  switch (mainIdx) {
    case 3:  savedIdx = EEPROM.read(80);  break;  // calvalue
    case 6:  savedIdx = EEPROM.read(76);  break;  // alarm
    case 7:  savedIdx = EEPROM.read(92);  break;  // lowalarm
    case 8:  savedIdx = EEPROM.read(96);  break;  // loggingset
    case 9:  savedIdx = EEPROM.read(200); break;  // loggingtime
    case 10: savedIdx = 0; break;  // hour — always start from 0
    case 11: savedIdx = 0; break;  // minute — always start from 0
    case 12: savedIdx = EEPROM.read(72);  break;  // range
    default: return;  // run / zero / cal pages / bump / info — no cursor to sync
  }
  sub->setCurrentMenu(savedIdx);
}

void syncTimeFromRTC() {
  RTC_PCF8563 rtc;
  if (rtc.begin()) {
    DateTime now = rtc.now();
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
//  Auto-sync RTC from firmware build timestamp on fresh upload
//
//  EEPROM address 152 holds the last-seen FW_BUILD string (28 bytes).
//  On every boot we compare it to the current compile-time string.
//  If they differ → fresh upload → set RTC to __DATE__ / __TIME__ → save string.
//  If they match  → normal reboot → RTC is not touched (manual time is preserved).
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

  const char* currentBuild = FW_BUILD;   // e.g. "May 15 2026 14:30:00"

  if (strncmp(storedBuild, currentBuild, EEPROM_BUILD_LEN - 1) == 0) {
    Serial.println("Build unchanged — RTC auto-set skipped.");
    return;
  }

  Serial.println("New build detected! Setting RTC to compile time: " + String(currentBuild));
  BT.println("New build: setting RTC to " + String(currentBuild));

  RTC_PCF8563 rtc;
  if (!rtc.begin()) {
    Serial.println("RTC not found — cannot auto-set time.");
    return;
  }

  DateTime buildTime = parseBuildDateTime();
  rtc.adjust(buildTime);

  // Also push directly to ESP32 system clock so getLocalTime() works immediately
  uint32_t t = buildTime.unixtime();
  struct timeval tv = { (time_t)t, 0 };
  settimeofday(&tv, NULL);

  Serial.println("RTC set to: " +
    String(buildTime.year()) + "-" +
    (buildTime.month()  < 10 ? "0" : "") + String(buildTime.month())  + "-" +
    (buildTime.day()    < 10 ? "0" : "") + String(buildTime.day())    + " " +
    (buildTime.hour()   < 10 ? "0" : "") + String(buildTime.hour())   + ":" +
    (buildTime.minute() < 10 ? "0" : "") + String(buildTime.minute()));

  // Save current build string so next reboot does not overwrite a manually-set time
  EEPROM.writeString(EEPROM_BUILD_ADDR, currentBuild);
  EEPROM.commit();
}

unsigned long buttonPressStartTime = 0;
bool isButtonBeingPressed = false;


void setup() {

  Serial.begin(115200);
  Serial.print("Firmware: "); Serial.println(FW_VERSION);
  Serial.print("Build:    "); Serial.println(FW_BUILD);
  BT.begin("ESP32 TCD H2");  // Bluetooth device name
  BT.print("Firmware: "); BT.println(FW_VERSION);
  BT.print("Build:    "); BT.println(FW_BUILD);
  Serial.println("The device started, now you can pair it with bluetooth!");
  Wire.begin(21, 22);  // SDA=21, SCL=22 (VerK board)

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
  ads1115.setGain(GAIN_ONE);

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
  // Update these pins to 23, 22 to match your hardware
  // If using the ThingPulse SSD1306 library:
  display.init();
#endif

  // Create DataSource with battery monitoring
  AnalogSourceInput* ads1115AnalogSourceInput = new ADS1115AnalogSourceInput(&ads1115, &ina219, &sht4x);
  DataSource* dataSource = new DataSource(&g_gasManager, ads1115AnalogSourceInput);


  // Gas Manager
  g_gasManager.setConfigurationManager(&g_configurationManager);

  // Gas: H2 — TCD direct measurement, GCF = 1.0 (no correction needed)
  g_gasManager.addGas(Gas("H2", 1.00));
  //
  /// Menus
  //

#ifdef USE_SSD1306_DISPLAY

  // Display (already init()'d above)
  display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_16);
  display.drawXbm(0, 0, 128, 60, image_arr);
  display.display();
  delay(1000);
  display.setFont(ArialMT_Plain_10);

  // ----- SYSTEM CHECK -----
  {
    // Re-assert Wire pins: ads1115/ina219/sht4x begin() reset Wire to default
    // GPIO21/22; all peripherals are on SDA=23 SCL=22, same bus as the display.
    Wire.begin(21, 22);  // Re-assert VerK pins after sensor begin() calls

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
    bool chk_rtc_vl_ok = true;   // PCF8563 VL flag: false = oscillator stopped, time invalid
    if (chk_rtc_hw) {
      // Read VL bit (register 0x02, bit 7). If set, the oscillator has stopped
      // since the last time it was cleared — strong indicator of a backup-
      // battery / crystal / VBAT-routing problem.
      Wire.beginTransmission(0x51);
      Wire.write(0x02);
      if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom((uint8_t)0x51, (uint8_t)1);
        if (Wire.available()) {
          uint8_t vlSec = Wire.read();
          chk_rtc_vl_ok = !(vlSec & 0x80);
          Serial.println("  RTC VL reg 0x02 = 0x" + String(vlSec, HEX) +
                         " (VL bit " + String(chk_rtc_vl_ok ? "clear" : "SET") + ")");
        }
      }

      RTC_PCF8563 rtc;
      if (rtc.begin()) {
        DateTime now = rtc.now();
        chk_rtc_time = (now.year() >= 2020 && now.year() <= 2040);
      }
    }

    bool allOK = chk_adc && chk_ina && chk_sht && chk_rtc_hw && chk_rtc_vl_ok;

    // Mirror results to Serial / BT
    Serial.println("-- SYSTEM CHECK --");
    Serial.println("  ADC1115 : " + String(chk_adc      ? "OK"    : "FAIL"));
    Serial.println("  INA219  : " + String(chk_ina      ? "OK"    : "FAIL"));
    Serial.println("  SHT4x   : " + String(chk_sht      ? "OK"    : "FAIL"));
    Serial.println("  RTC hw  : " + String(chk_rtc_hw   ? "OK"    : "FAIL"));
    Serial.println("  RTC time: " + String(chk_rtc_time ? "valid" : "needs set"));
    Serial.println("  RTC bat : " + String(chk_rtc_vl_ok ? "OK"    : "LOW (osc stopped)"));

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

    // RTC: hardware missing → FAIL; VL bit set → BAT? (oscillator stopped);
    // time invalid → SET! ; otherwise OK.
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(2, 54, "RTC");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    if      (!chk_rtc_hw)    display.drawString(126, 54, "[FAIL]");
    else if (!chk_rtc_vl_ok) display.drawString(126, 54, "[BAT?]");
    else if (!chk_rtc_time)  display.drawString(126, 54, "[SET!]");
    else                     display.drawString(126, 54, "[ OK ]");

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.display();
    delay(allOK ? 2000 : 3500);
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

  // Gas Menus
  vector<Menu*> gasMenus;


  gasMenus.push_back(new GasMenuItem("Air",   "LIBRARY", 0, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("O2",    "LIBRARY", 1, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("N2",    "LIBRARY", 2, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("He",    "LIBRARY", 3, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("H2",    "LIBRARY", 4, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("ArCH4", "LIBRARY", 5, &g_gasManager, gasMenuRenderer));

  CompositeMenu* libraryMenu = new CompositeMenu("LIBRARY", "Main Menu" , gasMenus);

  // Timer Menus
  vector<Menu*> sleepTimerMenus;
  sleepTimerMenus.push_back(new SleepTimerMenuItem("5", "TIMER",  0, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("60", "TIMER", 1, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("120", "TIMER", 2, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("360", "TIMER", 3, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("CONTINUOUS", "TIMER", 4, &g_sleepTimer, sleepTimerMenuRenderer));

  CompositeMenu* timerMenu = new CompositeMenu("TIMER", "Main Menu" , sleepTimerMenus);


  // Range Menus (ppm H2 full-scale)
  vector<Menu*> rangeMenus;
  rangeMenus.push_back(new RangeMenuItem("1000 ppm",  "Range", 0, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("5000 ppm",  "Range", 1, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("10000 ppm", "Range", 2, &g_range, rangeMenuRenderer));
  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu" , rangeMenus);


  // High Alarm Menus (ppm H2 thresholds, 0.0 = Off)
  vector<Menu*> alarmMenus;

  const float alarmPpm[] = {
    0.0f,
    100.0f, 500.0f, 1000.0f, 2000.0f, 3000.0f, 4000.0f,
    5000.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f, 10000.0f
  };

  for (int i = 0; i < (int)(sizeof(alarmPpm)/sizeof(alarmPpm[0])); i++) {
    if (alarmPpm[i] == 0.0f) {
      alarmMenus.push_back(new AlarmMenuItem("Off", "High Alarm", i, &g_alarm, alarmMenuRenderer));
    } else {
      alarmMenus.push_back(new AlarmMenuItem(String((long)alarmPpm[i]) + " ppm", "High Alarm", i, &g_alarm, alarmMenuRenderer));
    }
  }

  CompositeMenu* alarmMenu = new CompositeMenu("Alarm", "Main Menu", alarmMenus);


  // Low Alarm Menus (0 to 10000 ppm H2 in 500 ppm steps, 0 = Off)
  vector<Menu*> lowalarmMenus;

  for (int i = 0; i <= 20; i++) {
    float ppm = i * 500.0f;
    if (ppm == 0.0f) {
      lowalarmMenus.push_back(new LowalarmMenuItem("Off", "Low Alarm", i, &g_lowalarm, lowalarmMenuRenderer));
    } else {
      lowalarmMenus.push_back(new LowalarmMenuItem(String((long)ppm) + " ppm", "Low Alarm", i, &g_lowalarm, lowalarmMenuRenderer));
    }
  }
  CompositeMenu* lowalarmMenu = new CompositeMenu("Low Alarm", "Main Menu" , lowalarmMenus);


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



  // calvalue Menus — reference H2 concentration in ppm
  vector<Menu*> calvalueMenus;
  int calid = 0;

  const float ppmValues[] = {
    500.0f, 1000.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f,
    6000.0f, 7000.0f, 8000.0f, 9000.0f, 10000.0f
  };
  for (float v : ppmValues) {
    calvalueMenus.push_back(new CalvalueMenuItem(String((long)v) + " ppm", "Ref H2 ppm", calid++, &g_calvalue, calvalueMenuRenderer));
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

  // Bump Test menu — quick pass/fail check using current cal; no EEPROM writes.
  vector<Menu*> bumpTestMenus;
  g_bumpTestItem = new BumpTestMenuItem("Bump Test", dataSource, &g_gasManager, &g_calvalue, "BumpTest", BumpTestMenuRenderer);
  bumpTestMenus.push_back(g_bumpTestItem);
  CompositeMenu* bumpTestMenu = new CompositeMenu("BUMP TEST", "Main Menu", bumpTestMenus);

  // Standalone Zero menu — re-zero only (no span). Uses same renderer/action
  // as Cal Zero but is wired up so the S handler returns to Run after saving.
  vector<Menu*> zeroOnlyMenus;
  zeroOnlyMenus.push_back(new CalMenuItemZero("Zero", dataSource, &g_gasManager, "ZeroOnly", ZEROMenuRenderer));
  CompositeMenu* zeroOnlyMenu = new CompositeMenu("ZERO DRIFT", "Main Menu", zeroOnlyMenus);

  // Cal zero menu
  vector<Menu*> calMenus;
  calMenus.push_back(new CalMenuItemZero("Zero", dataSource, &g_gasManager, "ZeroGas", ZEROMenuRenderer));
  CompositeMenu* calMenu = new CompositeMenu("CALIBRATION Zero", "Main Menu", calMenus);

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

  horizontalMenus.push_back(runMenu);       // 0

  // Cal section (entered via DOWN from Run)
  horizontalMenus.push_back(zeroOnlyMenu);  // 1  Zero only  (S returns to Run)
  horizontalMenus.push_back(calMenu);       // 2  Cal Zero
  horizontalMenus.push_back(calvalueMenu);  // 3  Cal Value
  horizontalMenus.push_back(calgasMenu);    // 4  Cal Gas    (S returns to Run)
  horizontalMenus.push_back(calresMenu);    // 5  Cal Result (skipped — banner only)

  // libraryMenu omitted — single gas H2, no selection needed

  // Settings cycle (entered via UP from Run)
  horizontalMenus.push_back(alarmMenu);       // 6
  horizontalMenus.push_back(lowalarmMenu);    // 7
  horizontalMenus.push_back(loggingsetMenu);  // 8
  horizontalMenus.push_back(loggingtimeMenu); // 9 (skipped in UP cycle)

  horizontalMenus.push_back(hourMenu);        // 10
  horizontalMenus.push_back(minuteMenu);      // 11
  horizontalMenus.push_back(rangeMenu);       // 12
  horizontalMenus.push_back(bumpTestMenu);    // 13 Bump Test (S returns to Run)
  //horizontalMenus.push_back(outportMenu);

  // Info screen — last in sequence
  vector<Menu*> infoMenus;
  infoMenus.push_back(new InfoMenuItem("INFO", "Main Menu", infoMenuRenderer));
  CompositeMenu* infoMenu = new CompositeMenu("INFO", "Main Menu", infoMenus);
  horizontalMenus.push_back(infoMenu);        // 14

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
  g_configurationManager.loadFromEEPROM();

  autoSyncRTCFromBuild();  // Set RTC from compile time if this is a fresh upload
  syncTimeFromRTC();       // Push RTC time → ESP32 system clock
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
  delay(800);
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

    // DOWN behavior:
    //   Run (0)        -> enter cal section at Zero-only (1)
    //   Zero-only (1)  -> escalate to full cal at Cal Zero (2)
    //   elsewhere      -> cycle options within the active sub-menu
    int cur = g_mainMenu->getCurrentIndex();
    if (cur == 0) {
      g_mainMenu->setCurrentMenu(1);
    } else if (cur == 1) {
      g_mainMenu->setCurrentMenu(2);
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

    // Context-aware banner
    if (idx == 1) {
      // Standalone Zero — drift correction, no full cal sequence.
      showSavedBanner("ZERO SET");
      g_mainMenu->setCurrentMenu(0);   // back to Run
      syncTimeFromRTC();
      return;
    }
    else if (idx == 2)          showSavedBanner("ZERO SET");  // Cal Zero (in full cal flow)
    else if (idx == 4) {
      showSavedBanner(g_gasManager.wasCalibrationSuccessful() ? "CAL SUCCESS" : "CAL FAILED");
      g_mainMenu->setCurrentMenu(0);  // Return to run screen after calibration
      syncTimeFromRTC();
      return;
    }
    else if (idx == 13) {
      // Bump Test — compare-only, no EEPROM write. Result was computed in action().
      bool ok = g_bumpTestItem && g_bumpTestItem->wasBumpTestOK();
      showSavedBanner(ok ? "BUMP OK" : "RECALIBRATE");
      g_mainMenu->setCurrentMenu(0);   // back to Run
      syncTimeFromRTC();
      return;
    }
    else if (idx == 10 || idx == 11) {
      // Apply the saved hour/minute directly to the RTC
      RTC_PCF8563 rtc;
      if (rtc.begin()) {
        DateTime now = rtc.now();
        int h = (idx == 10) ? g_hour.getSelectedHour()     : (int)now.hour();
        int m = (idx == 11) ? g_minute.getSelectedMinute() : (int)now.minute();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
        Serial.println("RTC set to " + String(h) + ":" + String(m));
      } else {
        Serial.println("RTC begin failed — time not updated");
      }
      showSavedBanner("TIME SAVED");
      syncTimeFromRTC();
      return;
    }
    else                        showSavedBanner("SAVED");

    syncTimeFromRTC();

  });
  keyboard->addOnUpPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS UP");

    // UP navigates main menu (alarm/settings sequence)
    g_mainMenu->print();
    g_mainMenu->moveToNext();
    syncSubMenuToCurrent();  // open each menu on its currently-saved value

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



// ─────────────────────────────────────────────────────────────────────────────
//  RTC diagnostics — invoke via Serial or BT command "RTC?" / "RTCDIAG"
//
//  Use to debug "RTC loses time after N minutes" symptoms. Dumps:
//    - millis since boot (how long device has been on)
//    - current PCF8563 time via RTClib
//    - raw register dump 0x00..0x0F
//    - VL bit interpretation (osc-stopped flag)
//
//  Procedure to isolate backup-battery faults:
//    1. Send "RTC?" right after a fresh upload (sets time to compile timestamp).
//    2. Power the device off for N minutes (5, 30, 60, …).
//    3. Power on, wait for boot, send "RTC?" again.
//    4. If reported time advanced correctly → RTC + battery are fine.
//       If VL bit is SET, or time reset to 2000-01-01 → backup path broken.
// ─────────────────────────────────────────────────────────────────────────────
void dumpRtcDiagnostics() {
  auto out = [](const String& s) { Serial.println(s); BT.println(s); };

  out("---- RTC DIAG ----");
  out("millis since boot: " + String(millis()));

  // Raw register dump (PCF8563 registers 0x00..0x0F)
  Wire.beginTransmission(0x51);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission(false) != 0) {
    out("PCF8563 not responding on 0x51 — bus / wiring / power problem.");
    out("---- END RTC DIAG ----");
    return;
  }
  Wire.requestFrom((uint8_t)0x51, (uint8_t)16);
  uint8_t regs[16] = {0};
  for (int i = 0; i < 16 && Wire.available(); i++) regs[i] = Wire.read();

  String hex;
  for (int i = 0; i < 16; i++) {
    if (regs[i] < 0x10) hex += "0";
    hex += String(regs[i], HEX) + " ";
  }
  out("regs 00-0F: " + hex);

  bool vl = (regs[0x02] & 0x80);
  out("VL bit (reg 0x02 b7): " + String(vl ? "SET — oscillator stopped at some point" : "clear"));

  // RTClib readback
  RTC_PCF8563 rtc;
  if (rtc.begin()) {
    DateTime now = rtc.now();
    char buf[32];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    out(String("RTC time: ") + buf);
    if (now.year() < 2020 || now.year() > 2040) {
      out("  -> year out of range; time invalid.");
    }
  } else {
    out("rtc.begin() failed");
  }
  out("---- END RTC DIAG ----");
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
      } else if (inputString == "RTC?" || inputString == "RTCDIAG") {
        dumpRtcDiagnostics();
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
