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
   sudo apt install python3-pip
   pip3 install pyserial
   ```

## Single System Feasibility Test
Before dealing with two nodes talking to each other, you should absolutely perform a **Single System Test** to isolate any hardware or serial bugs.
You can run the provided `validate_hardware.py` script on a single Raspberry Pi. This validates:
1. The Pi can successfully detect the Arduino over USB.
2. The Pi can open the serial port and receive streaming data.
If this works, you know the physical hardware integration between the "Brain" (Pi) and "Sensor Coprocessor" (Arduino) is successful before introducing the complexity of UWB wireless communication.

## Running the Concept Scripts

We have provided two scripts: `hider.py` and `seeker.py`.

### Hider Node
1. Connect the hardware as described above.
2. Ensure the Arduino is running a sketch that outputs its state over Serial (e.g., `VELOCITY_HIGH` when moving).
3. Run the script:
   ```bash
   python3 hider.py
   ```

### Seeker Node
1. Connect the hardware as described above.
2. Run the script:
   ```bash
   python3 seeker.py
   ```
