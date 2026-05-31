#include <Adafruit_ADS1015.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SHT4x.h>
#include <TFT_eSPI.h> 
#include <Wire.h>
#include <vector>
#include "BluetoothSerial.h"

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

using namespace std;

#define USE_TFT_DISPLAY

#define MAX_SCCM 5000

GasManager g_gasManager(-2.054456771, 1, 0, 0, 0, 0, 0, 0, 0, 0);
BluetoothSerial BT;
CompositeMenu* g_mainMenu = nullptr;

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
DataLogger g_dataLogger;
TimeSync g_timeSync;

volatile bool CALIBRATION_MODE = false;
unsigned long buttonPressStartTime = 0;
bool isButtonBeingPressed = false;

void IRAM_ATTR dummyTouchISR() {}

void setup() {
  Serial.begin(115200);
  Serial.println("PID M40 TFT v2024 NO-WIFI");
  Wire.begin(23, 22); 

  BT.begin("ESP32 PID M40");
  BT.println("Bluetooth connection");
  Serial.println("The device started, now you can pair it with bluetooth!");

  delay(500);

  // DEEP-SLEEP init
  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);

  // Alarm Pin
  const int alarmPin = 32; 
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW);
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);

  // Initialize sensors
  ads1115.begin();
  ads1115.setGain(GAIN_TWOTHIRDS);
  ina219.begin();

  if (! sht4x.begin()) {
    Serial.println("Couldn't find SHT4x");
  }
  Serial.println("Found SHT4x sensor");

  AnalogSourceInput* ads1115AnalogSourceInput = new ADS1115AnalogSourceInput(&ads1115, &ina219, &sht4x);
  DataSource* dataSource = new DataSource(&g_gasManager, ads1115AnalogSourceInput);

  g_gasManager.setConfigurationManager(&g_configurationManager);
  g_gasManager.addGas(Gas("100", 1.0));
  g_gasManager.addGas(Gas("1000", 1.0));

  // --- TFT INIT ---
  tft.init();
  
  // CHANGED: 0 = Portrait (90 degrees from previous Landscape setting)
  tft.setRotation(0); 
  
  tft.fillScreen(TFT_BLACK);

  // Use TFT Renderers
  MenuRenderer* gasMenuRenderer = new TFTGasMenuRenderer(&tft);
  MenuRenderer* runMenuRenderer = new TFTRunMenuRenderer(&tft, dataSource, &g_gasManager, &g_alarm, &g_lowalarm, &g_range, &g_calvalue, &g_outport, &g_loggingset, &g_loggingtime);
  MenuRenderer* sleepTimerMenuRenderer = new TFTSleepTimerMenuRenderer(&tft, &g_sleepTimer);
  MenuRenderer* rangeMenuRenderer = new TFTRangeMenuRenderer(&tft, &g_range);
  MenuRenderer* alarmMenuRenderer = new TFTAlarmMenuRenderer(&tft, &g_alarm);
  MenuRenderer* lowalarmMenuRenderer = new TFTLowalarmMenuRenderer(&tft, &g_lowalarm);
  MenuRenderer* hourMenuRenderer = new TFTHourMenuRenderer(&tft, &g_hour);
  MenuRenderer* minuteMenuRenderer = new TFTMinuteMenuRenderer(&tft, &g_minute);
  MenuRenderer* calvalueMenuRenderer = new TFTCalvalueMenuRenderer(&tft, &g_calvalue);
  MenuRenderer* outportMenuRenderer = new TFTOutportMenuRenderer(&tft, &g_outport);
  MenuRenderer* loggingsetMenuRenderer = new TFTLoggingsetMenuRenderer(&tft, &g_loggingset);
  MenuRenderer* loggingtimeMenuRenderer = new TFTLoggingtimeMenuRenderer(&tft, &g_loggingtime);
  MenuRenderer* flashLoggerMenuRenderer = new TFTFlashLoggerMenuRenderer(&tft, &g_dataLogger);
  
  MenuRenderer* showTimeMenuRenderer = new TFTShowTimeMenuRenderer(&tft);
  MenuRenderer* CalStartMenuRenderer = new TFTCalStartMenuRenderer(&tft);
  MenuRenderer* ZEROMenuRenderer = new TFTZEROMenuRenderer(&tft, dataSource, &g_gasManager);
  MenuRenderer* CalGasMenuRenderer = new TFTCalGasMenuRenderer(&tft, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* CalResMenuRenderer = new TFTCalResMenuRenderer(&tft, dataSource, &g_calvalue, &g_gasManager);

  // --- MENU BUILDING ---
  vector<Menu*> runMenus;
  runMenus.push_back(new RunMenuItem(" ", "RUN", 0, &g_gasManager, dataSource, &g_loggingset, &g_range, &g_outport, runMenuRenderer));
  CompositeMenu* runMenu = new CompositeMenu("RUN", "Main Menu", runMenus);

  vector<Menu*> gasMenus;
  gasMenus.push_back(new GasMenuItem("100", "LIBRARY",  0, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("1000", "LIBRARY", 1, &g_gasManager, gasMenuRenderer));
  CompositeMenu* libraryMenu = new CompositeMenu("LIBRARY", "Main Menu" , gasMenus);

  vector<Menu*> sleepTimerMenus;
  sleepTimerMenus.push_back(new SleepTimerMenuItem("5", "TIMER",  0, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("60", "TIMER", 1, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("120", "TIMER", 2, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("360", "TIMER", 3, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("CONTINUOUS", "TIMER", 4, &g_sleepTimer, sleepTimerMenuRenderer));
  CompositeMenu* timerMenu = new CompositeMenu("TIMER", "Main Menu" , sleepTimerMenus);

  vector<Menu*> rangeMenus;
  rangeMenus.push_back(new RangeMenuItem("5000 ppm", "Range", 0, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("10000 ppm", "Range", 1, &g_range, rangeMenuRenderer));
  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu" , rangeMenus);

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

  vector<Menu*> outportMenus;
  for (int i = 0; i <= 49; ++i) {
    outportMenus.push_back(new OutportMenuItem("User " + String(i), " USER ID", i, &g_outport, outportMenuRenderer));
  }
  CompositeMenu* outportMenu = new CompositeMenu("USER ID", "Main Menu", outportMenus);

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

  vector<Menu*> dataLoggerMenus;
  dataLoggerMenus.push_back(new DataLoggerFlashStoreMenuItem("FLASH LOGGER", "DATALOGGER",  &g_dataLogger, flashLoggerMenuRenderer));
  CompositeMenu* dataLoggerMenu = new CompositeMenu("DATALOGGER", "Main Menu" , dataLoggerMenus);

  vector<Menu*> dateTimeMenus;
  dateTimeMenus.push_back(new ShowTimeMenuItem("Current DateTime", "DATETIME", showTimeMenuRenderer));
  CompositeMenu* dateTimeMenu = new CompositeMenu("DATETIME", "Main Menu" , dateTimeMenus);

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
  vector<Menu*> horizontalMenus;
  horizontalMenus.push_back(runMenu);
  horizontalMenus.push_back(calMenu);
  horizontalMenus.push_back(calvalueMenu);
  horizontalMenus.push_back(calgasMenu);
  horizontalMenus.push_back(calresMenu);
  horizontalMenus.push_back(libraryMenu);
  horizontalMenus.push_back(alarmMenu);
  horizontalMenus.push_back(lowalarmMenu);
  horizontalMenus.push_back(loggingsetMenu);
  horizontalMenus.push_back(loggingtimeMenu);
  horizontalMenus.push_back(hourMenu);
  horizontalMenus.push_back(minuteMenu);
  horizontalMenus.push_back(dataLoggerMenu);

  Serial.println("horizontal menu " + String(horizontalMenus.size()));
  CompositeMenu* verticalMenu = new CompositeMenu("Main Menu", "", horizontalMenus);
  g_mainMenu = verticalMenu;
  g_mainMenu->print();

  setupButtons();
  g_dataLogger.init(dataSource, &g_gasManager);

  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_timeSync);
  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_gasManager);

  g_configurationManager.init();
  g_configurationManager.loadFromEEPROM();
  g_timeSync.initTimeFromRTC();

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
}

void setupButtons() {
  Keyboard* keyboard = new Keyboard();
  keyboard->addOnDownPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    tft.fillScreen(TFT_BLACK); 
    Serial.println("PRESS DOWWWNNNN");
    g_mainMenu->print();
    g_mainMenu->moveToNext();
  });
  keyboard->addOnSPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    tft.fillScreen(TFT_BLACK); 
    Serial.println("PRESS S");
    g_mainMenu->action();
    g_timeSync.initTimeFromRTC();
  });
  keyboard->addOnRightPressedFctor([] {
    g_sleepTimer.resetIdleCounter();
    tft.fillScreen(TFT_BLACK); 
    Serial.println("PRESS RIGHT");
    if (g_mainMenu->getCurrentIndex() == 0) {
      g_mainMenu->setCurrentMenu(5);
    } else{
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
  Serial.println("log.txt has been cleared");
}

void loop() {
  ButtonPressDetector::handleTick();
  g_sleepTimer.handleTick();
  g_dataLogger.handleTick();
  g_mainMenu->render();
  listenForCode();
  delay(10);
}
