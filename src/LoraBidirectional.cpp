#include "lora_comm.h"
#include "OLED.h"

OLED display;

// Device configuration - SET THIS DIFFERENTLY ON EACH DEVICE
const uint8_t DEVICE_ID = 1; // Device 1 or Device 2
const char* DEVICE_NAME = "DEV1"; // "DEV1" or "DEV2"

// Communication timing
const unsigned long TX_INTERVAL = 5000; // 5 seconds between transmissions
const unsigned long RX_CHECK_INTERVAL = 100; // Check for messages every 100ms
const unsigned long DISPLAY_TIME = 2000; // Show message on display for 2 seconds

// Timing variables
unsigned long nextTx = 0;
unsigned long lastRxCheck = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long txSeq = 0;
unsigned long rxCount = 0;

// UI state
enum DisplayState {
  DISPLAY_WAITING,
  DISPLAY_SENDING,
  DISPLAY_RECEIVED_MSG,
  DISPLAY_TX_FAILED
};
DisplayState currentDisplay = DISPLAY_WAITING;
unsigned long displayStateStart = 0;

// Statistics
unsigned long totalTxAttempts = 0;
unsigned long successfulTx = 0;
unsigned long totalRxMessages = 0;
unsigned long validRxMessages = 0;
unsigned long corruptedRxMessages = 0;
unsigned long duplicateRxMessages = 0;

// Message tracking
String lastReceivedMessage = "";
unsigned long lastRxTxCount = 0;
String lastSentMessage = "";

// --- Forward declarations ---
bool transmitMessage();
void checkForIncomingMessages();
void updateDisplay();
void printSystemInfo();
void printTransceiverStats();
void printSeparator();
bool parseReceivedMessage(const String& message, String& sentence, unsigned long& txCount, String& fromDevice);
void handleReceivedMessage(const String& message, float rssi, float snr);
String getNextSentence();

void setup() {
  Serial.begin(115200);
  delay(200);
  
  printSeparator();
  Serial.println("╔══════════════════════════════════════════════════════════════╗");
  Serial.print("║                LoRa Two-Way Transceiver - ");
  Serial.print(DEVICE_NAME);
  Serial.println("               ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println();
  
  Serial.println("[STARTUP] Initializing two-way communication system...");
  Serial.print("[STARTUP] Device ID: ");
  Serial.print(DEVICE_ID);
  Serial.print(" (");
  Serial.print(DEVICE_NAME);
  Serial.println(")");
  Serial.print("[STARTUP] TX interval: ");
  Serial.print(TX_INTERVAL);
  Serial.println(" ms");
  Serial.print("[STARTUP] RX check interval: ");
  Serial.print(RX_CHECK_INTERVAL);
  Serial.println(" ms");
  Serial.println();

  // Initialize OLED
  Serial.print("[OLED] Initializing display... ");
  if (display.init()) {
    Serial.println("✓ SUCCESS");
    display.clearDisplay();
    display.drawString(0, 0, "LoRa " + String(DEVICE_NAME));
    display.drawString(0, 10, "Two-Way Ready");
    display.updateDisplay();
  } else {
    Serial.println("✗ FAILED");
    Serial.println("[ERROR] OLED initialization failed");
  }

  // Initialize LoRa
  Serial.print("[LORA] Initializing radio module... ");
  if (initLoRa()) {
    Serial.println("✓ SUCCESS");
    Serial.println("[LORA] Radio ready for two-way communication");
    printSystemInfo();
  } else {
    Serial.println("✗ FAILED");
    Serial.println("[ERROR] LoRa initialization failed");
  }

  // Set initial timing
  nextTx = millis() + (DEVICE_ID * 2500); // Stagger initial transmissions
  lastRxCheck = millis();
  
  Serial.print("[SCHEDULER] First transmission in ");
  Serial.print((nextTx - millis()));
  Serial.println(" ms");
  
  printSeparator();
  Serial.println("[MAIN LOOP] Starting two-way communication...");
  Serial.println("[MODE] Transmit every 5s, Listen continuously");
  Serial.println();
}

void loop() {
  unsigned long now = millis();
  
  // Check for incoming messages frequently
  if (now - lastRxCheck >= RX_CHECK_INTERVAL) {
    checkForIncomingMessages();
    lastRxCheck = now;
  }
  
  // Transmit on schedule
  if ((long)(now - nextTx) >= 0) {
    Serial.print("[TX-SCHEDULE] Transmission #");
    Serial.print(txSeq + 1);
    Serial.println(" triggered");
    
    bool success = transmitMessage();
    totalTxAttempts++;
    
    if (success) {
      successfulTx++;
      currentDisplay = DISPLAY_SENDING;
      Serial.println("[TX-SUCCESS] Message transmitted successfully");
    } else {
      currentDisplay = DISPLAY_TX_FAILED;
      Serial.println("[TX-FAILED] Transmission failed");
    }
    
    displayStateStart = now;
    updateDisplay();
    
    // Schedule next transmission
    nextTx = now + TX_INTERVAL;
    Serial.print("[SCHEDULER] Next transmission in ");
    Serial.print(TX_INTERVAL);
    Serial.println(" ms");
    
    // Show stats every 10 transmissions
    if (totalTxAttempts % 10 == 0) {
      printTransceiverStats();
    }
    
    Serial.println();
  }
  
  // Update display based on state and timing
  if (currentDisplay != DISPLAY_WAITING) {
    if (now - displayStateStart >= DISPLAY_TIME) {
      currentDisplay = DISPLAY_WAITING;
      updateDisplay();
    }
  }
  
  delay(10); // Small delay to prevent overwhelming the loop
}

bool transmitMessage() {
  if (!isLoRaReady()) {
    Serial.println("[TX-ERROR] LoRa module not ready");
    return false;
  }

  txSeq++;
  String sentence = getNextSentence();
  
  // Create message format: "SENTENCE [DEV1:#123]"
  char payload[120];
  snprintf(payload, sizeof(payload), "%s [%s:#%lu]", sentence.c_str(), DEVICE_NAME, txSeq);
  
  lastSentMessage = String(payload);
  
  Serial.println("┌─────────────────────────────────────────────────────────────┐");
  Serial.print("│ TRANSMITTING FROM ");
  Serial.print(DEVICE_NAME);
  Serial.print(" - MESSAGE #");
  Serial.print(txSeq);
  
  int padding = 29 - String(txSeq).length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("│");
  
  Serial.print("│ Content: \"");
  Serial.print(payload);
  Serial.print("\"");
  
  int contentLen = 11 + String(payload).length() + 1;
  if (contentLen < 65) {
    padding = 65 - contentLen;
    for(int i = 0; i < padding; i++) Serial.print(" ");
  }
  Serial.println("│");
  
  Serial.print("│ Size: ");
  Serial.print(strlen(payload));
  Serial.print(" bytes");
  
  String sizeStr = String(strlen(payload)) + " bytes";
  padding = 52 - sizeStr.length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("│");
  
  Serial.println("└─────────────────────────────────────────────────────────────┘");
  
  return sendMessage(String(payload));
}

void checkForIncomingMessages() {
  String msg = receiveMessage();
  
  if (msg.length() > 0) {
    float rssi = getLastRSSI();
    float snr = getLastSNR();
    
    Serial.println("[RX-ACTIVITY] ✓ MESSAGE RECEIVED");
    handleReceivedMessage(msg, rssi, snr);
    
    // Update display to show received message
    currentDisplay = DISPLAY_RECEIVED_MSG;
    displayStateStart = millis();
    updateDisplay();
  }
}

void handleReceivedMessage(const String& message, float rssi, float snr) {
  totalRxMessages++;
  
  // Parse the received message
  String sentence = "";
  unsigned long txCount = 0;
  String fromDevice = "";
  bool parseSuccess = parseReceivedMessage(message, sentence, txCount, fromDevice);
  
  if (parseSuccess) {
    validRxMessages++;
    rxCount++;
    
    // Check for duplicates
    if (message == lastReceivedMessage) {
      duplicateRxMessages++;
      Serial.println("[RX-WARNING] Duplicate message detected");
    }
    
    lastReceivedMessage = message;
    lastRxTxCount = txCount;
    
    Serial.println();
    Serial.println("┌─────────────────────────────────────────────────────────────┐");
    Serial.print("│ MESSAGE FROM ");
    Serial.print(fromDevice);
    Serial.print(" - RX #");
    Serial.print(rxCount);
    
    int padding = 39 - fromDevice.length() - String(rxCount).length();
    for(int i = 0; i < padding; i++) Serial.print(" ");
    Serial.println("│");
    
    Serial.print("│ Remote TX #");
    Serial.print(txCount);
    Serial.print(" | Timestamp: ");
    Serial.print(millis() / 1000);
    Serial.print("s");
    
    String timeInfo = String(txCount) + " | Timestamp: " + String(millis() / 1000) + "s";
    padding = 40 - timeInfo.length();
    for(int i = 0; i < padding; i++) Serial.print(" ");
    Serial.println("│");
    
    Serial.print("│ Content: \"");
    Serial.print(sentence);
    Serial.print("\"");
    
    int contentLen = 11 + sentence.length() + 1;
    if (contentLen < 65) {
      padding = 65 - contentLen;
      for(int i = 0; i < padding; i++) Serial.print(" ");
    }
    Serial.println("│");
    
    Serial.print("│ Raw: \"");
    Serial.print(message);
    Serial.print("\"");
    
    int rawLen = 8 + message.length() + 1;
    if (rawLen < 65) {
      padding = 65 - rawLen;
      for(int i = 0; i < padding; i++) Serial.print(" ");
    }
    Serial.println("│");
    
    Serial.println("├─────────────────────────────────────────────────────────────┤");
    
    Serial.print("│ RSSI: ");
    Serial.print(rssi, 1);
    Serial.print(" dBm | SNR: ");
    Serial.print(snr, 1);
    Serial.print(" dB | Quality: ");
    
    String quality;
    if (rssi > -70 && snr > 10) quality = "Excellent";
    else if (rssi > -85 && snr > 5) quality = "Good";
    else if (rssi > -100 && snr > 0) quality = "Fair";
    else quality = "Poor";
    
    Serial.print(quality);
    
    String signalInfo = String(rssi, 1) + " dBm | SNR: " + String(snr, 1) + " dB | Quality: " + quality;
    padding = 56 - signalInfo.length();
    for(int i = 0; i < padding; i++) Serial.print(" ");
    Serial.println("│");
    
    Serial.println("└─────────────────────────────────────────────────────────────┘");
    Serial.println();
    
  } else {
    corruptedRxMessages++;
    Serial.println("[RX-ERROR] Failed to parse received message");
    Serial.print("[RX-ERROR] Raw content: \"");
    Serial.print(message);
    Serial.println("\"");
  }
}

bool parseReceivedMessage(const String& message, String& sentence, unsigned long& txCount, String& fromDevice) {
  // Expected format: "Sentence text [DEV1:#123]" or "Sentence text [DEV2:#123]"
  
  int bracketStart = message.lastIndexOf(" [");
  if (bracketStart == -1) return false;
  
  int bracketEnd = message.indexOf(']', bracketStart);
  if (bracketEnd == -1) return false;
  
  // Extract sentence
  sentence = message.substring(0, bracketStart);
  sentence.trim();
  
  // Extract device and count info
  String bracketContent = message.substring(bracketStart + 2, bracketEnd); // Skip " ["
  
  int colonPos = bracketContent.indexOf(":#");
  if (colonPos == -1) return false;
  
  fromDevice = bracketContent.substring(0, colonPos);
  String countStr = bracketContent.substring(colonPos + 2); // Skip ":#"
  
  txCount = countStr.toInt();
  
  // Validate
  if (sentence.length() == 0 || fromDevice.length() == 0 || (txCount == 0 && countStr != "0")) {
    return false;
  }
  
  // Don't process our own messages
  if (fromDevice == DEVICE_NAME) {
    Serial.println("[RX-FILTER] Ignoring our own transmission");
    return false;
  }
  
  return true;
}

String getNextSentence() {
  const char* sentences[] = {
    "Hello from device transmitter!",
    "Two-way LoRa communication active.",
    "Testing bidirectional radio link.",
    "Wireless mesh network operational.",
    "Real-time data exchange working.",
    "Remote monitoring system online.",
    "IoT communication protocol active.",
    "Distributed sensor network ready.",
    "Long range telemetry established.",
    "Peer-to-peer radio link confirmed."
  };
  
  const int numSentences = sizeof(sentences) / sizeof(sentences[0]);
  int index = (txSeq - 1) % numSentences;
  return String(sentences[index]);
}

void updateDisplay() {
  display.clearDisplay();
  
  switch (currentDisplay) {
    case DISPLAY_WAITING:
      display.drawString(0, 0, String(DEVICE_NAME) + " Ready");
      display.drawString(0, 10, "TX:" + String(txSeq) + " RX:" + String(rxCount));
      if (totalRxMessages > 0) {
        float successRate = (float)validRxMessages / totalRxMessages * 100.0;
        display.drawString(0, 20, "RX Rate:" + String(successRate, 0) + "%");
      }
      if (totalTxAttempts > 0) {
        float txSuccessRate = (float)successfulTx / totalTxAttempts * 100.0;
        display.drawString(0, 30, "TX Rate:" + String(txSuccessRate, 0) + "%");
      }
      break;
      
    case DISPLAY_SENDING:
      display.drawString(0, 0, "Sending...");
      display.drawString(0, 10, "TX #" + String(txSeq));
      display.drawString(0, 20, "To: ALL");
      display.drawString(0, 30, "Size:" + String(lastSentMessage.length()) + "b");
      break;
      
    case DISPLAY_RECEIVED_MSG:
      display.drawString(0, 0, "Received!");
      display.drawString(0, 10, "From: " + String(lastReceivedMessage.indexOf("DEV1") > 0 ? "DEV1" : "DEV2"));
      display.drawString(0, 20, "RX #" + String(rxCount));
      display.drawString(0, 30, "TX #" + String(lastRxTxCount));
      break;
      
    case DISPLAY_TX_FAILED:
      display.drawString(0, 0, "TX Failed!");
      display.drawString(0, 10, "Check radio");
      display.drawString(0, 20, "TX #" + String(txSeq));
      display.drawString(0, 30, "Retrying...");
      break;
  }
  
  display.updateDisplay();
}

void printSystemInfo() {
  Serial.println();
  Serial.println("┌─────────────────────────────────────────────────────────────┐");
  Serial.println("│                    TWO-WAY SYSTEM INFO                     │");
  Serial.println("├─────────────────────────────────────────────────────────────┤");
  Serial.print("│ Device: ");
  Serial.print(DEVICE_NAME);
  Serial.print(" (ID: ");
  Serial.print(DEVICE_ID);
  Serial.print(")");
  
  String deviceInfo = String(DEVICE_NAME) + " (ID: " + String(DEVICE_ID) + ")";
  int padding = 48 - deviceInfo.length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("│");
  
  Serial.print("│ TX Interval: ");
  Serial.print(TX_INTERVAL);
  Serial.println(" ms                                   │");
  Serial.print("│ RX Check: ");
  Serial.print(RX_CHECK_INTERVAL);
  Serial.println(" ms                                     │");
  Serial.println("│ Mode: Two-way Transceiver                              │");
  Serial.println("└─────────────────────────────────────────────────────────────┘");
  Serial.println();
}

void printTransceiverStats() {
  Serial.println();
  Serial.println("╔═════════════════════════════════════════════════════════════╗");
  Serial.println("║                  TWO-WAY COMMUNICATION STATS               ║");
  Serial.println("╠═════════════════════════════════════════════════════════════╣");
  
  // TX Stats
  Serial.print("║ TX Attempts: ");
  Serial.print(totalTxAttempts);
  Serial.print(" | Successful: ");
  Serial.print(successfulTx);
  
  String txStats = String(totalTxAttempts) + " | Successful: " + String(successfulTx);
  int padding = 38 - txStats.length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("║");
  
  if (totalTxAttempts > 0) {
    float txRate = (float)successfulTx / totalTxAttempts * 100.0;
    Serial.print("║ TX Success Rate: ");
    Serial.print(txRate, 1);
    Serial.print("%");
    
    String txRateStr = String(txRate, 1) + "%";
    padding = 40 - txRateStr.length();
    for(int i = 0; i < padding; i++) Serial.print(" ");
    Serial.println("║");
  }
  
  // RX Stats
  Serial.print("║ RX Total: ");
  Serial.print(totalRxMessages);
  Serial.print(" | Valid: ");
  Serial.print(validRxMessages);
  Serial.print(" | Corrupted: ");
  Serial.print(corruptedRxMessages);
  
  String rxStats = String(totalRxMessages) + " | Valid: " + String(validRxMessages) + " | Corrupted: " + String(corruptedRxMessages);
  padding = 59 - rxStats.length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("║");
  
  if (totalRxMessages > 0) {
    float rxRate = (float)validRxMessages / totalRxMessages * 100.0;
    Serial.print("║ RX Success Rate: ");
    Serial.print(rxRate, 1);
    Serial.print("%");
    
    String rxRateStr = String(rxRate, 1) + "%";
    padding = 40 - rxRateStr.length();
    for(int i = 0; i < padding; i++) Serial.print(" ");
    Serial.println("║");
  }
  
  Serial.print("║ Duplicates: ");
  Serial.print(duplicateRxMessages);
  
  padding = 46 - String(duplicateRxMessages).length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("║");
  
  Serial.print("║ Runtime: ");
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  
  Serial.print(hours);
  Serial.print("h ");
  Serial.print(minutes);
  Serial.print("m ");
  Serial.print(seconds);
  Serial.print("s");
  
  String uptimeStr = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
  padding = 41 - uptimeStr.length();
  for(int i = 0; i < padding; i++) Serial.print(" ");
  Serial.println("║");
  
  Serial.println("╚═════════════════════════════════════════════════════════════╝");
  Serial.println();
}

void printSeparator() {
  Serial.println("===============================================================");
}