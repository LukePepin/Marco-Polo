// Seeker serial helper for seeker.py
// Replace the simulation section with actual UWB receive/range logic.

static const float kSimDistanceM = 2.5f;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for serial to connect.
  }

  Serial.println("Seeker serial helper running...");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "START_RANGING") {
      Serial.println("RANGING_STARTED");
      // TODO: start real UWB ranging here and emit UWB_DIST updates.
    } else if (command == "SIMULATE_PING") {
      Serial.println("UWB_PING_DETECTED [SIMULATED]");
      Serial.print("UWB_DIST: ");
      Serial.print(kSimDistanceM, 2);
      Serial.println(" [SIMULATED]");
    } else if (command.startsWith("SIMULATE_DIST")) {
      int spaceIndex = command.indexOf(' ');
      float distanceM = (spaceIndex > 0) ? command.substring(spaceIndex + 1).toFloat() : kSimDistanceM;
      Serial.print("UWB_DIST: ");
      Serial.print(distanceM, 2);
      Serial.println(" [SIMULATED]");
    }
  }
}
