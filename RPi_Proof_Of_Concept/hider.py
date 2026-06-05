import serial
import time
import threading

# Configuration
ARDUINO_PORT = '/dev/ttyACM0'  # Adjust based on your Pi (e.g., /dev/ttyUSB0)
ARDUINO_BAUD = 115200

VELOCITY_THRESHOLD_TRIGGER = "MOTION_DETECTED" # The string expected from Arduino

def setup_serial():
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"Connected to Arduino on {ARDUINO_PORT}")
        return arduino
    except serial.SerialException:
        print(f"Could not connect to Arduino on {ARDUINO_PORT}. Running without Arduino for testing.")
        return None

def send_pings(arduino):
    """Commands the Arduino to send 5 pings via UWB with a 10 second pause between each."""
    print("Threshold exceeded! Commanding Arduino to initiate UWB ping sequence...")
    for i in range(5):
        if arduino:
            arduino.write(b"SEND_PING\n")
            print(f"Commanded Arduino: SEND_PING ({i+1}/5)")
        else:
            print(f"Simulated Command: SEND_PING ({i+1}/5)")
            
        if i < 4:  # Don't sleep after the last ping
            print("Waiting 10 seconds...")
            time.sleep(10)
    print("Ping sequence complete. Resuming monitoring.")

def main():
    print("Starting Hider Node (UWB Only)...")
    arduino = setup_serial()
    
    try:
        while True:
            # 1. Read from Arduino
            if arduino and arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"Arduino says: {line}")
                    
                    # 2. Check for trigger condition
                    if VELOCITY_THRESHOLD_TRIGGER in line:
                        # 3. Execute the ping sequence
                        # Using a thread so we don't block reading other important Arduino messages 
                        ping_thread = threading.Thread(target=send_pings, args=(arduino,))
                        ping_thread.start()
            
            time.sleep(0.1) # Small delay to prevent CPU hogging

    except KeyboardInterrupt:
        print("\nExiting Hider...")
    finally:
        if arduino: arduino.close()

if __name__ == "__main__":
    main()
