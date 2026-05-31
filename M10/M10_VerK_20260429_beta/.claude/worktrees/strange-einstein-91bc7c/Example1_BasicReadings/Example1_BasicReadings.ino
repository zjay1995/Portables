/*
  Reading particulate matter data from the Panasonic SN-GCJA5
  This example outputs mass densities (PM1.0, PM2.5, PM10) and particle counts for particles
  sized 0.5um to 10um. Useful for monitoring air quality.

  What is particulate matter (PM)?
  PM refers to tiny particles in the air that can harm human health.
  More details: https://www.epa.gov/pm-pollution/particulate-matter-pm-basics

  Hardware Connections:
  - SN-GCJA5 VCC to ESP32 5V
  - SN-GCJA5 GND to ESP32 GND
  - SN-GCJA5 SDA (blue wire) to ESP32 GPIO21
  - SN-GCJA5 SCL (yellow wire) to ESP32 GPIO22

  Open the Serial Monitor at 115200 baud to view the data.
*/

#include <Wire.h>
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h"

SFE_PARTICLE_SENSOR myAirSensor;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Panasonic SN-GCJA5 Example"));

  Wire.begin();

  if (myAirSensor.begin() == false) {
    Serial.println("The particle sensor did not respond. Please check wiring. Freezing...");
    while (1); // Halt the program
  }

  Serial.println("Sensor initialized successfully!");
  Serial.println("Output Format:");
  Serial.println("PM1.0 (µg/m³), PM2.5 (µg/m³), PM10 (µg/m³), Particle Counts (0.5µm, 1.0µm, 2.5µm, 5.0µm, 7.5µm, 10µm)");
}

void loop() {
  // Read and print mass density (PM) values
  float pm1_0 = myAirSensor.getPM1_0();
  float pm2_5 = myAirSensor.getPM2_5();
  float pm10 = myAirSensor.getPM10();
  
  Serial.print("Mass Density (µg/m³): ");
  Serial.print("PM1.0: "); Serial.print(pm1_0, 2); Serial.print(", ");
  Serial.print("PM2.5: "); Serial.print(pm2_5, 2); Serial.print(", ");
  Serial.print("PM10: "); Serial.print(pm10, 2);
  Serial.println();

  // Read and print particle counts
  unsigned int pc0_5 = myAirSensor.getPC0_5();
  unsigned int pc1_0 = myAirSensor.getPC1_0();
  unsigned int pc2_5 = myAirSensor.getPC2_5();
  unsigned int pc5_0 = myAirSensor.getPC5_0();
  unsigned int pc7_5 = myAirSensor.getPC7_5();
  unsigned int pc10 = myAirSensor.getPC10();

  Serial.println("Particle Counts (# per 0.1L):");
  Serial.print("0.5µm: "); Serial.print(pc0_5); Serial.print(", ");
  Serial.print("1.0µm: "); Serial.print(pc1_0); Serial.print(", ");
  Serial.print("2.5µm: "); Serial.print(pc2_5); Serial.print(", ");
  Serial.print("5.0µm: "); Serial.print(pc5_0); Serial.print(", ");
  Serial.print("7.5µm: "); Serial.print(pc7_5); Serial.print(", ");
  Serial.print("10µm: "); Serial.print(pc10);
  Serial.println();

  // Wait 1 second before the next reading
  delay(1000);
}
