# Proof of Concept Architecture: Marco Polo

> **Pivot Note:** LoRa has been removed for this phase. This simplifies the architecture by relying entirely on the Ultra-Wideband (UWB) module (or optionally Bluetooth) for both precision distance ranging AND inter-node communication. This aligns perfectly with an indoor "Precision Industry 4.0 Asset Tracker" use case.

## System Overview

The Raspberry Pi 4 acts as the main controller for each node (Hider and Seeker). The Arduino Nano 33 BLE Sense is repurposed as a dedicated sensor coprocessor, handling the strict real-time demands of the UWB sensor and its onboard IMU.

### Node Architecture (Hider & Seeker)

```text
+-------------------+                      +-----------------------+
|   Raspberry Pi 4  |                      | Arduino Nano 33 BLE   |
|   (Main Brain)    | <--- USB Serial ---> | (Sensor Coprocessor)  |
|                   |                      |                       |
| - Python Scripts  |                      | - Reads IMU (Velocity)|
| - High-level logic|                      | - Reads UWB (Distance)|
| - Network / WiFi  |                      | - Sends UWB Data Pings|
| - Logs / DB       |                      +-----------------------+
+-------------------+                                 |
                                                      | SPI
                                                      v
                                               +---------------+
                                               |  UWB Sensor   |
                                               +---------------+
```

## Responsibilities

### 1. Arduino Nano 33 BLE Sense
- **IMU Processing**: Constantly polls the onboard accelerometer/gyroscope to calculate velocity/movement.
- **UWB Ranging & Communication**: Interfaces with the UWB module to perform Two-Way Ranging (TWR) for distance measurement. It also uses the UWB radio to send arbitrary data payloads (like "Pings" to wake up the Seeker).
- **Reporting**: Sends parsed state messages to the Raspberry Pi over the USB Serial connection (e.g., `MOTION_DETECTED` or `UWB_DIST: 12.4`).

### 2. Raspberry Pi 4
- **State Machine**: Runs the core `hider` or `seeker` logic in Python.
- **Decision Making**: Listens to the Arduino. If the velocity threshold is exceeded, it commands the Arduino to begin its wireless Ping sequence.
- **Future Expandability**: Provides the horsepower needed for the screen, data visualization, Node-RED, and any complex pathfinding or TinyML inference.

## The "Velocity Ping" Workflow

1. The Hider's Arduino detects that velocity > threshold.
2. Arduino sends `MOTION_DETECTED` over USB serial to the Hider Pi.
3. The Hider Pi receives this trigger and commands the Arduino to `SEND_PING`.
4. The Hider's Arduino uses the UWB module to broadcast a data packet.
5. The Seeker's UWB module receives the packet, and the Seeker's Arduino relays `UWB_PING_DETECTED` to the Seeker Pi via USB.
6. The Seeker Pi registers the ping and commands the Seeker's Arduino to `START_RANGING` to find the exact distance to the Hider.
