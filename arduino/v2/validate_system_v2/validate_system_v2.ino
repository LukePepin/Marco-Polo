#include <SPI.h>

const uint8_t PIN_CS  = 20; // D20
const uint8_t PIN_IRQ = 21; // D21
const uint8_t PIN_RST = 3;  // D3

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  
  // Start GPS Serial
  Serial1.begin(9600); 
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "CHECK_UWB") {
      testUWB();
    } else if (cmd == "STREAM_GPS") {
      testGPS();
    }
  }
}

void testUWB() {
  Serial.println("UWB_TEST_START");
  
  // Raw SPI test to check Dev ID without crashing any libraries
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(0x00);
  uint32_t dev_id = 0;
  for (int i=0; i<4; i++) {
    dev_id |= ((uint32_t)SPI.transfer(0x00)) << (i * 8);
  }
  digitalWrite(PIN_CS, HIGH);
  
  if (dev_id == 0xDECA0130) {
    Serial.println("UWB_STATUS: OK_DEV_ID_MATCH");
  } else if (dev_id == 0xFFFFFFFF || dev_id == 0x00000000 || dev_id == 0) {
    Serial.println("UWB_STATUS: ERROR_OFFLINE");
  } else {
    Serial.print("UWB_STATUS: ERROR_GARBAGE_0x");
    Serial.println(dev_id, HEX);
  }
  Serial.println("UWB_TEST_END");
}

void testGPS() {
  Serial.println("GPS_TEST_START");
  unsigned long start = millis();
  bool dataReceived = false;
  
  // Listen for 3 seconds
  while (millis() - start < 3000) {
    if (Serial1.available()) {
      String line = Serial1.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        Serial.print("GPS_DATA: ");
        Serial.println(line);
        dataReceived = true;
      }
    }
  }
  
  if (!dataReceived) {
    Serial.println("GPS_DATA: NO_DATA_RECEIVED");
  }
  Serial.println("GPS_TEST_END");
}
