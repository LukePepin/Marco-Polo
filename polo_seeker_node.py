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
    """Converts NMEA GGA to decimal degrees for JSON."""
    try:
        parts = sentence.split(',')
        if sentence.startswith("$GPGGA") and len(parts) >= 6:
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
    return False, 0.0, 0.0

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
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    # In V3, we assume the Arduino might send JSON directly or legacy UWB_PAYLOAD
                    if line.startswith("{") and line.endswith("}"):
                        # Direct JSON from newer Arduino firmware
                        try:
                            payload = json.loads(line)
                            # Ensure timestamp is set if not provided
                            if "timestamp" not in payload:
                                payload["timestamp"] = int(time.time())
                            client.publish(MQTT_TOPIC, json.dumps(payload))
                            print(f"[MQTT Published]: {json.dumps(payload)}")
                        except json.JSONDecodeError:
                            print(f"[Arduino JSON Error]: {line}")
                    
                    elif line.startswith("PAYLOAD:"):
                        # Legacy string payload (simulate LoRa/UWB NMEA parsing)
                        raw_data = line.split("PAYLOAD:")[1]
                        gps_valid, lat, lon = parse_nmea_to_decimal(raw_data)
                        
                        telemetry = {
                            "device_id": "hider_1",
                            "timestamp": int(time.time()),
                            "gps_valid": gps_valid,
                            "latitude": lat,
                            "longitude": lon,
                            "lora_rssi": -80, # Placeholder
                            "ble_rssi": -65,  # Placeholder
                            "motion_detected": False,
                            "battery_v": 3.9
                        }
                        client.publish(MQTT_TOPIC, json.dumps(telemetry))
                        print(f"[MQTT Published]: {json.dumps(telemetry)}")
                    else:
                        print(f"[Arduino]: {line}")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nShutting down Seeker Node...")
    finally:
        arduino.close()
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
