import serial
import time

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200

def parse_nmea_coordinates(sentence):
    """A basic parser to extract readable Lat/Lon from raw NMEA strings."""
    try:
        parts = sentence.split(',')
        if sentence.startswith("$GPGGA") and len(parts) >= 6:
            if parts[2] and parts[4]:
                lat = f"{parts[2][:2]}°{parts[2][2:]}' {parts[3]}"
                lon = f"{parts[4][:3]}°{parts[4][3:]}' {parts[5]}"
                return f"Lat: {lat} | Lon: {lon}"
            else:
                return "GPS String Received, but Hider doesn't have a satellite lock yet."
    except Exception:
        pass
    return f"Raw Data: {sentence}"

def main():
    print("========================================")
    print("        POLO SEEKER NODE (v2)           ")
    print("========================================")
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return
    
    print(">>> Listening the airwaves for UWB Packets from the Hider...")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        while True:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    if line.startswith("UWB_PAYLOAD:"):
                        payload = line.split("UWB_PAYLOAD:")[1]
                        print("\n" + "="*50)
                        print("🚨 WIRELESS ASSET LOCATION DETECTED 🚨")
                        print("="*50)
                        parsed_data = parse_nmea_coordinates(payload)
                        print(f"📍 Location Data: {parsed_data}")
                        print("="*50 + "\n")
                    else:
                        print(f"[Arduino]: {line}")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nShutting down Seeker Node...")
    finally:
        arduino.close()

if __name__ == "__main__":
    main()
