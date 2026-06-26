import serial
import time
import threading
import json
import paho.mqtt.client as mqtt

ARDUINO_PORT = '/dev/ttyACM0'
ARDUINO_BAUD = 115200
MQTT_BROKER = 'localhost'
MQTT_PORT = 1883
MQTT_TOPIC = 'marcopolo/telemetry/hider_local'

def read_arduino(arduino, mqtt_client):
    while True:
        try:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    if line.startswith("{") and line.endswith("}"):
                        try:
                            payload = json.loads(line)
                            if "timestamp" not in payload:
                                payload["timestamp"] = int(time.time())
                            mqtt_client.publish(MQTT_TOPIC, json.dumps(payload))
                            print(f"[MQTT Published]: {json.dumps(payload)}")
                        except json.JSONDecodeError:
                            print(f"[Arduino JSON Error]: {line}")
                    else:
                        print(f"[Hider Node]: {line}")
        except Exception as e:
            print(f"\n[Connection Error]: {e}")
            break
        time.sleep(0.01)

def main():
    print("========================================")
    print("        MARCO HIDER NODE (v3)           ")
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
        
    try:
        arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {ARDUINO_PORT}")
    except serial.SerialException:
        print(f"❌ Could not connect to Arduino on {ARDUINO_PORT}. Ensure it is plugged in.")
        return

    reader_thread = threading.Thread(target=read_arduino, args=(arduino, client), daemon=True)
    reader_thread.start()

    print("\n>>> System Online. Reading local Hider states (GPS, Motion).")
    print(">>> Press Ctrl+C to exit.\n")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down Hider Node...")
    finally:
        arduino.close()
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
