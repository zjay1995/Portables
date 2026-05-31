
/*
   Release Log
   Save the selected gas index to the EEPROM memory. 3/4/2023
   Range Selection Added but not implemented in the calculation. 2/28/2023
   Bluetooth Datalogging added and implemented.
*/

/* Parts List:
   Huzzah32
   ADS1115
*/

#include <Adafruit_ADS1015.h>
#include <SSD1306.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <vector>

#include "inc/TimeSync.h"
#include "inc/GasManager.h"
#include "inc/Button.h"
#include "inc/Menu.h"
#include "inc/MenuRenderer.h"
#include "inc/WebServer.h"
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



using namespace std;

#define USE_SSD1306_DISPLAY

//Adafruit_SHT4x sht4 = Adafruit_SHT4x();

#define MAX_SCCM 5000

#define wifi_ssid "22CDPro"
#define wifi_password "00525508"

GasManager g_gasManager(-2.054456771, 1, 0, 0, 0, 0, 0, 0, 0, 0);

WebServer g_webServer;

CompositeMenu* g_mainMenu = nullptr;

Adafruit_ADS1115 ads1115;

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
DataLogger g_dataLogger;

TimeSync g_timeSync;

const char* ssid     = "ESP32-Access-Point";
const char* password = "Polaroid1";

volatile bool CALIBRATION_MODE = false;

unsigned long buttonPressStartTime = 0;
bool isButtonBeingPressed = false;

void setupWiFi() {
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  g_webServer.begin(80);
}

void IRAM_ATTR dummyTouchISR() {}

void setup() {
  Serial.begin(115200);
  Serial.println("PID M40 v20240622");
  // DEEP-SLEEP init
  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
  //esp_sleep_enable_ext1_wakeup(0x8004, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);
  // ADC
  ads1115.begin();
  ads1115.setGain(GAIN_TWOTHIRDS);

  AnalogSourceInput* ads1115AnalogSourceInput = new ADS1115AnalogSourceInput(&ads1115);

  DataSource* dataSource = new DataSource(&g_gasManager, ads1115AnalogSourceInput);

  // Gas Manager
  g_gasManager.setConfigurationManager(&g_configurationManager);

  g_gasManager.addGas(Gas("CO2", 1.0));
  g_gasManager.addGas(Gas("H2S", 1.0));
  g_gasManager.addGas(Gas("CO", 1.0));
  g_gasManager.addGas(Gas("O2", 5.73));
  g_gasManager.addGas(Gas("H2", 6.84));
  g_gasManager.addGas(Gas("ArCH4", 0.85));
  //
  /// Menus
  //

#ifdef USE_SSD1306_DISPLAY

  // Display
  display.init();
  display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_16);
  display.drawXbm(0, 0, 128, 60, image_arr);
  display.display();
  delay(200);

  MenuRenderer* gasMenuRenderer = new SSD1306GasMenuRenderer(&display);
  MenuRenderer* runMenuRenderer = new SSD1306RunMenuRenderer(&display, dataSource, &g_gasManager, &g_alarm, &g_lowalarm, &g_range, &g_calvalue, &g_outport);
  MenuRenderer* sleepTimerMenuRenderer = new SSD1306SleepTimerMenuRenderer(&display, &g_sleepTimer);
  MenuRenderer* rangeMenuRenderer = new SSD1306RangeMenuRenderer(&display, &g_range);
  MenuRenderer* alarmMenuRenderer = new SSD1306AlarmMenuRenderer(&display, &g_alarm);
  MenuRenderer* lowalarmMenuRenderer = new SSD1306LowalarmMenuRenderer(&display, &g_lowalarm);

  MenuRenderer* hourMenuRenderer = new SSD1306HourMenuRenderer(&display, &g_hour);
  MenuRenderer* minuteMenuRenderer = new SSD1306MinuteMenuRenderer(&display, &g_minute);
  MenuRenderer* calvalueMenuRenderer = new SSD1306CalvalueMenuRenderer(&display, &g_calvalue);
  MenuRenderer* outportMenuRenderer = new SSD1306OutportMenuRenderer(&display, &g_outport);
  MenuRenderer* loggingsetMenuRenderer = new SSD1306LoggingsetMenuRenderer(&display, &g_loggingset);

  MenuRenderer* flashLoggerMenuRenderer = new SSD1306FlashLoggerMenuRenderer(&display, &g_dataLogger);
  MenuRenderer* wifiDumpMenuRenderer = new SSD1306WiFiDumpMenuRenderer(&display, &g_dataLogger);
  MenuRenderer* wifiRealTimeDumpMenuRenderer = new SSD1306WiFiRealTimeDumpMenuRenderer(&display, &g_dataLogger);
  MenuRenderer* NTPSyncMenuRenderer = new SSD1306NTPSyncMenuRenderer(&display, &g_timeSync);
  MenuRenderer* showTimeMenuRenderer = new SSD1306ShowTimeMenuRenderer(&display);
  MenuRenderer* CalStartMenuRenderer = new SSD1306CalStartMenuRenderer(&display);

  MenuRenderer* ZEROMenuRenderer = new SSD1306ZEROMenuRenderer(&display, dataSource, &g_gasManager);
  MenuRenderer* CalGasMenuRenderer = new SSD1306CalGasMenuRenderer(&display, dataSource, &g_calvalue, &g_gasManager);
  MenuRenderer* CalResMenuRenderer = new SSD1306CalResMenuRenderer(&display, &g_gasManager);

#endif

  // Run Menus
  vector<Menu*> runMenus;

  runMenus.push_back(new RunMenuItem(" ", "RUN", 0, &g_gasManager, runMenuRenderer));
  runMenus.push_back(new RunMenuItem("L", "RUN", 1, &g_gasManager, runMenuRenderer));
  runMenus.push_back(new RunMenuItem("P", "RUN", 1, &g_gasManager, runMenuRenderer));

  CompositeMenu* runMenu = new CompositeMenu("RUN", "Main Menu", runMenus);

  // Gas Menus
  vector<Menu*> gasMenus;


  gasMenus.push_back(new GasMenuItem("CO2", "LIBRARY",  0, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("H2S", "LIBRARY", 1, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("CO", "LIBRARY", 2, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("O2", "LIBRARY",  3, &g_gasManager, gasMenuRenderer));
  // gasMenus.push_back(new GasMenuItem("DET 5", "LIBRARY", 4, &g_gasManager, gasMenuRenderer));
  //  gasMenus.push_back(new GasMenuItem("DET 6", "LIBRARY", 5, &g_gasManager, gasMenuRenderer));

  CompositeMenu* libraryMenu = new CompositeMenu("LIBRARY", "Main Menu" , gasMenus);

  // Timer Menus
  vector<Menu*> sleepTimerMenus;
  sleepTimerMenus.push_back(new SleepTimerMenuItem("5", "TIMER",  0, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("60", "TIMER", 1, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("120", "TIMER", 2, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("360", "TIMER", 3, &g_sleepTimer, sleepTimerMenuRenderer));
  sleepTimerMenus.push_back(new SleepTimerMenuItem("CONTINUOUS", "TIMER", 4, &g_sleepTimer, sleepTimerMenuRenderer));

  CompositeMenu* timerMenu = new CompositeMenu("TIMER", "Main Menu" , sleepTimerMenus);


  // Range Menus
  vector<Menu*> rangeMenus;
  rangeMenus.push_back(new RangeMenuItem("50 ppm", "Range", 0, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("100 ppm", "Range", 1, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("200 ppm", "Range", 2, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("500 ppm", "Range", 3, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("1000 ppm", "Range", 4, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("5000 ppm", "Range", 5, &g_range, rangeMenuRenderer));
  rangeMenus.push_back(new RangeMenuItem("10000 ppm", "Range", 6, &g_range, rangeMenuRenderer));

  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu" , rangeMenus);


  // Alarm Menus
  vector<Menu*> alarmMenus;
  for (int i = 0; i <= 61; i++) {
    int ppm = 475 + i * 25;
    String label = "High Alarm";
    if (ppm == 475) {
      alarmMenus.push_back(new AlarmMenuItem("Off", label, i, &g_alarm, alarmMenuRenderer));
    } else {
      alarmMenus.push_back(new AlarmMenuItem(String(ppm) + " ppm", label, i, &g_alarm, alarmMenuRenderer));
    }
  }

  CompositeMenu* alarmMenu = new CompositeMenu("Alarm", "Main Menu" , alarmMenus);


  // Low Alarm Menus
  vector<Menu*> lowalarmMenus;
  for (int i = 0; i <= 61; i++) {
    int ppm = -25 + i * 25;
    String label = "Low Alarm";
    if (ppm == -25) {
      lowalarmMenus.push_back(new LowalarmMenuItem("Off", label, i, &g_lowalarm, lowalarmMenuRenderer));
    } else {
      lowalarmMenus.push_back(new LowalarmMenuItem(String(ppm) + " ppm", label, i, &g_lowalarm, lowalarmMenuRenderer));
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



  // calvalue Menus

  vector<Menu*> calvalueMenus;
  calvalueMenus.push_back(new CalvalueMenuItem(String(5000) + " ppm", "Cal Gas", 0, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem(String(6000) + " ppm", "Cal Gas", 1, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem(String(7000) + " ppm", "Cal Gas", 2, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem(String(8000) + " ppm", "Cal Gas", 3, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem(String(9000) + " ppm", "Cal Gas", 4, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem(String(10000) + " ppm", "Cal Gas", 5, &g_calvalue, calvalueMenuRenderer));

  CompositeMenu* calvalueMenu = new CompositeMenu("Calvalue", "Main Menu" , calvalueMenus);


  // outport menus
  vector<Menu*> outportMenus;
  outportMenus.push_back(new OutportMenuItem("Off", " Gas", 0, &g_outport, outportMenuRenderer));

  outportMenus.push_back(new OutportMenuItem("RS232", " Gas", 1, &g_outport, outportMenuRenderer));
  outportMenus.push_back(new OutportMenuItem("USB", " Gas", 2, &g_outport, outportMenuRenderer));
  outportMenus.push_back(new OutportMenuItem("BT", " Gas", 3, &g_outport, outportMenuRenderer));


  CompositeMenu* outportMenu = new CompositeMenu("Outport", "Main Menu" , outportMenus);



  // loggingset menus
  vector<Menu*> loggingsetMenus;
  loggingsetMenus.push_back(new LoggingsetMenuItem("Off", " Gas", 0, &g_loggingset, loggingsetMenuRenderer));

  loggingsetMenus.push_back(new LoggingsetMenuItem("Auto-5sec", " Gas", 1, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("Auto-30sec", " Gas", 2, &g_loggingset, loggingsetMenuRenderer));
  loggingsetMenus.push_back(new LoggingsetMenuItem("P", " Gas", 3, &g_loggingset, loggingsetMenuRenderer));


  CompositeMenu* loggingsetMenu = new CompositeMenu("Loggingset", "Main Menu" , loggingsetMenus);


  // DataLogger Menus
  vector<Menu*> dataLoggerMenus;

  dataLoggerMenus.push_back(new DataLoggerFlashStoreMenuItem("FLASH LOGGER", "DATALOGGER",  &g_dataLogger, flashLoggerMenuRenderer));
  dataLoggerMenus.push_back(new WiFiDumpMenuItem("WIFI DUMP", "DATALOGGER",           &g_dataLogger, wifiDumpMenuRenderer));
  dataLoggerMenus.push_back(new WiFiRealTimeDumpMenuItem("WIFI REAL-TIME DUMP", "DATALOGGER", &g_dataLogger, wifiRealTimeDumpMenuRenderer));


  CompositeMenu* dataLoggerMenu = new CompositeMenu("DATALOGGER", "Main Menu" , dataLoggerMenus);

  // DateTime menu
  vector<Menu*> dateTimeMenus;

  dateTimeMenus.push_back(new NTPSyncMenuItem("Time Sync", "DATETIME", &g_timeSync, NTPSyncMenuRenderer));
  dateTimeMenus.push_back(new ShowTimeMenuItem("Current DateTime", "DATETIME", showTimeMenuRenderer));

  CompositeMenu* dateTimeMenu = new CompositeMenu("DATETIME", "Main Menu" , dateTimeMenus);


  // Cal zero menu
  vector<Menu*> calMenus;
  calMenus.push_back(new CalMenuItemZero("Zero", dataSource, &g_gasManager, "ZeroGas", ZEROMenuRenderer));
  CompositeMenu* calMenu = new CompositeMenu("CALIBRATION Zero", "Main Menu", calMenus);

  // Cal gas menu
  vector<Menu*> calgasMenus;
  calgasMenus.push_back(new CalMenuItemCalGas("Cal Gas", dataSource, &g_gasManager, "CalGas", CalGasMenuRenderer));
  CompositeMenu* calgasMenu = new CompositeMenu("CALIBRATION Calgas", "Main Menu", calgasMenus);


  ////////////////////////////////////
  vector<Menu*> horizontalMenus;

  horizontalMenus.push_back(runMenu);


  horizontalMenus.push_back(calMenu);
  horizontalMenus.push_back(calvalueMenu);
  horizontalMenus.push_back(calgasMenu);
  horizontalMenus.push_back(libraryMenu);

  //horizontalMenus.push_back(dateTimeMenu);
  horizontalMenus.push_back(alarmMenu);
  horizontalMenus.push_back(lowalarmMenu);
  horizontalMenus.push_back(loggingsetMenu);

  horizontalMenus.push_back(outportMenu);

  horizontalMenus.push_back(hourMenu);
  horizontalMenus.push_back(minuteMenu);
  horizontalMenus.push_back(rangeMenu);


  Serial.println("horizontal menu " + String(horizontalMenus.size()));
  CompositeMenu* verticalMenu = new CompositeMenu("Main Menu", "", horizontalMenus);

  g_mainMenu = verticalMenu;

  g_mainMenu->print();

  setupButtons();

  g_webServer.init(&g_gasManager);
  //g_sleepTimer.init(&g_configurationManager);
  //g_range.init(&g_gasManager);

  g_dataLogger.init(dataSource, &g_gasManager);

  g_webServer.addParamChangeListener((ParamChangeListener*)&g_configurationManager);
  g_webServer.addParamChangeListener((ParamChangeListener*)&g_gasManager);

  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_timeSync);
  g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_gasManager);
  //g_configurationManager.addParamChangeListener((ParamChangeListener*)&g_range);

  g_configurationManager.addParamChangeListener((ParamChangeListener*)g_dataLogger.getMqttFlashPublisher());
  g_configurationManager.addParamChangeListener((ParamChangeListener*)g_dataLogger.getMqttRealTimePublisher());

  g_configurationManager.init();
  Serial.println("g_configManager.init finished.");
  g_configurationManager.loadFromEEPROM();
  Serial.println("LoadFromEEPROM Finished.");
  g_timeSync.initTimeFromRTC();
  Serial.println("initTimeFromRTC Finished.");
  int range = EEPROM.read(72);
  g_range.selectRangeByIndex(range);
  int alarm = EEPROM.read(76);
  g_alarm.selectAlarmByIndex(alarm);
  int lowalarm = EEPROM.read(92);
  g_lowalarm.selectLowalarmByIndex(lowalarm);
  Serial.println("Setup() finished.");
}

void setupButtons()
{

  Keyboard* keyboard = new Keyboard();

  keyboard->addOnDownPressedFctor([] {

    g_sleepTimer.resetIdleCounter();


    Serial.println("PRESS DOWWWNNNN");
    g_mainMenu->print();
    g_mainMenu->moveToNext();

  });
  keyboard->addOnSPressedFctor([] {

    g_sleepTimer.resetIdleCounter();


    Serial.println("PRESS S");
    g_mainMenu->action();

    g_timeSync.initTimeFromRTC();

  });
  keyboard->addOnRightPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS RIGHT");

    if (g_mainMenu->getCurrentIndex() == 0) {
      g_mainMenu->setCurrentMenu(4);
    } else{
      ((CompositeMenu*)g_mainMenu->getCurrentMenu())->moveToNext();
    }

  });
  keyboard->addOnOnPressedFctor([] {

    g_sleepTimer.resetIdleCounter();
    Serial.println("PRESS Mode");

    if (g_mainMenu->getCurrentIndex() == 0) {
      g_mainMenu->setCurrentMenu(11);
    } else if (g_mainMenu->getCurrentIndex() == 11) {
      g_mainMenu->setCurrentMenu(0);
    }

  });

  keyboard->addOnCalibrationComboPressedFctor([] {

    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();

    Serial.println("SleepTimer deep_sleep!");
    digitalWrite(26, LOW);
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



void loop()
{

  ButtonPressDetector::handleTick();

  g_sleepTimer.handleTick();
  g_dataLogger.handleTick();

  g_mainMenu->render();

  delay(10);
}
