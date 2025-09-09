// Simplified LoRa Test - All sensors commented out
// #include "sht30.h"
// #include "OLED.h"
// #include "bmp180.h"
#include "lora_comm.h" // LoRa communication only
// #include <Wire.h>
// #include <math.h>

// Function Prototypes - sensors commented out
// void initBMP180();
// void calibrateSeaLevel(float knownAltM);
// void readBMP180Data();
// void displaySensorData();
void sendDataViaLoRa();  // LoRa transmission function only

// static float seaLevelPressureFrom(float pressurePa, float knownAltitudeM);

// Sensor Classes - commented out
// OLED display;
// BMP180 bmp180;

// ====== Second I2C bus - commented out ======
// TwoWire I2C_second = TwoWire(1);
// #define SDA2_PIN 7
// #define SCL2_PIN 20

// BMP180 Related Global variables - commented out
// bool  bmp180_ready = false;
// float temperatureF = 0.0f;   // Fahrenheit
// float pressurePa   = 0.0f;   // Pascals
// float altitudeStd  = 0.0f;   // meters (Std Atmosphere 101325 Pa)
// float altitudeCal  = 0.0f;   // meters (Calibrated with local SLP)
// bool  calibrated   = false;
// static const float SEA_LEVEL_DEFAULT_PA = 101325.0f; // Standard Atmosphere
// float seaLevelPa = SEA_LEVEL_DEFAULT_PA;             // Set after calibration
// unsigned long lastSensorRead  = 0;
// const unsigned long sensorInterval = 1000; // ms

// LoRa timing variables - ONLY THESE ACTIVE
unsigned long lastLoRaTransmit = 0;
const unsigned long loraTransmitInterval = 3000; // 3 seconds for quick testing

void setup() {
  Serial.begin(115200);
  delay(3000); // Give extra time for serial connection
  
  Serial.println();
  Serial.println("=========================================");
  Serial.println("    SIMPLE LORA TRANSMISSION TEST       ");
  Serial.println("=========================================");
  Serial.println("Status: All sensors DISABLED");
  Serial.println("Test: LoRa transmission only");
  Serial.println("Interval: 3 seconds");
  Serial.println("=========================================");
  Serial.println();
  /*
  // Visual feedback with built-in LED
  pinMode(1, OUTPUT);   // pin 1 orange
  pinMode(35, OUTPUT);  // ESP32-S3 might use pin 48
  
  
  // Flash LED during setup
  Serial.println("Setup LED test");
  for (int i = 0; i < 5; i++) {
    digitalWrite(2, HIGH);
    digitalWrite(48, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    digitalWrite(48, LOW);
    delay(200);
    Serial.print(".");
  }
  Serial.println(" LED test complete");
  */
  // OLED - commented out
  // display.init();
  // display.clearDisplay();
  // display.drawString(0, 0, "LoRa Test Mode");
  // display.drawString(0, 10, "Initializing...");
  // display.updateDisplay();
  
  // I2C - commented out
  // I2C_second.begin(SDA2_PIN, SCL2_PIN, 100000); // 100 kHz
  
  // BMP180 - commented out
  // initBMP180();
  
  Serial.println();
  Serial.println("Initializing LoRa radio...");
  
  // Initialize LoRa - THIS IS THE ONLY ACTIVE COMPONENT
  if (initLoRa()) {
    Serial.println("✓✓✓ SUCCESS: LoRa ready for transmission ✓✓✓");
    Serial.println("✓ Radio module detected and configured");
    Serial.println("✓ SPI communication established");
    Serial.println("✓ Frequency: 915 MHz");
    Serial.println("✓ Ready to transmit test messages");
    
    // OLED success - commented out
    // display.clearDisplay();
    // display.drawString(0, 0, "LoRa Test Mode");
    // display.drawString(0, 10, "LoRa: READY");
    // display.updateDisplay();
  } else {
    Serial.println("✗✗✗ FAILED: LoRa initialization FAILED ✗✗✗");
    Serial.println("✗ Check pin connections:");
    Serial.println("✗ SCK=5, MISO=19, MOSI=27, CS=8, RST=12, IRQ=26");
    Serial.println("✗ Continuing test anyway for debugging...");
    
    // OLED failure - commented out
    // display.clearDisplay();
    // display.drawString(0, 0, "LoRa Test Mode");
    // display.drawString(0, 10, "LoRa: FAILED");
    // display.updateDisplay();
  }
  
  // Calibrate BMP180 - commented out
  // if (bmp180_ready) {
  //   calibrateSeaLevel(/*knownAltM=*/91.0f);
  // }
  
  Serial.println();
  Serial.println("=== STARTING TRANSMISSION LOOP ===");
  Serial.println("Watch for transmission messages...");
  Serial.println(">>> CHECK SPECTRUM ANALYZER AT 915 MHz <<<");
  Serial.println("=======================================");
  
  delay(1000);
}

void loop() {
  static unsigned long loopCount = 0;
  loopCount++;
  
  // Heartbeat LED - fast blink to show we're running
// bool ledState = (millis() / 250) % 2;  // Toggle every 250ms
//  digitalWrite(2, ledState);
  //digitalWrite(48, ledState);
  
  // Read sensor data - commented out
  // if (millis() - lastSensorRead >= sensorInterval) {
  //   readBMP180Data();
  //   lastSensorRead = millis();
  // }
  
  // Send data via LoRa every 3 seconds - MAIN TEST
  if (millis() - lastLoRaTransmit >= loraTransmitInterval) {
    sendDataViaLoRa();
    lastLoRaTransmit = millis();
  }
  


  // Update display - commented out
  // display.clearDisplay();
  // displaySensorData();
  // display.updateDisplay();
  
  // Show we're alive every 10 seconds
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 1000) {
    Serial.print("[HEARTBEAT] Loop count: ");
    Serial.print(loopCount);
    Serial.print(", Uptime: ");
    Serial.print(millis() / 1000);
    Serial.print("s, Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    lastHeartbeat = millis();
  }
  
  delay(50); // Small delay to prevent watchdog issues
}

// Send test data via LoRa - SIMPLIFIED VERSION WITH FAKE DATA
void sendDataViaLoRa() {
  static int transmissionCount = 0;
  transmissionCount++;
  
  Serial.println();
  Serial.println("==========================================");
  Serial.print(">>> LoRa TRANSMISSION #");
  Serial.print(transmissionCount);
  Serial.println(" STARTING <<<");
  Serial.println("==========================================");
  
  if (!isLoRaReady()) {
    Serial.println("✗ LoRa not ready - skipping transmission");
    Serial.println("✗ Check LoRa initialization errors above");
    return;
  }
  
  // Create test message with fake sensor data (since sensors are off)
  String message = "TEST" + String(transmissionCount) + ":";
  message += "72.5F,";        // Fake temperature
  message += "1013.2hPa,";    // Fake pressure  
  message += "91.0m,";        // Fake altitude
  message += "TIME=" + String(millis()/1000) + "s";
  
  Serial.print("Message: ");
  Serial.println(message);
  Serial.print("Length: ");
  Serial.print(message.length());
  Serial.println(" characters");
  Serial.println();
  Serial.println(">>> TRANSMITTING NOW - CHECK SPECTRUM ANALYZER! <<<");
  Serial.println(">>> Frequency: 915 MHz <<<");
  Serial.println(">>> Look for signal spike NOW! <<<");
  
  // Send the message
  bool success = sendMessage(message);
  
  Serial.println();
  if (success) {
    Serial.println("✓✓✓ LoRa transmission SUCCESSFUL! ✓✓✓");
    Serial.println("✓ Message sent without errors");
    Serial.println("✓ Should be visible on spectrum analyzer");
    Serial.println("✓ RF signal transmitted at 915 MHz");
  } else {
    Serial.println("✗✗✗ LoRa transmission FAILED! ✗✗✗");
    Serial.println("✗ Check radio module and connections");
    Serial.println("✗ Verify pin definitions in lora_comm.cpp");
  }
  
  Serial.println();
  Serial.print("Next transmission in ");
  Serial.print(loraTransmitInterval / 1000);
  Serial.println(" seconds");
  Serial.println("Total transmissions so far: " + String(transmissionCount));
  Serial.println("==========================================");
}

// ====== ALL SENSOR FUNCTIONS COMMENTED OUT ======

/*
// BMP180 Implementation - DISABLED
void initBMP180() {
  Serial.println(F("Initializing BMP180..."));
  if (bmp180.begin(&I2C_second, BMP180_OSS_ULTRAHIGHRES)) {
    bmp180_ready = true;
    Serial.println("✓ BMP180 initialized");
  } else {
    bmp180_ready = false;
    Serial.println("✗ BMP180 initialization failed");
  }
}

void calibrateSeaLevel(float knownAltM) {
  const int N = 8;
  float sumPa = 0.0f;
  for (int i = 0; i < N; ++i) {
    float p = bmp180.readPressure();
    if (p > 10000.0f && p < 120000.0f) {
      sumPa += p;
    } else {
      --i;
    }
    delay(60);
  }
  float pNow = sumPa / N;
  seaLevelPa = seaLevelPressureFrom(pNow, knownAltM);
  calibrated = true;
}

static float seaLevelPressureFrom(float pressurePa, float knownAltitudeM) {
  return pressurePa / powf(1.0f - (knownAltitudeM / 44330.0f), 5.255f);
}

void readBMP180Data() {
  if (!bmp180_ready) {
    temperatureF = 70.0f;
    pressurePa = 101325.0f;
    altitudeCal = 91.0f;
    return;
  }
  
  float tempC = bmp180.readTemperature();
  temperatureF = (tempC * 9.0f / 5.0f) + 32.0f;
  pressurePa = bmp180.readPressure();
  altitudeStd = bmp180.readAltitude(SEA_LEVEL_DEFAULT_PA);
  if (calibrated) {
    altitudeCal = bmp180.readAltitude(seaLevelPa);
  } else {
    altitudeCal = altitudeStd;
  }
}

void displaySensorData() {
  static int displayCount = 0;
  displayCount++;
  
  display.drawString(0, 0,  "LoRa Test #" + String(displayCount));
  display.drawString(0, 10, "Temp: " + String(temperatureF, 1) + "F");
  display.drawString(0, 20, "Press: " + String(pressurePa / 100.0f, 1) + "hPa");
  display.drawString(0, 30, "Alt: " + String(altitudeCal, 1) + "m");
  
  if (isLoRaReady()) {
    display.drawString(0, 40, "LoRa: ACTIVE");
    unsigned long timeToNext = loraTransmitInterval - (millis() - lastLoRaTransmit);
    if (timeToNext > loraTransmitInterval) timeToNext = 0;
    display.drawString(0, 50, "Next TX: " + String(timeToNext / 1000) + "s");
  } else {
    display.drawString(0, 40, "LoRa: FAILED");
  }
}
*/