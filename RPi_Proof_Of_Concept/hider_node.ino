#include <SPI.h>
#include <DW1000.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;
const uint8_t PIN_RST = 3;

#define MSG_PING 0x05 // Custom ping message ID

byte txBuffer[2];

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // Wait up to 5s for USB serial connection

  Serial.println("INIT_SUCCESS: UWB Sensor Ready (Hider Node)");

  // Initialize DW1000
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(2);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
}

void loop() {
  // Listen to USB Serial for commands from Raspberry Pi
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "SEND_PING") {
      txBuffer[0] = MSG_PING;
      txBuffer[1] = 0x01; // Arbitrary payload byte

      DW1000.newTransmit();
      DW1000.setData(txBuffer, 2);
      DW1000.startTransmit();
      
      // Poll the DW1000 to ensure transmission finishes
      unsigned long start = millis();
      while ((millis() - start) < 50) {
        byte status[5];
        DW1000.readBytes(SYS_STATUS, 0x00, status, 5);
        if (status[0] & 0x80) break; // TXFRS: Transmit frame sent
        delayMicroseconds(50);
      }
      
      // Clear status registers
      byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);

      // Report back to Pi
      Serial.println("PING_SENT_SUCCESSFULLY");
    }
  }
}
