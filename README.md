# Marco Polo: Precision Indoor Asset Tracking

**Marco Polo** is a precision indoor tracking and localization system. Designed for Industry 4.0 applications, it utilizes **Ultra-Wideband (UWB)** technology (via the DWM1000 sensor) to achieve centimeter-level accuracy for indoor positioning.

## System Architecture

The system uses a decoupled hardware approach to guarantee real-time performance while maintaining a flexible, high-level application environment:

- **Brain (Raspberry Pi 4):** Handles the high-level Python application logic, node coordination, complex data processing, and user interfaces.
- **Sensor Coprocessor (Arduino Nano 33 BLE):** Dedicated entirely to real-time UWB radio operations via SPI. This ensures that the highly accurate timing requirements of UWB are met without interruptions from a full operating system.

These two devices communicate via USB Serial. The nodes are logically divided into:
- **Hider Node (Transmitter/Tag):** Broadcasts its presence and participates in ranging.
- **Seeker Node (Receiver/Anchor):** Listens for tags, calculates distance, and reports data.

## Current Status (End of Week 3)

The fundamental hardware and communication backbone is complete and highly stable:
- Established a robust **6.8 Mbps wireless UWB link** between Seeker and Hider nodes.
- Re-architected the Arduino firmware to use thread-safe SPI polling, preventing crashes on the Mbed OS platform.
- Implemented **Live Packet Diagnostics** and cross-platform (Windows & Linux) system verification scripts.
- Implemented **Auto-Port Detection** to seamlessly find Arduinos connected to the Raspberry Pi.

## Repository Structure

- `arduino/`: Firmware source code for the Arduino Nano 33 BLE (`hider_node`, `seeker_node`).
- `hider_node.py` / `seeker_node.py`: The Python applications meant to run on the Raspberry Pis.
- `system_verification.py` / `system_verification_win.py`: Tools for validating hardware connections and monitoring live UWB packet statistics.
- `archive/`: Old testing scripts, deprecated validations, and historical logs.

## Documentation

- **[Setup Guide](setup.md)**: Hardware wiring, environment setup, and how to verify your nodes.
- **[TODO / Roadmap](TODO.md)**: The plan for Weeks 4 through 8, focusing on distance calculation and visualization.

---
*Created as a prototype for precision indoor asset tracking.*
