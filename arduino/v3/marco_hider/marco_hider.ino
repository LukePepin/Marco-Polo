#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoJson.h>

const uint8_t PIN_CS  = 20; 
const uint8_t PIN_IRQ = 21; 
const uint8_t PIN_RST = 3;  

char gpsBuffer[100] = "NO_GPS_LOCK_YET";
unsigned long lastSendTime = 0;

// Adjust threshold as needed
const float MOVEMENT_THRESHOLD = 1.5; 

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
  // Using LONGDATA to support larger JSON payloads (up to ~1023 bytes)
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Disable DW1000 hardware interrupts
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, 0x00, zeros, 4);

  Serial.println("Marco Hider Node (v3) Ready.");
  Serial.println("Waiting for movement...");
}

void loop() {
  // 1. Read incoming GPS data
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && (line.startsWith("$GPGGA") || line.startsWith("$GPRMC"))) {
      line.toCharArray(gpsBuffer, sizeof(gpsBuffer));
    }
  }

  // 2. Read IMU data
  float ax = 0, ay = 0, az = 0;
  float gx = 0, gy = 0, gz = 0;
  float mx = 0, my = 0, mz = 0;

  if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
  if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gx, gy, gz);
  if (IMU.magneticFieldAvailable()) IMU.readMagneticField(mx, my, mz);

  // 3. Check for movement
  float gForce = sqrt(ax*ax + ay*ay + az*az);
  
  if (gForce > MOVEMENT_THRESHOLD && (millis() - lastSendTime > 2000)) {
    Serial.println("MOVEMENT DETECTED! Generating JSON payload...");
    
    // Allocate JsonDocument
    // Size 512 is plenty for this data
    StaticJsonDocument<512> doc;
    
    doc["id"] = "hider_1";
    doc["gps"] = gpsBuffer;
    
    JsonArray acc = doc.createNestedArray("acc");
    acc.add(ax); acc.add(ay); acc.add(az);
    
    JsonArray gyr = doc.createNestedArray("gyr");
    gyr.add(gx); gyr.add(gy); gyr.add(gz);
    
    JsonArray mag = doc.createNestedArray("mag");
    mag.add(mx); mag.add(my); mag.add(mz);

    // Serialize to string
    String output;
    serializeJson(doc, output);
    
    Serial.print("Broadcasting: ");
    Serial.println(output);
    
    transmitUWBPayload(output.c_str());
    lastSendTime = millis();
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
  while ((millis() - start) < 150) {
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
    Serial.println("--> TX Success");
  } else {
    Serial.println("--> TX Timeout!");
  }
}
