import sys
import time
import serial
import serial.tools.list_ports

def main():
    print("============================================================")
    # Highlight Windows-specific tool
    print("      WINDOWS SERIAL BAUD & DIAGNOSTICS TESTER              ")
    print("============================================================")
    
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("\n❌ ERROR: No COM ports detected on Windows!")
        print("  - Make sure your Arduino is connected to this PC via USB.")
        print("  - Check Device Manager -> Ports (COM & LPT) to see if drivers are installed.")
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
    
    # Bauds to test
    bauds = [115200, 9600]
    
    for baud in bauds:
        print(f"\n--- Testing {port_name} at {baud} baud ---")
        try:
            ser = serial.Serial(port_name, baud, timeout=2.0)
            time.sleep(1.0) # Wait for connection stabilization
            
            print("   - Sending 'GET_STATUS\\n'...")
            ser.write(b"GET_STATUS\n")
            ser.flush()
            
            # Read output for 2 seconds
            start_time = time.time()
            received_data = []
            while time.time() - start_time < 2.5:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"   [Recv]: \"{line}\"")
                        received_data.append(line)
            
            if received_data:
                # Analyze response
                has_status = any("STATUS:" in l for l in received_data)
                has_init = any("INIT_" in l for l in received_data)
                
                if has_status:
                    print(f"   ✅ SUCCESS: Device responded to GET_STATUS at {baud} baud.")
                    for l in received_data:
                        if "STATUS: OK" in l:
                            print("      🎉 Hardware Status: UWB is OK and connected!")
                        elif "STATUS: ERROR_UWB_OFFLINE" in l:
                            print("      ❌ Hardware Status: UWB is OFFLINE! Check SPI wiring.")
                            
                    monitor_choice = input("\nDo you want to enter Live Diagnostics Monitor? (y/n): ").strip().lower()
                    if monitor_choice == 'y':
                        print("\n" + "=" * 60)
                        print("   LIVE DIAGNOSTICS MONITOR (Press Ctrl+C to exit)")
                        print("=" * 60)
                        try:
                            while True:
                                ser.reset_input_buffer()
                                ser.write(b"GET_STATUS\n")
                                ser.flush()
                                time.sleep(0.05)
                                
                                resp = None
                                s_time = time.time()
                                while time.time() - s_time < 0.5:
                                    if ser.in_waiting > 0:
                                        mline = ser.readline().decode('utf-8', errors='ignore').strip()
                                        if mline.startswith("STATUS:"):
                                            resp = mline
                                            break
                                if resp:
                                    print(f"[{port_name}] {resp}")
                                else:
                                    print(f"[{port_name}] TIMEOUT")
                                time.sleep(1.0)
                        except KeyboardInterrupt:
                            print("\nExiting Live Monitor...")
                            
                    ser.close()
                    break
                elif has_init:
                    print(f"   ⚠️ WARNING: Received initialization output but no response to status query.")
                    print("      This suggests an older firmware is running that does not support GET_STATUS.")
                else:
                    print(f"   ℹ️ Received data but it doesn't match expected diagnostics commands.")
            else:
                print("   ❌ No data received at this baud rate.")
                
            ser.close()
        except Exception as e:
            print(f"   ❌ Error opening/communicating: {e}")

if __name__ == "__main__":
    main()
