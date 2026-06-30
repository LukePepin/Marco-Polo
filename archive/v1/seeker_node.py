import serial
import time

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

def main():
    print("========================================")
    print("        SEEKER NODE (UWB RX)            ")
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
