import sys
import time
import serial
import serial.tools.list_ports

def print_header(title):
    print("=" * 60)
    print(f" {title.center(58)} ")
    print("=" * 60)

def main():
    print_header("MARCO POLO: SYSTEM VERIFICATION TOOL")
    print("Scanning for connected Arduino boards...")
    
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("\n❌ ERROR: No serial ports detected!")
        print("  - Ensure your Arduinos are connected via USB.")
        print("  - If running on a Pi, check 'dmesg | grep tty' to confirm the device node.")
        sys.exit(1)
        
    print(f"Found {len(ports)} serial port(s). Querying status...\n")
    
    verified_nodes = 0
    
    for p in ports:
        port_name = p.device
        port_desc = p.description
        print(f"🔍 Testing {port_name} ({port_desc})...")
        
        try:
            print("   [1/5] Opening serial port...")
            ser = serial.Serial(port_name, 115200, timeout=1.5, write_timeout=1)
            
            print("   [2/5] Waiting for connection to stabilize...")
            time.sleep(1.0)  # Wait for port to stabilize
            
            print("   [3/5] Clearing input/output buffers...")
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            print("   [4/5] Sending GET_STATUS command...")
            ser.write(b"GET_STATUS\n")
            ser.flush()
            
            print("   [5/5] Reading response...")
            response = None
            start_time = time.time()
            while time.time() - start_time < 2.0:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        if line.startswith("STATUS:"):
                            response = line
                            break
                        # If we hit an init message directly, retry the status command
                        elif "INIT_SUCCESS" in line or "INIT_FAILURE" in line:
                            print(f"   (Detected boot message: \"{line}\", retrying status...)")
                            ser.write(b"GET_STATUS\n")
                            ser.flush()
            
            if response:
                print(f"   ↳ Response: \"{response}\"")
                if "STATUS: OK" in response:
                    print(f"   ✅ SUCCESS: Node is online and responding.")
                    verified_nodes += 1
                elif "STATUS: ERROR_UWB_OFFLINE" in response:
                    print(f"   ❌ ERROR: Arduino is online but the UWB (DW1000) chip is OFFLINE.")
                    print("      Check your SPI wiring (MOSI, MISO, SCK, CS, RST, IRQ) and power.")
                else:
                    print(f"   ⚠️ WARNING: Received unexpected status: {response}")
            else:
                # Try sending one more time in case it was slow to boot
                print("   (No initial response. Retrying GET_STATUS query...)")
                ser.write(b"GET_STATUS\n")
                ser.flush()
                time.sleep(0.5)
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line.startswith("STATUS:"):
                        print(f"   ↳ Response (retry): \"{line}\"")
                        if "STATUS: OK" in line:
                            print(f"   ✅ SUCCESS: Node is online and responding.")
                            verified_nodes += 1
                        else:
                            print(f"   ❌ ERROR: UWB is offline on this node ({line})")
                    else:
                        print(f"   ❌ ERROR: No response to GET_STATUS query (Got: \"{line}\").")
                        print("      Check if the correct sketch is uploaded.")
                else:
                    print("   ❌ ERROR: No response to GET_STATUS query. Timed out.")
                    print("      Ensure the Arduino has the correct sketch uploaded and is running.")
                    
            print("   Closing serial port...")
            ser.close()
        except Exception as e:
            print(f"   ❌ ERROR: Could not communicate with port: {e}")
        print("-" * 60)
        
    print("\n" + "=" * 60)
    print("   VERIFICATION SUMMARY")
    print("=" * 60)
    print(f"Total serial ports scanned: {len(ports)}")
    print(f"Fully functional UWB nodes verified: {verified_nodes}")
    if verified_nodes < len(ports):
        print("⚠️  Some nodes failed verification. Please review the errors above.")
    else:
        print("🎉 All connected nodes are verified and ready for action!")
    print("=" * 60)

if __name__ == "__main__":
    main()
