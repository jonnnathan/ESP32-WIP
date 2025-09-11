/*
#include "lora_comm.h"
#include "OLED.h"

OLED display;

// precise schedule
const unsigned long TX_INTERVAL = 3000; // 3s
unsigned long nextTx = 0;
unsigned long txSeq = 0;

void sendHelloWorld();  // fwd

void setup() {
  Serial.begin(115200);
  delay(200);

  if (display.init()) {
    display.clearDisplay();
    display.drawString(0, 0, "LoRa TX Ready");
    display.updateDisplay();
  }

  if (initLoRa()) {
    Serial.println("LoRa initialized");
  } else {
    Serial.println("LoRa init failed");
  }

  // first send happens immediately (set to millis() to delay 3s instead)
  nextTx = millis();
}

void loop() {
  unsigned long now = millis();

  // fire exactly on schedule, no drift
  if ((long)(now - nextTx) >= 0) {
    sendHelloWorld();
    nextTx += TX_INTERVAL;  // schedule the next at +3s from the last target, not from 'now'

    // 1s "sending", then 2s "waiting" drawn non-blocking
    display.clearDisplay();
    display.drawString(0, 0, "Sending...");
    display.updateDisplay();
  }

  // show "Waiting..." after the first second following a send
  static unsigned long lastSendAt = 0;
  static bool sentFlag = false;
  if (sentFlag == false && (long)(millis() - nextTx + TX_INTERVAL) >= 0) {
    // mark the moment we actually sent (one loop ago)
    lastSendAt = nextTx - TX_INTERVAL;
    sentFlag = true;
  }
  if (sentFlag && millis() - lastSendAt >= 1000 && millis() - lastSendAt < 3000) {
    display.clearDisplay();
    display.drawString(0, 0, "Waiting...");
    display.updateDisplay();
  }
  if (sentFlag && millis() - lastSendAt >= 3000) {
    sentFlag = false;  // next cycle will set it again
  }
}

void sendHelloWorld() {
  if (!isLoRaReady()) { Serial.println("LoRa not ready"); return; }

  txSeq++;
  String payload = "Hello World "
                   + String(millis()/1000) + "s"
                   + ",SEQ=" + String(txSeq);

  Serial.print("TX #"); Serial.print(txSeq);
  Serial.print(" @ "); Serial.print(millis()/1000); Serial.print("s: ");
  Serial.println(payload);

  bool ok = sendMessage(payload);
  if (!ok) Serial.println("Transmission failed");
}
*/