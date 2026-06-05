import serial
import time
import os

ARDUINO_PORT = '/dev/ttyACM0'  # Commonly /dev/ttyACM0 or /dev/ttyUSB0
ARDUINO_BAUD = 115200

def check_port_exists(port):
    return os.path.exists(port)

def validate_arduino():
    print("\n--- Testing Arduino Nano Connection ---")
    if not check_port_exists(ARDUINO_PORT):
        print(f"❌ ERROR: Cannot find Arduino at {ARDUINO_PORT}.")
        print("   Make sure the USB cable is plugged in between the Pi and the Arduino.")
        return False
    
    print(f"✅ Found Arduino port at {ARDUINO_PORT}")
    try:
        # Try to open the port and read 5 lines of data
        with serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=3) as ser:
            print("✅ Successfully opened Arduino serial port. Waiting for data...")
            time.sleep(2) # Wait for Arduino to reset upon serial connection
            
            lines_read = 0
            while lines_read < 3:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"   [Arduino]: {line}")
                    lines_read += 1
            print("✅ Successfully received data from Arduino!")
            return True
    except Exception as e:
        print(f"❌ ERROR reading from Arduino: {e}")
        return False

if __name__ == "__main__":
    print("========================================")
    print("   SINGLE SYSTEM VALIDATION TOOL        ")
    print("========================================")
    print("Use this to verify your Pi -> Arduino connection")
    print("before attempting a two-node wireless test.")
    
    arduino_ok = validate_arduino()
    
    print("\n========================================")
    print("   SUMMARY                              ")
    print("========================================")
    print(f"Arduino Nano Connection : {'✅ PASS' if arduino_ok else '❌ FAIL'}")
    if arduino_ok:
        print("Your single-system hardware is ready.")
    print("========================================")
