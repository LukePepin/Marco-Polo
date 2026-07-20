void setup() {
  // Start the serial communication at 115200 baud
  Serial.begin(115200);
  
  // Wait for the serial port to connect (required for Native USB on Nano 33 BLE)
  while (!Serial);

  Serial.println("=========================================");
  Serial.println("✅ ARDUINO IS ALIVE AND WORKING!");
  Serial.println("Type any message in the box above and hit Enter...");
  Serial.println("=========================================\n");
}

void loop() {
  // Check if you typed something in the Serial Monitor
  if (Serial.available() > 0) {
    // Read whatever you typed
    String incomingMessage = Serial.readStringUntil('\n');
    incomingMessage.trim(); // Remove invisible newline characters

    // If it's not empty, reply back to you!
    if (incomingMessage.length() > 0) {
      Serial.print("🤖 Arduino says: I received your message: \"");
      Serial.print(incomingMessage);
      Serial.println("\"");
    }
  }
}
