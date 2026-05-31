#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <SSD1306.h>
// Use your existing project files
#include "inc/Button.h"
#include "inc/AnalogSourceInput.h"

// ==========================================
// Hardware Objects
// ==========================================
SSD1306 display(0x3c, 23, 22);
Adafruit_ADS1115 ads1115;
Adafruit_INA219 ina219;
Adafruit_SHT4x sht4x = Adafruit_SHT4x();

// UART 4-in-1 Sensor Config
HardwareSerial sensorSerial(1);
const int S_RX = 16;
const int S_TX = 17;

// Logic Objects
Keyboard* keyboard;
ADS1115AnalogSourceInput* analogInput;

// State Variables
String lastButtonPressed = "NONE";
unsigned long lastButtonPressTime = 0; // Timer for clearing button text
bool g_sensorConnected = false;
unsigned long g_lastValidPacketTime = 0;

// Component Health Flags
bool g_ads_ok = false;
bool g_ina_ok = false;
bool g_sht_ok = false;

// Sensor Values
float g_o2 = 0.0, g_ch4 = 0.0, g_co = 0.0, g_h2s = 0.0;

// ==========================================
// Helper: I2C Address Pinger
// ==========================================
bool checkI2C(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// ==========================================
// System Health Check (Polls I2C bus)
// ==========================================
void checkSystemHealth() {
  g_ads_ok = checkI2C(0x48);
  g_ina_ok = checkI2C(0x40);
  g_sht_ok = checkI2C(0x44);
}

// ==========================================
// Helpers: ADS1115 Channel Readings
// ==========================================
uint16_t readAdsChannel(int channel) {
  const int numReadings = 4;
  long total = 0;
  const float multiplier = 0.125F; 

  for (int i = 0; i < numReadings; i++) {
    total += ads1115.readADC_SingleEnded(channel) * multiplier;
  }
  return total / numReadings;
}

// Wrappers for distinct calls
uint16_t getMiliVoltsCh1() { return readAdsChannel(1); }
uint16_t getMiliVoltsCh2() { return readAdsChannel(2); }
uint16_t getMiliVoltsCh3() { return readAdsChannel(3); }

// ==========================================
// Winsen 4-in-1 Parser (Non-Blocking)
// ==========================================
void handleSensorCommunication() {
  static unsigned long lastRequest = 0;
  unsigned long now = millis();

  // 1. Send Request 'R' every 500ms
  if (now - lastRequest > 500) {
    while (sensorSerial.available()) sensorSerial.read();
    sensorSerial.write('R');
    lastRequest = now;
  }

  // 2. Read & Parse
  if (sensorSerial.available() >= 11) {
    if (sensorSerial.read() == 0xFF) {
      uint8_t buf[10];
      sensorSerial.readBytes(buf, 10);

      uint8_t checksum = 0;
      for (int i = 0; i < 9; i++) checksum += buf[i];
      checksum = (uint8_t)(~checksum) + 1;

      if (checksum == buf[9]) {
        uint16_t co_raw  = (buf[1] << 8) | buf[2];
        uint16_t h2s_raw = (buf[3] << 8) | buf[4];
        uint16_t o2_raw  = (buf[5] << 8) | buf[6];
        uint16_t ch4_raw = (buf[7] << 8) | buf[8];

        if (co_raw == 0 && h2s_raw == 0 && o2_raw == 0 && ch4_raw == 0) {
          g_sensorConnected = false;
        } else {
          g_sensorConnected = true;
          g_lastValidPacketTime = millis();
          g_co  = co_raw / 1.0;
          g_h2s = h2s_raw / 1.0;
          g_o2  = o2_raw / 10.0;
          g_ch4 = ch4_raw / 1.0;
        }
      }
    }
  }

  if (millis() - g_lastValidPacketTime > 3000) {
    g_sensorConnected = false;
  }
}

// ==========================================
// Display Function
// ==========================================
void updateDisplay() {
  // Button Auto-Clear Logic
  if (millis() - lastButtonPressTime > 2000) {
    lastButtonPressed = "NONE";
  }

  // ADC Cycle logic: 0->1->2->3 every 2 seconds
  static int adcChannelToShow = 0;
  static unsigned long lastToggleTime = 0;
  
  if (millis() - lastToggleTime > 2000) {
    adcChannelToShow++;
    if (adcChannelToShow > 3) adcChannelToShow = 0;
    lastToggleTime = millis();
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  // --- Line 1: 4-in-1 Sensor Status ---
  if (g_sensorConnected) {
    display.drawString(0, 0, "4-in-1: OK");
    display.drawString(60, 0, "O2:" + String(g_o2, 1) + "%");
  } else {
    display.drawString(0, 0, "4-in-1: MISSING");
  }

  // --- Line 2: Component Health (I2C) ---
  String statusLine = "";
  statusLine += "PWR:" + String(g_ina_ok ? "OK" : "NO") + " ";
  statusLine += "ENV:" + String(g_sht_ok ? "OK" : "NO");
  display.drawString(0, 12, statusLine);

  // --- Line 3 (0,24): ADS1115 Cycling Channels ---
  if (g_ads_ok) {
    String label = "ADC A" + String(adcChannelToShow) + ": ";
    uint16_t val = 0;
    
    switch(adcChannelToShow) {
      case 0: val = analogInput->getMiliVolts(); break; // Channel 0
      case 1: val = getMiliVoltsCh1(); break;           // Channel 1
      case 2: val = getMiliVoltsCh2(); break;           // Channel 2
      case 3: val = getMiliVoltsCh3(); break;           // Channel 3
    }
    display.drawString(0, 24, label + String(val) + " mV");
  } else {
    display.drawString(0, 24, "ADC: NOT FOUND");
  }

  // --- Line 4: Values (if valid) ---
  if (g_ina_ok) {
    display.drawString(65, 36, "Bat: " + String(analogInput->getMiliVolts_battery()) + " mV");
  } else {
    display.drawString(65, 36, "Bat: --");
  }

  // --- Line 5: Temp/Hum (if valid) ---
  if (g_sht_ok) {
    display.drawString(0, 36, "Temp: " + String(analogInput->getTemperature(), 1) + "C");
  } else {
    display.drawString(0, 36, "Temp: --");
  }

  // --- Line 6: Last Button ---
  display.drawString(0, 48, "Btn: " + lastButtonPressed);

  display.display();
}

// ==========================================
// Main Setup & Loop
// ==========================================
void setup() {
  Serial.begin(115200);

  // Init I2C
  Wire.begin(23, 22);

  // Display Init
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.drawString(0, 20, "Booting...");
  display.display();

  // Attempt to init sensors
  ads1115.begin();
  ina219.begin();
  sht4x.begin();

  // Logic Init
  analogInput = new ADS1115AnalogSourceInput(&ads1115, &ina219, &sht4x);

  // UART Init
  sensorSerial.begin(9600, SERIAL_8N1, S_RX, S_TX);

  // Keyboard Init
  keyboard = new Keyboard();
  
  // NOTE: We now capture timestamps on press events
  keyboard->addOnDownPressedFctor([]  { 
    lastButtonPressed = "DOWN"; 
    lastButtonPressTime = millis(); 
  });
  keyboard->addOnSPressedFctor([]     { 
    lastButtonPressed = "S (SELECT)"; 
    lastButtonPressTime = millis(); 
  });
  keyboard->addOnRightPressedFctor([] { 
    lastButtonPressed = "RIGHT"; 
    lastButtonPressTime = millis(); 
  });
  keyboard->addOnOnPressedFctor([]    { 
    lastButtonPressed = "ON"; 
    lastButtonPressTime = millis(); 
  });
}

void loop() {
  // 1. Poll Inputs
  ButtonPressDetector::handleTick();
  handleSensorCommunication();

  // 2. Poll System Health (every 1 second)
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 1000) {
    checkSystemHealth();
    lastHealthCheck = millis();
  }

  // 3. Update Display (every 200ms)
  static unsigned long lastDisp = 0;
  if (millis() - lastDisp > 200) {
    updateDisplay();
    lastDisp = millis();
  }
}
