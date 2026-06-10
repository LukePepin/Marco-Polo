#include <SPI.h>
#include <DW1000.h>

// ---------- Custom Hardware Pins ----------
const uint8_t PIN_CS  = 20; // D20 (A6)
const uint8_t PIN_IRQ = 21; // D21 (A7)
const uint8_t PIN_RST = 3;  // D3

// Hardware interrupt flags
volatile boolean received = false;
volatile boolean error = false;

void handleReceived() {
  received = true;
}

void handleError() {
  error = true;
}

void startReceiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
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

  // Attach the proper hardware interrupt handlers to the DW1000 library
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveFailedHandler(handleError);
  DW1000.attachErrorHandler(handleError);

  startReceiver();
  Serial.println("Polo Seeker Node (v2) Ready.");
  Serial.println("Listening for wireless UWB payloads...");
}

void loop() {
  // Check the interrupt flag
  if (received) {
    received = false;
    
    // Extract payload
    String payload;
    DW1000.getData(payload);
    
    // Pass the payload up to Python using a specific prefix
    Serial.print("UWB_PAYLOAD:");
    Serial.println(payload);
  }
  
  if (error) {
    error = false;
    Serial.println("[Arduino]: Packet failed (CRC Error).");
  }
}
