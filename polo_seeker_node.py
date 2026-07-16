import serial
import time
import json
import statistics
import sys
from collections import deque
import paho.mqtt.client as mqtt
import serial.tools.list_ports

# ============================================================
# POLO SEEKER NODE v4 — with UWB distance/RSSI filtering
#
# WHAT CHANGED FROM v3:
#   The Arduino firmware now reports two new fields in its JSON:
#       distance_m  — real distance from Two-Way Ranging
#       rssi_dbm    — received signal power
#
#   Both are noisy. This script smooths them BEFORE they go to MQTT,
#   so everything downstream (Node-RED -> SQLite -> dashboard) gets
#   clean values.
#
#   Raw values are preserved alongside the filtered ones, so the
#   before/after comparison can be shown in the report.
#
# WHY MEDIAN FOR DISTANCE:
#   The TWR distance noise is symmetric scatter caused by the Hider's
#   reply delay varying between roughly 340M and 490M DW1000 ticks.
#   Longer reply delay -> larger reported distance. A median filter is
#   brutal on that kind of jitter and does not get dragged around by
#   the outliers the way a mean does.
#
# WHY MOVING AVERAGE FOR RSSI:
#   RSSI noise is closer to gaussian, so averaging suits it better.
# ============================================================

# Port is auto-detected in main()
ARDUINO_BAUD = 115200
MQTT_BROKER  = 'localhost'
MQTT_PORT    = 1883
MQTT_TOPIC   = 'marcopolo/telemetry/seeker'

# ---------- filter configuration ----------
# Window size is the core tradeoff:
#   LARGER  = smoother output, but slower to react to real movement (lag)
#   SMALLER = responsive, but noisier
# At 1 reading/sec, a window of 5 means the filter reflects the last ~5
# seconds. Good for a person walking. Raise it if the output is still
# too jumpy; lower it if the tracking feels sluggish.
DISTANCE_WINDOW = 5     # median filter — kills the reply-delay jitter
RSSI_WINDOW     = 5     # moving average — smooths signal power

# Sanity gate: reject physically impossible distances outright so a single
# glitched reading never poisons the filter buffer.
DISTANCE_MIN_M = -1.0
DISTANCE_MAX_M = 60.0


class SignalFilter:
    """Rolling filters for the UWB distance and RSSI streams."""

    def __init__(self, distance_window=DISTANCE_WINDOW, rssi_window=RSSI_WINDOW):
        self.distance_buf = deque(maxlen=distance_window)
        self.rssi_buf     = deque(maxlen=rssi_window)
        self.rejected     = 0
        self.accepted     = 0

    def filter_distance(self, raw):
        """
        Median filter. Returns (filtered_value, is_valid).

        Returns None while the buffer is still filling — better to report
        nothing than to report a 'filtered' value based on one sample.
        """
        if raw is None:
            return None, False

        # Reject impossible readings before they contaminate the buffer
        if not (DISTANCE_MIN_M < raw < DISTANCE_MAX_M):
            self.rejected += 1
            return None, False

        self.distance_buf.append(raw)
        self.accepted += 1

        # Wait until we have a meaningful sample before claiming a filtered value
        if len(self.distance_buf) < 3:
            return None, False

        return statistics.median(self.distance_buf), True

    def filter_rssi(self, raw):
        """Moving average. Returns (filtered_value, is_valid)."""
        if raw is None:
            return None, False

        self.rssi_buf.append(raw)

        if len(self.rssi_buf) < 3:
            return None, False

        return sum(self.rssi_buf) / len(self.rssi_buf), True

    def stats(self):
        total = self.accepted + self.rejected
        if total == 0:
            return "no readings yet"
        pct = (self.rejected / total) * 100
        return f"{self.accepted} accepted, {self.rejected} rejected ({pct:.1f}%)"


def parse_nmea_to_decimal(sentence):
    """Converts NMEA GGA/RMC to decimal degrees for JSON."""
    try:
        parts = sentence.split(',')

        if sentence.startswith("$GPRMC") and len(parts) >= 6:
            if parts[2] in ('A', 'V'):
                lat_raw, lat_dir = parts[3], parts[4]
                lon_raw, lon_dir = parts[5], parts[6]

                if lat_raw and lon_raw:
                    lat = float(lat_raw[:2]) + (float(lat_raw[2:]) / 60.0)
                    if lat_dir == 'S':
                        lat = -lat

                    lon = float(lon_raw[:3]) + (float(lon_raw[3:]) / 60.0)
                    if lon_dir == 'W':
                        lon = -lon

                    return True, lat, lon

        elif sentence.startswith("$GPGGA") and len(parts) >= 6:
            if parts[2] and parts[4]:
                lat_raw, lat_dir = parts[2], parts[3]
                lon_raw, lon_dir = parts[4], parts[5]

                lat = float(lat_raw[:2]) + (float(lat_raw[2:]) / 60.0)
                if lat_dir == 'S':
                    lat = -lat

                lon = float(lon_raw[:3]) + (float(lon_raw[3:]) / 60.0)
                if lon_dir == 'W':
                    lon = -lon

                return True, lat, lon
    except Exception:
        pass

    # Fallback so Node-RED doesn't choke during indoor testing (no GPS lock)
    return False, 34.0522, -118.2437


def find_arduino_port():
    """Auto-detects the Arduino by checking available serial ports."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
        
    for p in ports:
        desc = p.description.lower()
        # Look for common Arduino Nano 33 BLE identifiers or standard ACM ports
        if 'arduino' in desc or 'mbed' in desc or 'nano' in desc or 'ttyacm' in p.device.lower() or 'ttyusb' in p.device.lower() or 'com' in p.device.lower():
            return p.device
            
    # Fallback to the first available port
    return ports[0].device


def main():
    print("=" * 44)
    print("     POLO SEEKER NODE (v4 — filtered)")
    print("=" * 44)
    print(f"Distance filter : median, window={DISTANCE_WINDOW}")
    print(f"RSSI filter     : moving average, window={RSSI_WINDOW}")
    print("=" * 44)

    # The Hider is the only node we range against, so one filter instance
    # is enough. If more tags are added later, key this by device_id.
    sig = SignalFilter()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        print(f"Connected to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}")
    except Exception as e:
        print(f"MQTT connection error: {e}")
        return

    arduino_port = find_arduino_port()
    if not arduino_port:
        print("❌ Could not find any connected Arduinos. Is it plugged in?")
        sys.exit(1)

    try:
        arduino = serial.Serial(arduino_port, ARDUINO_BAUD, timeout=1)
        print(f"✅ Connected to Arduino on {arduino_port}")
    except serial.SerialException:
        print(f"❌ Could not open {arduino_port}.")
        print("   (Check that no other script or the Arduino IDE Serial Monitor is holding the port.)")
        sys.exit(1)

    print("\nListening for packets. Ctrl+C to exit.\n")

    msg_count = 0

    try:
        while True:
            try:
                if arduino.in_waiting > 0:
                    line = arduino.readline().decode('utf-8', errors='ignore').strip()

                    if not line:
                        continue

                    if line.startswith("{") and line.endswith("}"):
                        try:
                            payload = json.loads(line)

                            # Map id -> device_id for Node-RED
                            if "id" in payload:
                                payload["device_id"] = payload["id"]

                            # Parse the NMEA string into lat/lon
                            if "gps" in payload:
                                gps_valid, lat, lon = parse_nmea_to_decimal(payload["gps"])
                                payload["gps_valid"] = gps_valid
                                payload["latitude"]  = lat
                                payload["longitude"] = lon

                            # ==========================================
                            # THE FILTERING — only applies to Hider
                            # packets, since distance/RSSI describe the
                            # radio link between Seeker and Hider.
                            # ==========================================
                            if payload.get("id") == "hider_1":

                                raw_dist = payload.get("distance_m")
                                raw_rssi = payload.get("rssi_dbm")

                                filt_dist, dist_ok = sig.filter_distance(raw_dist)
                                filt_rssi, rssi_ok = sig.filter_rssi(raw_rssi)

                                # Keep the raw values AND add the filtered ones.
                                # Having both lets the report show before/after.
                                payload["distance_m_raw"]      = raw_dist
                                payload["distance_m_filtered"] = filt_dist
                                payload["rssi_dbm_raw"]        = raw_rssi
                                payload["rssi_dbm_filtered"]   = filt_rssi

                                # The dashboard and DB should use the filtered
                                # distance as the primary value. Fall back to raw
                                # while the filter buffer is still filling.
                                if dist_ok:
                                    payload["distance_m"] = filt_dist

                            # Timestamp
                            if "timestamp" not in payload:
                                payload["timestamp"] = int(time.time())

                            client.publish(MQTT_TOPIC, json.dumps(payload))
                            msg_count += 1

                            # Readable console output — show raw vs filtered
                            if payload.get("id") == "hider_1":
                                r = payload.get("distance_m_raw")
                                f = payload.get("distance_m_filtered")
                                rs = payload.get("rssi_dbm_filtered")

                                r_s = f"{r:6.2f}" if r is not None else "  ----"
                                f_s = f"{f:6.2f}" if f is not None else "  ----"
                                rs_s = f"{rs:6.1f}" if rs is not None else "  ----"

                                print(f"[HIDER ] raw={r_s} m   filtered={f_s} m   rssi={rs_s} dBm")
                            else:
                                print(f"[SEEKER] telemetry published")

                            # Periodic filter health report
                            if msg_count % 50 == 0:
                                print(f"\n--- filter stats: {sig.stats()} ---\n")

                        except json.JSONDecodeError:
                            print(f"[JSON error] {line}")

                    else:
                        # Non-JSON lines from the Arduino ([TWR] diagnostics, warnings)
                        print(f"[Arduino] {line}")

                time.sleep(0.01)

            except (OSError, serial.SerialException) as e:
                print(f"\n[Hardware error] Arduino disconnected: {e}")
                print("Check the USB connection and restart.")
                break

    except KeyboardInterrupt:
        print("\n\nShutting down Seeker Node...")
        print(f"Final filter stats: {sig.stats()}")
    finally:
        arduino.close()
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
