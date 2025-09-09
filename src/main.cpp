/**/
#include "neo6m.h"     // GPS functionality
#include "sht30.h"     // Temperature and humidity sensor
#include "OLED.h"
#include "bmp180.h"
#include <Wire.h>
#include <math.h>

// Function Prototypes
void initBMP180();
void initSHT30();
void calibrateSeaLevel(float knownAltM);
void readBMP180Data();
void readSHT30Data();
void displaySensorData();
void displayCombinedSensorData();
void updateHybridAltimeter();
float getHybridAltitude();
String getAltitudeSource();

static float seaLevelPressureFrom(float pressurePa, float knownAltitudeM);

// Sensor Classes
OLED display;
BMP180 bmp180;
SHT30 sht30;

// ====== Second I2C bus (Heltec/MakerFocus V3 uses Wire1 on custom pins) ======
TwoWire I2C_second = TwoWire(1);
#define SDA2_PIN 7
#define SCL2_PIN 20

// BMP180 Related Global variables
bool  bmp180_ready = false;
float temperatureF = 0.0f;   // Fahrenheit from BMP180
float pressurePa   = 0.0f;   // Pascals
float altitudeStd  = 0.0f;   // meters (Std Atmosphere 101325 Pa)
float altitudeCal  = 0.0f;   // meters (Calibrated with local SLP)
bool  calibrated   = false;
static const float SEA_LEVEL_DEFAULT_PA = 101325.0f; // Standard Atmosphere
float seaLevelPa = SEA_LEVEL_DEFAULT_PA;             // Set after calibration
unsigned long lastSensorRead  = 0;
const unsigned long sensorInterval = 1000; // ms

// SHT30 Related Global variables
bool sht30_ready = false;
float humidity = 0.0f;        // %RH from SHT30
float temperatureC_SHT = 0.0f; // Celsius from SHT30
float temperatureF_SHT = 0.0f; // Fahrenheit from SHT30

// Hybrid Altimeter Variables
bool autoCalibrated = false;
unsigned long lastGPSCalibration = 0;
const unsigned long RECALIBRATION_INTERVAL = 300000; // 5 minutes

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
  
  // SHT30
  initSHT30();
  
  Serial.println("System initialized. GPS will auto-calibrate BMP180 when available.");
}

void loop() {
  bool newGPSData = false;
  
  // Read sensor data periodically
  if (millis() - lastSensorRead >= sensorInterval) {
    readBMP180Data();
    readSHT30Data();
    lastSensorRead = millis();
  }
  
  // Process GPS data
  newGPSData = processGPSData();
  
  // Update hybrid altimeter (auto-calibration)
  updateHybridAltimeter();
  
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
    Serial.println(F("BMP180 initialized successfully"));
  } else {
    bmp180_ready = false;
    Serial.println(F("BMP180 initialization failed"));
  }
}

// ====== SHT30 Implementation ======
void initSHT30() {
  Serial.println(F("Initializing SHT30..."));
  if (sht30.begin(&I2C_second)) {
    sht30_ready = true;
    Serial.println(F("SHT30 initialized successfully"));
  } else {
    sht30_ready = false;
    Serial.println(F("SHT30 initialization failed"));
  }
}

void readSHT30Data() {
  if (!sht30_ready) return;
  
  if (sht30.read()) {
    temperatureC_SHT = sht30.getTemperature();
    temperatureF_SHT = (temperatureC_SHT * 9.0f / 5.0f) + 32.0f;
    humidity = sht30.getHumidity();
  } else {
    Serial.println(F("SHT30 read failed"));
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
  Serial.println("BMP180 calibrated to altitude: " + String(knownAltM) + "m, SLP: " + String(seaLevelPa/100.0f) + "hPa");
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
}

// ====== Hybrid Altimeter Implementation ======
void updateHybridAltimeter() {
  // Auto-calibrate with GPS when available
  if (isLocationValid() && isAltitudeValid() && bmp180_ready &&
      (millis() - lastGPSCalibration > RECALIBRATION_INTERVAL)) {
    
    float gpsAlt = getGPSAltitude();
    
    // Sanity check GPS altitude (reasonable range)
    if (gpsAlt > -500.0f && gpsAlt < 9000.0f) {
      calibrateSeaLevel(gpsAlt);
      autoCalibrated = true;
      lastGPSCalibration = millis();
      
      Serial.println("Auto-calibrated BMP180 with GPS altitude: " + String(gpsAlt, 1) + "m");
    }
  }
}

float getHybridAltitude() {
  if (autoCalibrated && bmp180_ready) {
    // Use calibrated barometric reading for precision
    return bmp180.readAltitude(seaLevelPa);
  } else if (isAltitudeValid()) {
    // Fall back to GPS when barometric not calibrated
    return getGPSAltitude();
  }
  return -999.0f; // No valid reading
}

String getAltitudeSource() {
  if (autoCalibrated && bmp180_ready) return "BAR";
  else if (isAltitudeValid()) return "GPS";
  return "NONE";
}

void displaySensorData() {
  // Show BMP180 temperature and pressure
  display.drawString(0, 0,  "BMP Temp: " + String(temperatureF, 1) + "F");
  display.drawString(0, 10, "Pressure: " + String(pressurePa / 100.0f, 1) + "hPa");
  
  // Show SHT30 temperature and humidity
  if (sht30_ready) {
    display.drawString(0, 20, "SHT Temp: " + String(temperatureF_SHT, 1) + "F");
    display.drawString(0, 30, "Humidity: " + String(humidity, 1) + "%");
  } else {
    display.drawString(0, 20, "SHT30: Not Ready");
  }
  
  // Show hybrid altitude with source indicator
  float hybridAlt = getHybridAltitude();
  String altSource = getAltitudeSource();
  
  if (hybridAlt != -999.0f) {
    display.drawString(0, 40, "Alt " + altSource + ": " + String(hybridAlt, 1) + "m");
  } else {
    display.drawString(0, 40, "Altitude: No Data");
  }
  
  // Display GPS coordinates
  if (isLocationValid()) {
    display.drawString(0, 50, "GPS: " + getFormattedCoordinates());
  } else {
    display.drawString(0, 50, "GPS: No Fix");
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
  
  if (isAltitudeValid()) {
    Serial.print("GPS Altitude: "); Serial.println(getGPSAltitude());
  }
}

void displayCombinedSensorData() {
  // Print BMP180 sensor data
  Serial.print(F("BMP180 - Temp: "));  
  Serial.print(temperatureF, 1); 
  Serial.print(F("°F, Pressure: "));  
  Serial.print(pressurePa / 100.0f, 1); Serial.print(F("hPa, "));
  
  // Print SHT30 sensor data
  if (sht30_ready) {
    Serial.print(F("SHT30 - Temp: "));  
    Serial.print(temperatureF_SHT, 1); 
    Serial.print(F("°F, Humidity: "));  
    Serial.print(humidity, 1); Serial.print(F("%, "));
  } else {
    Serial.print(F("SHT30 - Not Ready, "));
  }
  
  // Print hybrid altitude with source
  float hybridAlt = getHybridAltitude();
  String altSource = getAltitudeSource();
  
  if (hybridAlt != -999.0f) {
    Serial.print(F("Altitude (")); Serial.print(altSource); Serial.print(F(") = ")); 
    Serial.print(hybridAlt, 1); Serial.print(F("m, "));
  } else {
    Serial.print(F("Altitude = No Data, "));
  }
  
  // Show GPS altitude separately if available for comparison
  if (isAltitudeValid() && altSource == "BAR") {
    Serial.print(F("GPS Alt = ")); 
    Serial.print(getGPSAltitude(), 1); Serial.print(F("m, "));
  }
  
  // Then print GPS data (also without newline)
  displayGPSInfo();
  
  // End the line - displayGPSInfo() already ends with println()
}