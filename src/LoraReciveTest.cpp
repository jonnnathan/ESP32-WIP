#include "lora_comm.h"
#include "OLED.h"

// OLED object
OLED display;

// Timing control
unsigned long lastMessageTime = 0;
bool messageOnScreen = false;

// Counter for received messages
unsigned long messageCount = 0;

void setup() {
  Serial.begin(115200);
  delay(300);

  // Initialize OLED
  if (display.init()) {
    Serial.println("OLED initialized");
    display.clearDisplay();
    display.drawString(0, 0, "LoRa Receiver Ready");
    display.updateDisplay();
  } else {
    Serial.println("OLED failed");
  }

  // Initialize LoRa
  if (initLoRa()) {
    Serial.println("LoRa initialized");
    setLoRaReceiveMode(); // enter continuous RX mode
  } else {
    Serial.println("LoRa init failed");
  }
}

void loop() {
  // Try to receive a new message
  String msg = receiveMessage();

  if (msg.length() > 0) {
    // Got new message
    float rssi = getLastRSSI();
    float snr  = getLastSNR();
    messageCount++;   // increment counter

    Serial.print("Received #");
    Serial.print(messageCount);
    Serial.print(": ");
    Serial.print(msg);
    Serial.print(" | RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm, SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    // Show on OLED for 1 second
    display.clearDisplay();
    display.drawString(0, 0, "Msg #" + String(messageCount));
    display.drawString(0, 10, msg);
    display.drawString(0, 20, "RSSI: " + String(rssi, 1) + " dBm");
    display.drawString(0, 30, "SNR: " + String(snr, 1) + " dB");
    display.updateDisplay();

    lastMessageTime = millis();
    messageOnScreen = true;
  }

  // After 1 second, switch to "Waiting..."
  if (messageOnScreen && (millis() - lastMessageTime > 1000)) {
    display.clearDisplay();
    display.drawString(0, 0, "Waiting...");
    display.updateDisplay();
    messageOnScreen = false;
  }

  delay(50); // small delay
}
