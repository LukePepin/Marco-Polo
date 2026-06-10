import sys
import time
import serial
import serial.tools.list_ports

def main():
    print("============================================================")
    print("      WINDOWS V2 SYSTEM VALIDATION TOOL (UWB + GPS)         ")
    print("============================================================")
    
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("\n❌ ERROR: No COM ports detected!")
        return
        
    print("Available COM ports:")
    for idx, p in enumerate(ports):
        print(f"  [{idx}] {p.device} - {p.description}")
        
    try:
        choice = input("\nSelect a COM port number (e.g. 0): ").strip()
        selected_idx = int(choice)
        port_name = ports[selected_idx].device
    except:
        return
        
    try:
        ser = serial.Serial(port_name, 115200, timeout=1.0)
        time.sleep(2.0) # Wait for reboot
        
        # TEST 1: UWB
        print("\n--- TEST 1: UWB HARDWARE CHECK (Pins: D20, D21, D3) ---")
        ser.write(b"CHECK_UWB\n")
        ser.flush()
        
        uwb_pass = False
        start_time = time.time()
        while time.time() - start_time < 3.0:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("UWB_STATUS:"):
                    status = line.split("UWB_STATUS:")[1].strip()
                    if status == "OK_DEV_ID_MATCH":
                        print("  ✅ PASS: DW1000 Silicon ID successfully read! UWB wiring is perfect.")
                        uwb_pass = True
                    elif status == "ERROR_OFFLINE":
                        print("  ❌ FAIL: UWB is completely offline. Check Power, GND, and D20 (CS).")
                    else:
                        print(f"  ⚠️ FAIL: Unknown error or garbage data: {status}")
                elif line == "UWB_TEST_END":
                    break
        
        if not uwb_pass:
            print("  -> UWB Test Failed.")
            
        # TEST 2: GPS
        print("\n--- TEST 2: GPS DATA STREAM CHECK (Serial1) ---")
        print("  (Listening for 3 seconds...)")
        ser.write(b"STREAM_GPS\n")
        ser.flush()
        
        gps_pass = False
        start_time = time.time()
        while time.time() - start_time < 5.0: # give it time
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("GPS_DATA:"):
                    data = line.split("GPS_DATA:")[1].strip()
                    if data == "NO_DATA_RECEIVED":
                        print("  ❌ FAIL: No GPS data received. Check TX/RX on D0/D1.")
                    else:
                        print(f"  ✅ [GPS]: {data}")
                        gps_pass = True
                elif line == "GPS_TEST_END":
                    break
                    
        if gps_pass:
            print("  ✅ PASS: NMEA data is successfully flowing from the GPS module!")
            
        print("\n============================================================")
        if uwb_pass and gps_pass:
            print("CONCLUSION: All hardware is perfectly wired and fully functional!")
            print("If your v2 integration scripts fail, the issue is software-based (IMU).")
        else:
            print("CONCLUSION: Hardware failure detected. Fix the wiring before continuing.")
        print("============================================================")

        ser.close()
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()
