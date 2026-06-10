#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>

const uint8_t PIN_CS  = 20; 
const uint8_t PIN_IRQ = 21; 
const uint8_t PIN_RST = 3;  

char gpsBuffer[100] = "NO_GPS_LOCK_YET";
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial1.begin(9600);
  
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU! Check hardware.");
    while (1);
  }

  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_CS);
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(2);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // CRITICAL FIX: Disable DW1000 hardware interrupts entirely!
  // This prevents the library from clearing the registers in the background,
  // allowing our manual polling loop to catch the TXFRS success flag.
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, 0x00, zeros, 4);

  Serial.println("Marco Hider Node (v2) Ready.");
  Serial.println("Waiting for shake events...");
}

void loop() {
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && (line.startsWith("$GPGGA") || line.startsWith("$GPRMC"))) {
      line.toCharArray(gpsBuffer, sizeof(gpsBuffer));
    }
  }

  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    float gForce = sqrt(x*x + y*y + z*z);
    
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
  DW1000.setDefaults();
  DW1000.setData(String(payload)); 
  DW1000.startTransmit();
  
  // Manual Polling for TX Complete
  unsigned long start = millis();
  bool txok = false;
  while ((millis() - start) < 100) {
    byte status[5];
    DW1000.readBytes(SYS_STATUS, 0x00, status, 5);
    if (status[0] & 0x80) { // Check TXFRS flag
      txok = true; 
      break; 
    }
    delayMicroseconds(50);
  }
  
  // Clear the status registers
  byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);
  
  if (txok) {
    Serial.print("SUCCESS: Payload Airborne -> ");
    Serial.println(payload);
  } else {
    Serial.println("ERROR: TX Timeout!");
  }
}
