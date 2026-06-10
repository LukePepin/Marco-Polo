#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>

// ---------- Custom Hardware Pins ----------
const uint8_t PIN_CS  = 20; // D20 (A6)
const uint8_t PIN_IRQ = 21; // D21 (A7)
const uint8_t PIN_RST = 3;  // D3

char gpsBuffer[100] = "NO_GPS_LOCK_YET";
unsigned long lastSendTime = 0;

volatile boolean sent = false;

// Interrupt handler required by DW1000 library
void handleSent() {
  sent = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  // Start GPS Serial
  Serial1.begin(9600);
  
  // Start IMU (Accelerometer)
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU! Check hardware.");
    while (1);
  }

  // Start UWB
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(2);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Attach hardware interrupt for transmission success
  DW1000.attachSentHandler(handleSent);

  Serial.println("Marco Hider Node (v2) Ready.");
  Serial.println("Waiting for shake events...");
}

void loop() {
  // 1. Constantly cache the most recent GPS location string
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && (line.startsWith("$GPGGA") || line.startsWith("$GPRMC"))) {
      line.toCharArray(gpsBuffer, sizeof(gpsBuffer));
    }
  }

  // 2. Monitor the Accelerometer for a "Shake"
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    
    // Calculate total G-force magnitude (Gravity is 1.0G)
    float gForce = sqrt(x*x + y*y + z*z);
    
    // If we shake the board violently (> 2.0 Gs) and haven't sent a ping recently
    if (gForce > 2.0 && (millis() - lastSendTime > 3000)) {
      Serial.print("SHAKE DETECTED! G-Force: ");
      Serial.println(gForce);
      Serial.println("Transmitting GPS Payload via UWB...");
      
      transmitUWBPayload(gpsBuffer);
      lastSendTime = millis();
    }
  }
  
  // 3. Handle asynchronous transmission success
  if (sent) {
    sent = false;
    Serial.print("SUCCESS: Payload Airborne -> ");
    Serial.println(gpsBuffer);
  }
}

void transmitUWBPayload(const char* payload) {
  DW1000.newTransmit();
  DW1000.setDefaults();
  DW1000.setData(String(payload)); 
  DW1000.startTransmit();
}
