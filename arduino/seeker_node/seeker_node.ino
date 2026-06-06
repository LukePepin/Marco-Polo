#include <SPI.h>
#include <DW1000.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;
const uint8_t PIN_RST = 3;

#define MSG_PING 0x05 // Must match the Hider's custom ping ID

bool uwbInitialized = false;
byte rxBuffer[20];

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
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

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
        Serial.println(msg);
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

  byte status[5];
  DW1000.readBytes(SYS_STATUS, 0x00, status, 5);

  bool dataReady = (status[1] & 0x20); // RXDFR: Receiver Data Frame Ready
  bool goodCRC   = (status[1] & 0x40); // RXFCG: Receiver FCS Good

  if (dataReady && goodCRC) {
    uint16_t len = DW1000.getDataLength();
    if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
    DW1000.getData(rxBuffer, len);

    // Clear status registers
    byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);

    // Check if the received message was our custom Ping
    if (rxBuffer[0] == MSG_PING) {
      Serial.println("UWB_PING_DETECTED");
    }
    
    // Restart receiver to keep listening
    startReceiver();
    
  } else if (dataReady) {
    // Bad CRC or partial frame, clear and restart receiver
    byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);
    startReceiver();
  }
  
  delayMicroseconds(100);
}
