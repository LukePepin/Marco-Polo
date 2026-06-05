// Hider serial helper for hider.py
// Emits MOTION_DETECTED when velocity crosses a threshold.
// Requires the Arduino_LSM9DS1 library.

#include <Arduino_LSM9DS1.h>

static const unsigned long kSamplePeriodMs = 100;
static const float kGravity = 9.80665f;
static const float kDamping = 0.98f;  // drift control
static const float kVelocityTriggerMps = 0.25f;

float velocityMps = 0.0f;
unsigned long lastSampleMs = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for serial to connect.
  }

  if (!IMU.begin()) {
    Serial.println("IMU init failed. Check Arduino_LSM9DS1 library.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Hider serial helper running...");
  lastSampleMs = millis();
}

void loop() {
  unsigned long nowMs = millis();
  if (nowMs - lastSampleMs >= kSamplePeriodMs) {
    float ax, ay, az;
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);

      float accelMps2 = (sqrt(ax * ax + ay * ay + az * az) - 1.0f) * kGravity;
      float dt = (nowMs - lastSampleMs) / 1000.0f;
      velocityMps = (velocityMps + accelMps2 * dt) * kDamping;

      Serial.print("velocity_mps: ");
      Serial.println(velocityMps, 3);

      if (fabs(velocityMps) >= kVelocityTriggerMps) {
        Serial.println("MOTION_DETECTED");
      }
    }

    lastSampleMs = nowMs;
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "SEND_PING") {
      Serial.println("PING_SENT");
      // TODO: trigger the UWB radio ping here.
    }
  }
}
