#include <SPI.h>
#include <DW1000.h>

// ---------- Custom Hardware Pins ----------
const uint8_t PIN_CS  = 20; // D20 (A6)
const uint8_t PIN_IRQ = 21; // D21 (A7)
const uint8_t PIN_RST = 3;  // D3

byte rxBuffer[128]; // Max DW1000 frame size is 127 bytes

void startReceiver() {
  DW1000.newReceive();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  // Initialize UWB
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  startReceiver();
  Serial.println("Polo Seeker Node (v2) Ready.");
  Serial.println("Listening for wireless UWB payloads...");
}

void loop() {
  byte status[5];
  DW1000.readBytes(SYS_STATUS, 0x00, status, 5);

  bool dataReady = (status[1] & 0x20); // RXDFR
  bool goodCRC   = (status[1] & 0x40); // RXFCG

  if (dataReady && goodCRC) {
    uint16_t len = DW1000.getDataLength();
    if (len > sizeof(rxBuffer)) len = sizeof(rxBuffer);
    DW1000.getData(rxBuffer, len);

    // Clear status registers
    byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);

    // Ensure it's treated as a null-terminated string
    rxBuffer[len - 1] = '\0'; 
    
    // Pass the payload up to Python using a specific prefix
    Serial.print("UWB_PAYLOAD:");
    Serial.println((char*)rxBuffer);
    
    startReceiver();
  } else if (dataReady) {
    // Bad CRC, clear and restart
    byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);
    startReceiver();
  }
  
  delayMicroseconds(100);
}
