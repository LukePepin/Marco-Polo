// UWB Pins (Kept here for documentation, they don't need to be disconnected)
const uint8_t PIN_CS  = A1;
const uint8_t PIN_IRQ = A4;
const uint8_t PIN_RST = 7;

void setup() {
  // 1. Start the USB Serial communication to the Raspberry Pi
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println("========================================");
  Serial.println("        GPS VALIDATION SCRIPT           ");
  Serial.println("========================================");
  
  // 2. Start the Hardware Serial communication to the GPS Module
  // The Adafruit Ultimate GPS breakout v3 defaults to 9600 baud
  Serial1.begin(9600);
  Serial.println("Listening to GPS on Hardware Serial1 (RX=D0, TX=D1)...");
  Serial.println("Waiting for NMEA sentences...\n");
}

void loop() {
  // If the GPS module sends data to the Arduino...
  if (Serial1.available()) {
    // Read the full sentence
    String gpsSentence = Serial1.readStringUntil('\n');
    gpsSentence.trim();
    
    // Forward it directly to the Raspberry Pi over USB
    if (gpsSentence.length() > 0) {
      Serial.print("[GPS]: ");
      Serial.println(gpsSentence);
    }
  }
}
