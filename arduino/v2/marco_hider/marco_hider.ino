#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>

// ---------- Custom Hardware Pins ----------
const uint8_t PIN_CS  = 20; // D20 (A6)
const uint8_t PIN_IRQ = 21; // D21 (A7)
const uint8_t PIN_RST = 3;  // D3

char gpsBuffer[100] = "NO_GPS_LOCK_YET";
unsigned long lastSendTime = 0;

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
}

void transmitUWBPayload(const char* payload) {
  DW1000.newTransmit();
  // We send the entire GPS string as a single UWB frame (max 127 bytes)
  DW1000.setData((byte*)payload, strlen(payload) + 1); 
  DW1000.startTransmit();
  
  // Wait for the transmission to finish
  unsigned long start = millis();
  while ((millis() - start) < 100) {
    byte status[5];
    DW1000.readBytes(SYS_STATUS, 0x00, status, 5);
    if (status[0] & 0x80) break; // TXFRS flag
    delayMicroseconds(50);
  }
  
  // Clear the status registers
  byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);
  
  Serial.print("SUCCESS: Payload Airborne -> ");
  Serial.println(payload);
}
