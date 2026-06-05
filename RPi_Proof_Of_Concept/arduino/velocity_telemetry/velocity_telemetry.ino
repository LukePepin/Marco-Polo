// Velocity telemetry for Arduino Nano 33 BLE Sense
// Prints velocity data over Serial for validate_hardware2.py.
// Requires the Arduino_LSM9DS1 library.

#include <Arduino_LSM9DS1.h>

static const unsigned long kSamplePeriodMs = 100;
static const float kGravity = 9.80665f;
static const float kDamping = 0.98f;  // drift control

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

  Serial.println("Velocity telemetry running...");
  lastSampleMs = millis();
}

void loop() {
  unsigned long nowMs = millis();
  if (nowMs - lastSampleMs < kSamplePeriodMs) {
    return;
  }

  float ax, ay, az;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    // Convert g to m/s^2 and remove 1g bias (simple gravity compensation).
    float accelMps2 = (sqrt(ax * ax + ay * ay + az * az) - 1.0f) * kGravity;

    float dt = (nowMs - lastSampleMs) / 1000.0f;
    velocityMps = (velocityMps + accelMps2 * dt) * kDamping;

    Serial.print("velocity_mps: ");
    Serial.println(velocityMps, 3);
  }

  lastSampleMs = nowMs;
}
