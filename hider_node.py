import serial
import time
import threading
import sys

import serial.tools.list_ports

ARDUINO_BAUD = 115200

def find_arduino_port():
    """Scan available serial ports and return the most likely Arduino port."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    # Prioritize USB serial ports (ACM, USB, COM)
    for p in ports:
        dev = p.device.lower()
        if "acm" in dev or "usb" in dev or "com" in dev:
            return p.device
    return ports[0].device

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
    
    port = find_arduino_port()
    if not port:
        print("❌ ERROR: No serial ports detected! Plug in the Arduino via USB.")
        return
        
    try:
        arduino = serial.Serial(port, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {port}")
    except serial.SerialException as e:
        print(f"❌ Could not connect to Arduino on {port}: {e}")
        print("   Ensure it is plugged in and not open in another terminal/program.")
        print("   If this is a permission error, run: sudo usermod -a -G dialout $USER")
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
