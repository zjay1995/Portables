#include <Wire.h>
#include <RTClib.h> // Include RTClib for PCF8563 RTC

RTC_PCF8563 rtc; // Initialize the RTC object for PCF8563

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  Wire.begin();         // Start I2C communication

  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1); // Stop execution if RTC initialization fails
  }

  // Check if the RTC lost power, and if so, set the time to the current system time
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the current system time");

    // Set the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set the time to the compile time
  } else {
    Serial.println("RTC is running normally");
  }

  // Print confirmation of the set time
  DateTime now = rtc.now();
  Serial.println("Time initialized to system compile time!");
  printTime(now);
}

void loop() {
  DateTime now = rtc.now(); // Get the current time from the RTC

  // Print the current time every second
  printTime(now);
  delay(1000);
}

// Function to print the time in a readable format
void printTime(DateTime t) {
  Serial.print(t.year(), DEC);
  Serial.print('/');
  Serial.print(t.month(), DEC);
  Serial.print('/');
  Serial.print(t.day(), DEC);
  Serial.print(" ");
  Serial.print(t.hour(), DEC);
  Serial.print(':');
  Serial.print(t.minute(), DEC);
  Serial.print(':');
  Serial.println(t.second(), DEC);
}
