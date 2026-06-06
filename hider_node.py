import serial
import time
import threading
import sys

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200

def read_arduino(arduino):
    """Background thread to continuously print Arduino serial output."""
    while True:
        try:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"\n[Arduino]: {line}")
        except Exception as e:
            print(f"\n[Error reading serial]: {e}")
            break
        time.sleep(0.01)

def main():
    print("========================================")
    print("        HIDER NODE (UWB TX)             ")
    print("========================================")
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return

    # Start a background thread to print Arduino messages
    reader_thread = threading.Thread(target=read_arduino, args=(arduino,), daemon=True)
    reader_thread.start()

    print("\n>>> Instructions: Press ENTER to trigger a wireless PING broadcast.")
    print(">>> Press Ctrl+C to exit.\n")
    try:
        while True:
            sys.stdin.readline()
            print(">>> Sending 'SEND_PING' Command to Arduino...")
            arduino.write(b"SEND_PING\n")
    except KeyboardInterrupt:
        print("\nExiting Hider...")
    finally:
        arduino.close()

if __name__ == "__main__":
    main()
