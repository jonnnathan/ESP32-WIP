#include "lora_comm.h"

// Heltec WiFi LoRa 32 V3 pin definitions
#define LORA_SCK     9
#define LORA_MISO    11
#define LORA_MOSI    10
#define LORA_CS      8
#define LORA_RST     12
#define LORA_IRQ     14




// LoRa globals - Updated SPI initialization
SPIClass spi(HSPI);  // HSPI
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC, spi, spiSettings);

bool loraReady = false;
float lastRSSI = 0.0;
float lastSNR = 0.0;

bool initLoRa() {
    Serial.println(F("Initializing LoRa..."));
    
    // Initialize SPI with explicit pins
    spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    // Initialize radio
    int state = radio.begin(915.0); // 915 MHz - 
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("LoRa initialization successful"));
        
        // Set LoRa parameters
        radio.setBandwidth(125.0);      // kHz
        radio.setSpreadingFactor(7);
        radio.setCodingRate(5);
        radio.setOutputPower(14);       // dBm
        
        loraReady = true;
        return true;
    } else {
        Serial.print(F("LoRa initialization failed, code: "));
        Serial.println(state);
        loraReady = false;
        return false;
    }
}

bool sendSensorData(float tempF, float pressureHPa, float altitudeM, String gpsData) {
    if (!loraReady) return false;
    
    // Create formatted sensor data message
    String message = "SENSOR:";
    message += String(tempF, 1) + "F,";
    message += String(pressureHPa, 1) + "hPa,";
    message += String(altitudeM, 1) + "m,";
    message += gpsData;
    
    return sendMessage(message);
}

bool sendMessage(String message) {
    if (!loraReady) return false;
    
    Serial.print(F("LoRa TX: "));
    Serial.println(message);
    
    int state = radio.transmit(message);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("LoRa transmission successful!"));
        return true;
    } else {
        Serial.print(F("LoRa transmission failed, code: "));
        Serial.println(state);
        return false;
    }
}

void setLoRaReceiveMode() {
    if (loraReady) {
        radio.startReceive();
    }
}
String receiveMessage() {
    if (!loraReady) return "";

    String message;
    int state = radio.readData(message);

    if (state == RADIOLIB_ERR_NONE) {
        // Track the last message seen
        static String lastMessage = "";

        if (message == lastMessage) {
            // Same as before → ignore it
            radio.startReceive();   // re-arm the receiver
            return "";
        }

        // New message detected
        lastMessage = message;
        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();

        Serial.print(F("LoRa RX: "));
        Serial.print(message);
        Serial.print(F(" (RSSI: "));
        Serial.print(lastRSSI);
        Serial.print(F(" dBm, SNR: "));
        Serial.print(lastSNR);
        Serial.println(F(" dB)"));

        radio.startReceive();   // re-arm the receiver
        return message;
    } 

    // If some other error (but not just timeout), print and re-arm
    if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.print(F("LoRa receive error: "));
        Serial.println(state);
        radio.startReceive();
    }

    return "";
}




bool isLoRaReady() {
    return loraReady;
}

float getLastRSSI() {
    return lastRSSI;
}

float getLastSNR() {
    return lastSNR;
}