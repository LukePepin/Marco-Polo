import serial
import time

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200

def main():
    print("========================================")
    print("        SEEKER NODE (UWB RX)            ")
    print("========================================")
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return
    
    print(">>> Listening the airwaves for UWB Pings from the Hider...")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        while True:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    if "UWB_PING_DETECTED" in line:
                        print("\n🚀 >>> SUCCESS: UWB PING DETECTED WIRELESSLY FROM HIDER! <<< 🚀")
                    else:
                        print(f"[Arduino]: {line}")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nExiting Seeker...")
    finally:
        arduino.close()

if __name__ == "__main__":
    main()
