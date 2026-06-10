import serial
import time
import threading

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200

def read_arduino(arduino):
    while True:
        try:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"[Hider Node]: {line}")
        except Exception as e:
            print(f"\n[Connection Error]: {e}")
            break
        time.sleep(0.01)

def main():
    print("========================================")
    print("        MARCO HIDER NODE (v2)           ")
    print("========================================")
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return

    reader_thread = threading.Thread(target=read_arduino, args=(arduino,), daemon=True)
    reader_thread.start()

    print("\n>>> System Online. Waiting for physical shake events on the hardware.")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down Hider Node...")
    finally:
        arduino.close()

if __name__ == "__main__":
    main()
