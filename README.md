# Marco Polo: Precision Indoor Asset Tracking

**Marco Polo** is a precision indoor tracking and localization system serving as an educational Industry 4.0 application. Designed to bridge the gap between physical sensor hardware and high-level 3D visualization, it utilizes **Ultra-Wideband (UWB)** technology (via DWM1000 sensors) to achieve centimeter-level accuracy for indoor positioning, mapped directly to a **Unity Digital Twin**.

## System Engineering Architecture

The system uses a decoupled hardware approach to guarantee real-time performance while maintaining a flexible, high-level application environment:

- **Sensor Coprocessor (Arduino Nano 33 BLE):** Dedicated entirely to real-time UWB radio operations via SPI. This ensures that the highly accurate timing requirements of Two-Way Ranging (TWR) are met without interruptions from a full operating system. It calculates the physical distance mathematically using carrier integrator clock offset correction to prevent temporal drift.
- **Data Router (Raspberry Pi 4):** Handles the high-level Python application logic. A local listener parses the serial telemetry from the Arduino, applies median and moving-average filtering, and publishes the clean JSON data to a local MQTT broker.
- **Visualization Engine (Unity):** The Digital Twin runs on a host PC. It subscribes to the MQTT broker over the local network and live-updates the 3D model transforms to match the physical asset.

---

## Hardware Requirements

### Per Node (Seeker & Hider)
- 1x Arduino Nano 33 BLE Sense
- 1x UWB DWM1000 Sensor
- USB cable (Type-A to Micro-USB for Nano)

### Base Station
- 1x Raspberry Pi 4 (with power supply and SD card)

---

## Getting Started

### 1. Arduino Wiring & Firmware
The UWB sensor connects directly to the Arduino Nano 33 BLE via SPI:
- **CS**: D20 (A6)
- **IRQ**: D21 (A7)
- **RST**: D3
- **MOSI/MISO/SCK**: Standard Nano SPI pins (D11, D12, D13)

Flash the `arduino/marco_hider/marco_hider.ino` sketch to the Hider node.
Flash the `arduino/polo_seeker/polo_seeker.ino` sketch to the Seeker node.

### 2. Raspberry Pi Initialization
Log into the central Raspberry Pi base station via SSH on your local network:
```bash
ssh hunter@fox-hunt-pi.local
```

Ensure the Python environment has the necessary serial and MQTT packages installed:
```bash
cd ~/Documents/Marco-Polo
sudo apt update
sudo apt install python3-pip python3-venv
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 3. Launching the System
Plug the Seeker Arduino into the Pi's USB port. Run the hardware listener script to begin pulling data from the Arduino and pushing it to the MQTT broker:

```bash
cd ~/Documents/Marco-Polo
python3 polo_seeker_node.py
```

*(Leave this terminal running! You should see `[MQTT Published]` printing every second as long as the Hider is powered on nearby).*

### 4. Database & Routing (Optional)
The Node-RED pipeline runs invisibly in the background. If you need to debug the raw JSON routing or database storage:
- Access the Node-RED Flow Editor: [http://fox-hunt-pi.local:1880](http://fox-hunt-pi.local:1880)
- To view live Node-RED error logs from the Pi terminal: `sudo journalctl -u nodered -n 50 -f`
- To restart the Node-RED service: `sudo systemctl restart nodered`

---

## Repository Structure

- `arduino/`: Firmware source code for the Arduino Nano 33 BLE (`marco_hider`, `polo_seeker`).
- `polo_seeker_node.py`: The Python MQTT gateway running on the Raspberry Pi.
- `system-verify/`: Tools for validating hardware connections and monitoring live UWB packet statistics via USB.
- `TODO.md`: The roadmap for Unity implementation and further hardware optimizations (like IMU gating).

---
*Developed as a prototype for precision indoor asset tracking, integrating UWB telemetry into modern 3D engines.*
