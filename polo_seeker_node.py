import serial
import time
import json
import paho.mqtt.client as mqtt

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200
MQTT_BROKER = 'localhost'
MQTT_PORT = 1883
MQTT_TOPIC = 'marcopolo/telemetry/seeker'

def parse_nmea_to_decimal(sentence):
    """Converts NMEA GGA/RMC to decimal degrees for JSON."""
    try:
        parts = sentence.split(',')
        # GPRMC parsing
        if sentence.startswith("$GPRMC") and len(parts) >= 6:
            if parts[2] == 'A' or parts[2] == 'V': # Valid or Invalid
                lat_raw = parts[3]
                lat_dir = parts[4]
                lon_raw = parts[5]
                lon_dir = parts[6]
                
                if lat_raw and lon_raw:
                    lat_deg = float(lat_raw[:2])
                    lat_min = float(lat_raw[2:])
                    lat = lat_deg + (lat_min / 60.0)
                    if lat_dir == 'S': lat = -lat

                    lon_deg = float(lon_raw[:3])
                    lon_min = float(lon_raw[3:])
                    lon = lon_deg + (lon_min / 60.0)
                    if lon_dir == 'W': lon = -lon

                    return True, lat, lon
        
        # GPGGA parsing
        elif sentence.startswith("$GPGGA") and len(parts) >= 6:
            if parts[2] and parts[4]:
                lat_raw = parts[2]
                lat_dir = parts[3]
                lon_raw = parts[4]
                lon_dir = parts[5]

                lat_deg = float(lat_raw[:2])
                lat_min = float(lat_raw[2:])
                lat = lat_deg + (lat_min / 60.0)
                if lat_dir == 'S': lat = -lat

                lon_deg = float(lon_raw[:3])
                lon_min = float(lon_raw[3:])
                lon = lon_deg + (lon_min / 60.0)
                if lon_dir == 'W': lon = -lon

                return True, lat, lon
    except Exception:
        pass
    
    # Return fake LA coordinates if GPS string is invalid, just so Node-RED doesn't crash during testing
    # In production, you might want to return False, 0.0, 0.0
    return False, 34.0522, -118.2437

def main():
    print("========================================")
    print("        POLO SEEKER NODE (v3)           ")
    print("========================================")
    
    # Setup MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        print(f"✅ Connected to MQTT Broker on {MQTT_BROKER}:{MQTT_PORT}")
    except Exception as e:
        print(f"❌ MQTT Connection Error: {e}")
        return

    # Setup Serial
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return
    
    print(">>> Listening for Packets from the Hider...")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        while True:
            try:
                if arduino.in_waiting > 0:
                    line = arduino.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Direct JSON from newer Arduino firmware
                        if line.startswith("{") and line.endswith("}"):
                            try:
                                payload = json.loads(line)
                                
                                # 1. Map ID to device_id for Node-RED
                                if "id" in payload:
                                    payload["device_id"] = payload["id"]
                                    
                                # 2. Parse the NMEA string to extract Latitude/Longitude
                                if "gps" in payload:
                                    gps_valid, lat, lon = parse_nmea_to_decimal(payload["gps"])
                                    payload["gps_valid"] = gps_valid
                                    payload["latitude"] = lat
                                    payload["longitude"] = lon
                                    
                                # 3. Ensure timestamp exists
                                if "timestamp" not in payload:
                                    payload["timestamp"] = int(time.time())
                                    
                                client.publish(MQTT_TOPIC, json.dumps(payload))
                                print(f"[MQTT Published]: {json.dumps(payload)}")
                            except json.JSONDecodeError:
                                print(f"[Arduino JSON Error]: {line}")
                                
                        else:
                            print(f"[Arduino]: {line}")

                time.sleep(0.01)
            except (OSError, serial.SerialException) as e:
                print(f"\n[Hardware Error]: Arduino disconnected unexpectedly! ({e})")
                print("Please check the USB connection and restart the script.")
                break

    except KeyboardInterrupt:
        print("\nShutting down Seeker Node...")
    finally:
        arduino.close()
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
