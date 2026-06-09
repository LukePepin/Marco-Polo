#include <SPI.h>
#include <DW1000.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = A1;
const uint8_t PIN_IRQ = A4;
const uint8_t PIN_RST = 7;

#define MSG_PING 0x05 // Must match the Hider's custom ping ID

bool uwbInitialized = false;
byte rxBuffer[20];

// Statistics
unsigned long rx_ok = 0;
unsigned long rx_fail = 0;
unsigned long rx_err = 0;

void handleReceived() {
  uint16_t len = DW1000.getDataLength();
  if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
  DW1000.getData(rxBuffer, len);

  // Check if the received message was our custom Ping
  if (rxBuffer[0] == MSG_PING) {
    rx_ok++;
    Serial.println("UWB_PING_DETECTED");
  }
}

void handleReceiveFailed() {
  rx_fail++;
  // Silent restart (library does it automatically if receivePermanently is true)
}

void handleError() {
  rx_err++;
}

void startReceiver() {
  DW1000.newReceive();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // Wait up to 5s for USB serial connection

  // Initialize DW1000
  DW1000.begin(PIN_IRQ, PIN_RST);
  // Detach interrupt to prevent SPI calls inside ISR on Mbed OS (which causes crashes)
  detachInterrupt(digitalPinToInterrupt(PIN_IRQ));
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  // Use 6.8 Mbps (short preamble, fast data rate) which is much more tolerant to crystal offset
  DW1000.enableMode(DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
  DW1000.commitConfiguration();

  // Attach callbacks
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveFailedHandler(handleReceiveFailed);
  DW1000.attachErrorHandler(handleError);

  // Read device identifier to verify connection
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);

  // A working DW1000 should return a string starting with "DECA" (0xDECA)
  if (strstr(msg, "DECA") != NULL) {
    uwbInitialized = true;
    Serial.print("INIT_SUCCESS: UWB Sensor Ready (Seeker Node) - ID: ");
    Serial.println(msg);
    // Turn on receiver
    startReceiver();
  } else {
    uwbInitialized = false;
    Serial.print("INIT_FAILURE: UWB Sensor NOT detected! Read ID: ");
    Serial.println(msg);
  }
}

void loop() {
  // Listen to USB Serial for status commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "GET_STATUS") {
      if (uwbInitialized) {
        char msg[128];
        DW1000.getPrintableDeviceIdentifier(msg);
        Serial.print("STATUS: OK (Seeker), ID: ");
        Serial.print(msg);
        Serial.print(" | RX_OK: ");
        Serial.print(rx_ok);
        Serial.print(" | RX_FAIL: ");
        Serial.print(rx_fail);
        Serial.print(" | RX_ERR: ");
        Serial.println(rx_err);
      } else {
        Serial.println("STATUS: ERROR_UWB_OFFLINE (Seeker)");
      }
    }
  }

  // If UWB is not initialized, don't read registers
  if (!uwbInitialized) {
    delay(100);
    return;
  }

  // Poll the physical IRQ pin. If it is high, handle the interrupt in thread context.
  if (digitalRead(PIN_IRQ) == HIGH) {
    DW1000.handleInterrupt();
  }
  
  delayMicroseconds(100);
}
