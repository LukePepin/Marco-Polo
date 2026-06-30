import sys
import time
import serial
import serial.tools.list_ports

def main():
    print("============================================================")
    print("      WINDOWS GPS HARDWARE VALIDATION TOOL                  ")
    print("============================================================")
    
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("\n❌ ERROR: No COM ports detected on Windows!")
        print("  - Make sure your Arduino is connected to this PC via USB.")
        return
        
    print("Available COM ports:")
    for idx, p in enumerate(ports):
        print(f"  [{idx}] {p.device} - {p.description}")
        
    try:
        choice = input("\nSelect a COM port number (e.g. 0): ").strip()
        selected_idx = int(choice)
        if selected_idx < 0 or selected_idx >= len(ports):
            print("Invalid selection.")
            return
    except ValueError:
        print("Please enter a valid number.")
        return
        
    port_name = ports[selected_idx].device
    baud_rate = 115200
    
    print(f"\n--- Listening for GPS NMEA sentences on {port_name} at {baud_rate} baud ---")
    print(">>> Take your setup outside if you want to get a satellite fix.")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        ser = serial.Serial(port_name, baud_rate, timeout=1.0)
        time.sleep(1.0) # Wait for connection stabilization
        
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(line)
            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nExiting GPS Validator...")
    except Exception as e:
        print(f"\n❌ Error opening/communicating: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
