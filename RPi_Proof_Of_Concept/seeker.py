import serial
import time

# Configuration
ARDUINO_PORT = '/dev/ttyACM0'  # Adjust based on your Pi
ARDUINO_BAUD = 115200

def setup_serial():
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"Connected to Arduino on {ARDUINO_PORT}")
        return arduino
    except serial.SerialException:
        print(f"Could not connect to Arduino on {ARDUINO_PORT}. Running without Arduino for testing.")
        return None

def main():
    print("Starting Seeker Node (UWB Only)...")
    arduino = setup_serial()
    awaiting_distance = False
    
    try:
        while True:
            # 1. Listen for Arduino messages (e.g., UWB Pings from Hider, distance results)
            if arduino and arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"Arduino says: {line}")
                    
                    # 2. Check if the Arduino reported a ping from the Hider
                    if "UWB_PING_DETECTED" in line:
                        if "[SIMULATED]" in line:
                            print(">>> Simulated UWB ping received for bench testing. <<<")
                        else:
                            print(">>> UWB Ping detected from Hider! <<<")
                        # 3. Tell the Arduino to start UWB ranging
                        command = "START_RANGING\n"
                        arduino.write(command.encode('utf-8'))
                        print("Commanded Arduino to start UWB precise ranging.")
                        awaiting_distance = True
                        
                    # If Arduino sends back distance, you can log it or display it here.
                    elif "UWB_DIST:" in line:
                        if awaiting_distance or "[SIMULATED]" in line:
                            print(f"🚀 Distance Updated: {line}")
                            awaiting_distance = False
                        else:
                            print(f"⚠️ Distance received without a preceding ping: {line}")
                    elif line == "RANGING_STARTED":
                        print("Arduino acknowledged the ranging command.")

            time.sleep(0.1) # Small delay to prevent CPU hogging

    except KeyboardInterrupt:
        print("\nExiting Seeker...")
    finally:
        if arduino: arduino.close()

if __name__ == "__main__":
    main()
