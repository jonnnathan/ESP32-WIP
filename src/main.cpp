#include "neo6m.h"     // GPS functionality
#include "sht30.h"
#include "OLED.h"
#include "bmp180.h"
#include <Wire.h>
#include <math.h>

// Function Prototypes
void initBMP180();
void calibrateSeaLevel(float knownAltM);
void readBMP180Data();
void displaySensorData();
void displayCombinedSensorData();

static float seaLevelPressureFrom(float pressurePa, float knownAltitudeM);

// Sensor Classes
OLED display;
BMP180 bmp180;

// ====== Second I2C bus (Heltec/MakerFocus V3 uses Wire1 on custom pins) ======
TwoWire I2C_second = TwoWire(1);
#define SDA2_PIN 19
#define SCL2_PIN 20

// BMP180 Related Global variables
bool  bmp180_ready = false;
float temperatureF = 0.0f;   // Fahrenheit
float pressurePa   = 0.0f;   // Pascals
float altitudeStd  = 0.0f;   // meters (Std Atmosphere 101325 Pa)
float altitudeCal  = 0.0f;   // meters (Calibrated with local SLP)
bool  calibrated   = false;
static const float SEA_LEVEL_DEFAULT_PA = 101325.0f; // Standard Atmosphere
float seaLevelPa = SEA_LEVEL_DEFAULT_PA;             // Set after calibration
unsigned long lastSensorRead  = 0;
const unsigned long sensorInterval = 1000; // ms

void setup() {
  Serial2.begin(9600, SERIAL_8N1,46,45);
  Serial.begin(115200);
  delay(1000);
  
  // Initialize GPS
  initGPS();
  
  // OLED
  display.init();
  
  // I2C (second bus)
  I2C_second.begin(SDA2_PIN, SCL2_PIN, 100000); // 100 kHz
  
  // BMP180
  initBMP180();
  
  // Calibrate according the current known altitude, WIP
  if (bmp180_ready) {
    calibrateSeaLevel(/*knownAltM=*/91.0f);
  }
}

void loop() {
  bool newGPSData = false;
  
  // Read sensor data periodically
  if (millis() - lastSensorRead >= sensorInterval) {
    readBMP180Data();
    lastSensorRead = millis();
  }
  
  // Process GPS data
  newGPSData = processGPSData();
  
  // Display combined data only when we have new GPS data
  if (newGPSData) {
    displayCombinedSensorData();
  }
  
  // Update display
  display.clearDisplay();
  displaySensorData();
  display.updateDisplay();
}

// ====== BMP180 Implementation ======
void initBMP180() {
  Serial.println(F("Initializing BMP180..."));
  if (bmp180.begin(&I2C_second, BMP180_OSS_ULTRAHIGHRES)) {
    bmp180_ready = true;
  } else {
    bmp180_ready = false;
  }
}

// Average a few pressure samples, compute sea-level pressure from known altitude
void calibrateSeaLevel(float knownAltM) {
  const int N = 8; // sample count for averaging
  float sumPa = 0.0f;
  for (int i = 0; i < N; ++i) {
    float p = bmp180.readPressure(); // Pa
    if (p > 10000.0f && p < 120000.0f) { // rough sanity
      sumPa += p;
    } else {
      // If a weird value appears, don't skew the average; repeat this sample
      --i;
    }
    delay(60);
  }
  float pNow = sumPa / N; // Pa
  seaLevelPa = seaLevelPressureFrom(pNow, knownAltM);
  calibrated = true;
}

// Rearranged barometric formula to compute SLP (QNH) from known altitude
static float seaLevelPressureFrom(float pressurePa, float knownAltitudeM) {
  // seaLevelPa = pressurePa / (1 - h/44330)^5.255
  return pressurePa / powf(1.0f - (knownAltitudeM / 44330.0f), 5.255f);
}

void readBMP180Data() {
  if (!bmp180_ready) return;
  // Temperature (sensor returns °C)
  float tempC = bmp180.readTemperature();
  temperatureF = (tempC * 9.0f / 5.0f) + 32.0f;
  // Pressure (Pa)
  pressurePa = bmp180.readPressure();
  // Altitude with standard sea-level pressure (101325 Pa)
  altitudeStd = bmp180.readAltitude(SEA_LEVEL_DEFAULT_PA);
  // Altitude
  if (calibrated) {
    altitudeCal = bmp180.readAltitude(seaLevelPa);
  } else {
    altitudeCal = altitudeStd; // until calibrated, just mirror std
  }
  // Serial debug
  //Serial.print(F("Temperature = "));  
  //Serial.print(temperatureF, 1); 
  //Serial.print(F("°F, "));
  //Serial.print(F("Pressure = "));  
  //Serial.print(pressurePa / 100.0f, 1); Serial.print(F("hPa, "));
  //Serial.print(F("Altitude = ")); 
  //Serial.print(altitudeCal, 1); Serial.println(F("m"));
}

void displaySensorData() {
  // Show temp and pressure
  display.drawString(0, 0,  "Temperature: " + String(temperatureF, 1) + "F");
  display.drawString(0, 10, "Pressure: " + String(pressurePa / 100.0f, 1) + "hPa");
  // Show known elevation (reference)
  display.drawString(0, 20, "Altitude: " + String(altitudeCal, 1) + "m");
  
  // Display GPS data using the GPS module functions
  if (isLocationValid()) {
    display.drawString(0, 30, "GPS: " + getFormattedCoordinates());
  } else {
    display.drawString(0, 30, "GPS: No Fix");
  }
  
  if (isDateTimeValid()) {
    display.drawString(0, 40, "Time: " + getFormattedTime24Hour());
    display.drawString(0, 50, "Date: " + getFormattedDate());
  } else {
    display.drawString(0, 40, "Time: INVALID");
  }
}

// Example function showing how to use GPS module functions
void exampleGPSUsage() {
  if (isLocationValid()) {
    Serial.print("Current position: ");
    Serial.println(getFormattedCoordinates());
    
    // Access individual coordinates
    double lat = getLatitude();
    double lng = getLongitude();
    Serial.print("Lat: "); Serial.print(lat, 6);
    Serial.print(", Lng: "); Serial.println(lng, 6);
  }
  
  if (isDateTimeValid()) {
    Serial.print("Current date: "); Serial.println(getFormattedDate());
    Serial.print("Current time (12h): "); Serial.println(getFormattedTime12Hour());
    Serial.print("Current time (24h): "); Serial.println(getFormattedTime24Hour());
    
    // Access individual time components
    Serial.print("Hour: "); Serial.println(getGPSHour());
    Serial.print("Minute: "); Serial.println(getGPSMinute());
    Serial.print("Second: "); Serial.println(getGPSSecond());
  }
}

void displayCombinedSensorData() {
  // Print sensor data first (without newline)
  Serial.print(F("Temperature = "));  
  Serial.print(temperatureF, 1); 
  Serial.print(F("°F, "));
  Serial.print(F("Pressure = "));  
  Serial.print(pressurePa / 100.0f, 1); Serial.print(F("hPa, "));
  Serial.print(F("Altitude = ")); 
  Serial.print(altitudeCal, 1); Serial.print(F("m, "));
  
  // Then print GPS data (also without newline)
  displayGPSInfo();
  
  // End the line
  Serial.println();
}