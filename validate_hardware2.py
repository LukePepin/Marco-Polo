import os
import re
import time

import serial

ARDUINO_PORT = "/dev/ttyACM0"  # Commonly /dev/ttyACM0 or /dev/ttyUSB0
ARDUINO_BAUD = 115200
DURATION_SEC = 10

VELOCITY_KEYWORDS = ("vel", "velocity")


def check_port_exists(port):
    return os.path.exists(port)


def parse_velocity(line):
    lowered = line.lower()
    if not any(keyword in lowered for keyword in VELOCITY_KEYWORDS):
        return None

    # Look for a numeric value after a velocity keyword.
    match = re.search(
        r"(?:vel(?:ocity)?)[^0-9+\-]*([+\-]?\d+(?:\.\d+)?)",
        lowered,
    )
    if match:
        return float(match.group(1))

    # Fallback: if the line mentions velocity but the regex failed, try any float.
    match = re.search(r"([+\-]?\d+(?:\.\d+)?)", lowered)
    if match:
        return float(match.group(1))

    return None


def collect_velocity_samples():
    print("\n--- Collecting Velocity Samples ---")
    if not check_port_exists(ARDUINO_PORT):
        print(f"❌ ERROR: Cannot find Arduino at {ARDUINO_PORT}.")
        print("   Make sure the USB cable is plugged in between the Pi and the Arduino.")
        return []

    print(f"✅ Found Arduino port at {ARDUINO_PORT}")
    samples = []

    try:
        with serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1) as ser:
            print("✅ Successfully opened Arduino serial port.")
            print(f"Listening for velocity data for {DURATION_SEC} seconds...")
            time.sleep(2)  # Wait for Arduino to reset upon serial connection

            end_time = time.time() + DURATION_SEC
            while time.time() < end_time:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                velocity = parse_velocity(line)
                if velocity is not None:
                    samples.append(velocity)
                    print(f"   [Velocity]: {velocity}")

        return samples
    except Exception as exc:
        print(f"❌ ERROR reading from Arduino: {exc}")
        return []


def summarize_samples(samples):
    if not samples:
        print("\nNo velocity samples were detected.")
        print("Make sure the Arduino is outputting a line with 'VEL' or 'velocity'.")
        return

    avg_value = sum(samples) / len(samples)
    min_value = min(samples)
    max_value = max(samples)

    print("\n========================================")
    print("   VELOCITY SUMMARY                      ")
    print("========================================")
    print(f"Samples collected : {len(samples)}")
    print(f"Average velocity  : {avg_value:.3f}")
    print(f"Minimum velocity  : {min_value:.3f}")
    print(f"Maximum velocity  : {max_value:.3f}")
    print("========================================")


if __name__ == "__main__":
    print("========================================")
    print("   VELOCITY VALIDATION TOOL             ")
    print("========================================")
    print(
        "Reads velocity data from the Arduino for 10 seconds and reports average, min, and max."
    )

    velocity_samples = collect_velocity_samples()
    summarize_samples(velocity_samples)
