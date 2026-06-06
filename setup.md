# Raspberry Pi + Arduino Nano + UWB Proof of Concept Setup

> **Pivot Note:** As part of a risk mitigation strategy outlined in the Project Charter, the LoRa module has been temporarily removed from this initialization test. This shifts the focus from wide-area tracking to **Precision Indoor Asset Tracking** (Industry 4.0), utilizing UWB's centimeter-level accuracy for both ranging and communication.

This guide covers the basic setup for integrating the UWB sensors and Arduino Nano 33 BLE setup with a Raspberry Pi 4.

## Hardware Requirements (per node)
- 1x Raspberry Pi 4 (with power supply and SD card)
- 1x Arduino Nano 33 BLE Sense (with UWB DWM1000 sensor attached)
- USB cable (Type-A to Micro-USB for Nano)

## Wiring Guide

### 1. Arduino Nano 33 BLE to Raspberry Pi 4
The easiest and most reliable way to connect the Arduino Nano to the Raspberry Pi is via USB.
- **Connection**: Plug the Arduino Nano into one of the Raspberry Pi's USB ports using a USB cable.
- **Why?**: This provides power to the Arduino and establishes a reliable serial connection (`/dev/ttyACM0` or `/dev/ttyUSB0`) without worrying about logic level shifting or GPIO conflicts.

### 2. UWB Sensor to Arduino Nano 33 BLE
The UWB sensor (DWM1000) connects directly to the Arduino Nano 33 BLE via SPI. Based on your existing code, the connections are:
- **CS**: D10
- **IRQ**: D2
- **RST**: D3
- **MOSI/MISO/SCK**: Standard Nano SPI pins (D11, D12, D13)

## Raspberry Pi Configuration

1. **Install Python Dependencies**:
   ```bash
   sudo apt update
   sudo apt install python3-pip python3-venv
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   ```

   If you prefer the Debian package manager instead of a virtual environment, the serial library package is `python3-serial`.

   **Clean, complete command sequence**:
   ```bash
   cd ~/Documents/Marco-Polo

   sudo apt update
   sudo apt install python3-pip python3-venv

   python3 -m venv .venv
   source .venv/bin/activate

   pip install -r requirements.txt
   ```

2. **Install Arduino IDE 2.x (Debian 13)**:
   ```bash
   cd ~/Downloads
   wget https://downloads.arduino.cc/arduino-ide/arduino-ide_2.3.4_Linux_64bit.zip
   unzip arduino-ide_2.3.4_Linux_64bit.zip
   ./arduino-ide_2.3.4_Linux_64bit/arduino-ide
   ```

   Optional: add to PATH for easy launch:
   ```bash
   sudo ln -s ~/Downloads/arduino-ide_2.3.4_Linux_64bit/arduino-ide /usr/local/bin/arduino-ide
   ```

## System Verification & Live Diagnostics

Before running the main node scripts, you should verify that the Arduino and UWB sensors are communicating properly. We have built robust validation tools for both Windows and Raspberry Pi.

### On Windows
If you are testing the Arduino connected directly to a Windows PC:
1. Run `python system_verification_win.py`
2. Select your COM port. The script will test baud rates and query the UWB hardware status.
3. You can optionally enter the **Live Diagnostics Monitor** to view real-time packet statistics (`TX_OK`, `RX_OK`, etc.).

### On Raspberry Pi
If the Arduino is connected to the Raspberry Pi:
1. Run `python system_verification.py`
2. This script auto-detects connected Arduinos and performs a 5-step validation to ensure the UWB hardware is responding.
3. For continuous monitoring of packet drops/successes, run:
   ```bash
   python system_verification.py --monitor
   ```

## Running the Concept Scripts

We have provided two scripts: `hider_node.py` and `seeker_node.py`. These scripts automatically detect the connected Arduino, so you don't need to hardcode COM ports or `/dev/ttyACM0` paths.

### Hider Node (Transmitter)
1. Connect the hardware as described above.
2. Ensure the Arduino is flashed with the `arduino/hider_node/hider_node.ino` sketch.
3. Run the script:
   ```bash
   python3 hider_node.py
   ```
4. Press ENTER to broadcast a UWB ping.

### Seeker Node (Receiver)
1. Connect the hardware as described above.
2. Ensure the Arduino is flashed with the `arduino/seeker_node/seeker_node.ino` sketch.
3. Run the script:
   ```bash
   python3 seeker_node.py
   ```
4. It will listen for pings from the Hider and log them to the console.
