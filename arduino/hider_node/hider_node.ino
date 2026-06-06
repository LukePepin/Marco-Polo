#include <SPI.h>
#include <DW1000.h>

// ---------- DW1000 wiring ----------
const uint8_t PIN_CS  = 10;
const uint8_t PIN_IRQ = 2;
const uint8_t PIN_RST = 3;

#define MSG_PING 0x05 // Custom ping message ID

bool uwbInitialized = false;
byte txBuffer[2];

// Statistics
unsigned long tx_ok = 0;
unsigned long tx_fail = 0;

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
  DW1000.setDeviceAddress(2);
  DW1000.setNetworkId(10);
  // Use 6.8 Mbps (short preamble, fast data rate) which is much more tolerant to crystal offset
  DW1000.enableMode(DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
  DW1000.commitConfiguration();

  // Read device identifier to verify connection
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);

  // A working DW1000 should return a string starting with "DECA" (0xDECA)
  if (strstr(msg, "DECA") != NULL) {
    uwbInitialized = true;
    Serial.print("INIT_SUCCESS: UWB Sensor Ready (Hider Node) - ID: ");
    Serial.println(msg);
  } else {
    uwbInitialized = false;
    Serial.print("INIT_FAILURE: UWB Sensor NOT detected! Read ID: ");
    Serial.println(msg);
  }
}

void loop() {
  // Listen to USB Serial for commands from Raspberry Pi
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "GET_STATUS") {
      if (uwbInitialized) {
        char msg[128];
        DW1000.getPrintableDeviceIdentifier(msg);
        Serial.print("STATUS: OK (Hider), ID: ");
        Serial.print(msg);
        Serial.print(" | TX_OK: ");
        Serial.print(tx_ok);
        Serial.print(" | TX_FAIL: ");
        Serial.println(tx_fail);
      } else {
        Serial.println("STATUS: ERROR_UWB_OFFLINE (Hider)");
      }
    } 
    else if (cmd == "SEND_PING") {
      if (!uwbInitialized) {
        Serial.println("ERROR_UWB_OFFLINE");
      } else {
        txBuffer[0] = MSG_PING;
        txBuffer[1] = 0x01; // Arbitrary payload byte

        DW1000.newTransmit();
        DW1000.setData(txBuffer, 2);
        DW1000.startTransmit();
        
        // Poll the DW1000 to ensure transmission finishes
        unsigned long start = millis();
        bool txSuccess = false;
        while ((millis() - start) < 50) {
          byte status[5];
          DW1000.readBytes(SYS_STATUS, 0x00, status, 5);
          if (status[0] & 0x80) { // TXFRS: Transmit frame sent
            txSuccess = true;
            break;
          }
          delayMicroseconds(50);
        }
        
        // Clear status registers
        byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);

        // Report back to Pi
        if (txSuccess) {
          tx_ok++;
          Serial.println("PING_SENT_SUCCESSFULLY");
        } else {
          tx_fail++;
          Serial.println("ERROR_TX_TIMEOUT");
        }
      }
    }
  }
}
