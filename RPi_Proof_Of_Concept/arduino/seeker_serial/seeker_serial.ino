// Seeker serial helper for seeker.py
// Emits UWB_PING_DETECTED and UWB_DIST: lines for the Pi.
// Replace the simulation section with actual UWB receive/range logic.

static const unsigned long kSimPingIntervalMs = 5000;
static const float kSimDistanceM = 2.5f;

unsigned long lastSimPingMs = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for serial to connect.
  }

  Serial.println("Seeker serial helper running...");
}

void loop() {
  unsigned long nowMs = millis();

  if (nowMs - lastSimPingMs >= kSimPingIntervalMs) {
    lastSimPingMs = nowMs;
    Serial.println("UWB_PING_DETECTED");
    Serial.print("UWB_DIST: ");
    Serial.println(kSimDistanceM, 2);
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "START_RANGING") {
      Serial.println("RANGING_STARTED");
      // TODO: start real UWB ranging here and emit UWB_DIST updates.
    }
  }
}
