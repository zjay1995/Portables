
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
#include <Arduino.h>
#include <HardwareSerial.h>

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


#define GAS_INDEX_CO 0
#define GAS_INDEX_H2S 1
#define GAS_INDEX_O2 2
#define GAS_INDEX_CH4 3


GasManager g_gasManager(-2.054456771, 1, 0, 0, 0, 0, 0, 0, 0, 0);

WebServer g_webServer;

CompositeMenu* g_mainMenu = nullptr;

Adafruit_ADS1115 ads1115;
Adafruit_INA219 ina219;      // Add this line to DEFINE the global instance

#ifdef USE_SSD1306_DISPLAY
SSD1306 display(0x3c, 21, 22);
#endif

// Initialize the HardwareSerial object for UART communication with the sensor

HardwareSerial sensorSerial(1);

const int rxPin = 16; // RX pin for ESP32 (connected to TX of sensor)
const int txPin = 17; // TX pin for ESP32 (connected to RX of sensor)

// Create an instance of the UARTAnalogSourceInput class
UARTAnalogSourceInput uartSourceInput;


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

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  } else {
    Serial.println("INA219 initialized");
  }

  pinMode(25, OUTPUT);
  digitalWrite(25, HIGH);
  //esp_sleep_enable_ext1_wakeup(0x8004, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, LOW);

  // ADC
  ads1115.begin();
  ads1115.setGain(GAIN_TWOTHIRDS);

  // Initialize the sensor serial port
  sensorSerial.begin(9600, SERIAL_8N1, rxPin, txPin);

  AnalogSourceInput* uartAnalogSourceInput = new UARTAnalogSourceInput();

  DataSource* dataSource = new DataSource(&g_gasManager, uartAnalogSourceInput);

  // Gas Manager
  g_gasManager.setConfigurationManager(&g_configurationManager);

  g_gasManager.addGas(Gas("CO", 1.0));
  g_gasManager.addGas(Gas("H2S", 1.0));
  g_gasManager.addGas(Gas("O2", 1.0));
  g_gasManager.addGas(Gas("CH4", 5.73));
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
  MenuRenderer* rangeMenuRenderer = new SSD1306RangeMenuRenderer(&display, &g_range, dataSource, &g_gasManager);
  MenuRenderer* alarmMenuRenderer = new SSD1306AlarmMenuRenderer(&display, &g_alarm, dataSource, &g_gasManager);
  MenuRenderer* lowalarmMenuRenderer = new SSD1306LowalarmMenuRenderer(&display, &g_lowalarm, dataSource, &g_gasManager);

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


  gasMenus.push_back(new GasMenuItem("CO", "LIBRARY",  0, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("H2S", "LIBRARY", 1, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("O2", "LIBRARY", 2, &g_gasManager, gasMenuRenderer));
  gasMenus.push_back(new GasMenuItem("CH4", "LIBRARY",  3, &g_gasManager, gasMenuRenderer));
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


// Range Menu Setup in setup()
  vector<Menu*> rangeValueItems;
  // Create 15 items to cover the longest array (ppm)
  for (int i = 0; i < 15; i++) {
    // Label is placeholder; Renderer uses getRangeValueByIndex(gasIndex, i)
    rangeValueItems.push_back(new RangeMenuItem("Select", "Range", i, &g_range, &g_gasManager, rangeMenuRenderer));
  }
  CompositeMenu* rangeMenu = new CompositeMenu("Range", "Main Menu", rangeValueItems);

  
  
  // High Alarm
  vector<Menu*> alarmValueItems;
  for (int i = 0; i < 62; i++) {
    int ppmValue = g_alarm.getAlarmValueByIndex(i);
    // Use "Off" if the value is 0, otherwise add the units
    String label = (ppmValue == 0) ? "Off" : String(ppmValue) + " ppm";

    alarmValueItems.push_back(new AlarmMenuItem(label, "High Alarm", i, &g_alarm, &g_gasManager, alarmMenuRenderer));
  }
  CompositeMenu* alarmMenu = new CompositeMenu("High Alarm", "Main Menu", alarmValueItems);


  //Low Alarm Menu
  vector<Menu*> lowAlarmValueItems;
  for (int i = 0; i < 62; i++) {
    int ppmValue = g_lowalarm.getLowAlarmValueByIndex(i);
    String label = (ppmValue == 0) ? "Off" : String(ppmValue) + " ppm";

    lowAlarmValueItems.push_back(new LowalarmMenuItem(label, "Low Alarm", i, &g_lowalarm, &g_gasManager, lowalarmMenuRenderer));
  }
  CompositeMenu* lowAlarmMenu = new CompositeMenu("Low Alarm", "Main Menu", lowAlarmValueItems);

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

  // O2 Reference
  calvalueMenus.push_back(new CalvalueMenuItem("20.9 %VOL", "Cal Gas", 2, &g_calvalue, calvalueMenuRenderer));

  // Toxic Gas References (CO / H2S)
  calvalueMenus.push_back(new CalvalueMenuItem("10 ppm",    "Cal Gas", 0, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem("20 ppm",    "Cal Gas", 1, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem("25 ppm",    "Cal Gas", 3, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem("50 ppm",    "Cal Gas", 4, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem("100 ppm",   "Cal Gas", 5, &g_calvalue, calvalueMenuRenderer));
  calvalueMenus.push_back(new CalvalueMenuItem("200 ppm",   "Cal Gas", 6, &g_calvalue, calvalueMenuRenderer));

  // Combustible Reference (CH4)
  calvalueMenus.push_back(new CalvalueMenuItem("50 %LEL",   "Cal Gas", 4, &g_calvalue, calvalueMenuRenderer));

  CompositeMenu* calvalueMenu = new CompositeMenu("Calvalue", "Main Menu", calvalueMenus);


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
  // Pass &g_calvalue as the new 4th argument
  // Inside setup() around line 410
  calgasMenus.push_back(new CalMenuItemCalGas("Cal Gas", dataSource, &g_gasManager, &g_calvalue, "CalGas", CalGasMenuRenderer));
  CompositeMenu* calgasMenu = new CompositeMenu("CALIBRATION Calgas", "Main Menu", calgasMenus);

  ////////////////////////////////////
  vector<Menu*> horizontalMenus;

  horizontalMenus.push_back(runMenu);
  horizontalMenus.push_back(calMenu);
  horizontalMenus.push_back(calvalueMenu);
  horizontalMenus.push_back(calgasMenu);
  //horizontalMenus.push_back(calresMenu);
  horizontalMenus.push_back(libraryMenu);
  //horizontalMenus.push_back(dateTimeMenu);
  horizontalMenus.push_back(alarmMenu);
  horizontalMenus.push_back(lowAlarmMenu);
  horizontalMenus.push_back(loggingsetMenu);

  horizontalMenus.push_back(outportMenu);

  horizontalMenus.push_back(hourMenu);
  horizontalMenus.push_back(minuteMenu);
  horizontalMenus.push_back(rangeMenu);


  Serial.println("horizontal menu " + String(horizontalMenus.size()));
  CompositeMenu* verticalMenu = new CompositeMenu("Main Menu", "", horizontalMenus);

  g_mainMenu = verticalMenu;

 // g_mainMenu->print();

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
  g_configurationManager.loadFromEEPROM();

  g_timeSync.initTimeFromRTC();
  // FIX: Initialize Range settings for all 4 gases from EEPROM
  for (int i = 0; i < 4; i++) {
    // Read the range index stored in EEPROM (Offset 72 + 4 bytes per gas)
    int storedRangeIndex = EEPROM.readInt(72 + (i * 4));

    // Safety check: if EEPROM is empty/new, default to index 0
    if (storedRangeIndex < 0 || storedRangeIndex > 15) storedRangeIndex = 0;

    // Pass BOTH the gas index (i) and the choice (storedRangeIndex)
    g_range.selectRangeByIndex(i, storedRangeIndex);
  }
  for (int gasIndex = 0; gasIndex < 4; gasIndex++) {
    int alarmAddress = 96 + (gasIndex * 4);
    int alarm = EEPROM.read(alarmAddress);
    g_alarm.selectAlarmByIndex(gasIndex, alarm);
  }
  // FIX: Update low alarm initialization for all 4 gases
  for (int i = 0; i < 4; i++) {
    // Example: load index from EEPROM or set a default
    int alarmIdx = EEPROM.readInt(120 + (i * 4));
    g_lowalarm.selectLowalarmByIndex(i, alarmIdx);
  }
}

void setupButtons()
{

  Keyboard* keyboard = new Keyboard();

  keyboard->addOnDownPressedFctor([] {

    g_sleepTimer.resetIdleCounter();


    Serial.println("PRESS DOWWWNNNN");
    //g_mainMenu->print();
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



void loop()
{

  ButtonPressDetector::handleTick();

  g_sleepTimer.handleTick();
  g_dataLogger.handleTick();

  g_mainMenu->render();

  delay(10);
}
