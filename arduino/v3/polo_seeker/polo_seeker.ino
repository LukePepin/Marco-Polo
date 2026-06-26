#include <SPI.h>
#include <DW1000.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoJson.h>

const uint8_t PIN_CS  = 20; 
const uint8_t PIN_IRQ = 21; 
const uint8_t PIN_RST = 3;  

byte rxBuffer[1024]; // Large enough for JSON from Hider
char gpsBuffer[100] = "NO_GPS_LOCK_YET";
unsigned long lastLocalSendTime = 0;

void startReceiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

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
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();

  // Disable DW1000 hardware interrupts
  byte zeros[4] = {0, 0, 0, 0};
  DW1000.writeBytes(SYS_MASK, 0x00, zeros, 4);

  startReceiver();
  // We do not print extraneous strings here because Node-RED (via Python) 
  // is listening. If we print normal strings, Python will just ignore them 
  // or print them as [Arduino] logs, which is fine.
  Serial.println("Polo Seeker Node (v3) Ready.");
}

void loop() {
  // 1. Read Local GPS Data
  if (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && (line.startsWith("$GPGGA") || line.startsWith("$GPRMC"))) {
      line.toCharArray(gpsBuffer, sizeof(gpsBuffer));
    }
  }

  // 2. Poll for UWB Packets (Hider Telemetry)
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

    rxBuffer[len - 1] = '\0'; 
    
    // The received buffer IS the JSON string from the hider!
    // We just pipe it directly to Serial so Python can pick it up.
    Serial.println((char*)rxBuffer);
    
    startReceiver();
  } else if (dataReady) {
    // Bad CRC, clear and restart
    byte clear[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    DW1000.writeBytes(SYS_STATUS, 0x00, clear, 5);
    startReceiver();
  }

  // 3. Periodically output Local Seeker Telemetry
  // We output every 2 seconds
  if (millis() - lastLocalSendTime > 2000) {
    float ax=0, ay=0, az=0, gx=0, gy=0, gz=0, mx=0, my=0, mz=0;
    if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
    if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gx, gy, gz);
    if (IMU.magneticFieldAvailable()) IMU.readMagneticField(mx, my, mz);

    StaticJsonDocument<512> doc;
    doc["id"] = "seeker_1";
    doc["gps"] = gpsBuffer;
    
    JsonArray acc = doc.createNestedArray("acc");
    acc.add(ax); acc.add(ay); acc.add(az);
    
    JsonArray gyr = doc.createNestedArray("gyr");
    gyr.add(gx); gyr.add(gy); gyr.add(gz);
    
    JsonArray mag = doc.createNestedArray("mag");
    mag.add(mx); mag.add(my); mag.add(mz);

    String output;
    serializeJson(doc, output);
    
    // Print Seeker JSON to Serial
    Serial.println(output);
    
    lastLocalSendTime = millis();
  }
  
  delayMicroseconds(100);
}
