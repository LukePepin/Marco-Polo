# Marco-Polo Project Status & To-Do

## ✅ What We Accomplished (Current Sprint)

- **Hardware & User Migration:** Successfully migrated the central backend from the old Seeker Pi to the new Hub Pi (`fox-hunt-pi`). Ported all configuration files to the new `hunter` user and resolved underlying permission issues.
- **Continuous UWB Telemetry:** Flashed new Arduino firmware (v3) to bypass the old request/response architecture, enabling a continuous, unbroken 1Hz telemetry stream from the Hider to the Seeker over the UWB radios.
- **Data Pipeline Rebuild:** Rewrote the Python hardware listener (`polo_seeker_node.py`) to parse the continuous JSON stream. Forcefully compiled a native SQLite C++ binary from source on the Pi to resolve GLIBC compatibility issues and restore the Node-RED database ingest pipeline.
- **Predictive Capstone Dashboard:** Created a live Streamlit dashboard (`dashboard.py`) mathematically mapped to the physical layout of the Capstone room. Built in live physics tuning sliders (Accel Deadband, Scale Factor, Velocity Decay) to allow manual adjustment of the movement algorithms.

---

## 🔬 Dead Reckoning Analysis

The current dashboard visualizes movement using **Dead Reckoning**—specifically, by taking the raw IMU accelerometer values and applying Euler integration (Acceleration → Velocity → Position). 

### Why this approach will struggle:
1. **Exponential Drift:** Cheap IMU accelerometers have inherent electrical noise. Because we have to integrate the data twice to find the position, any tiny bit of noise is mathematically magnified. Over time, this causes "drift" where the dashboard dot will slowly slide across the room even if the hardware is sitting perfectly still on a desk.
2. **No Absolute Reference:** Unlike GPS (which provides absolute coordinates), the IMU only measures *relative* changes. If the system loses power or drops a packet, it has no idea where it actually is in the room. This is why we have to manually input the starting X/Y coordinates and heading on the dashboard.
3. **Tilt Interference:** Gravity is an acceleration of 1G. If the user tilts the Arduino slightly downward, the IMU will read gravity as "forward acceleration" and the dashboard will think the user is sprinting across the room. We currently have no Kalman Filter to fuse the Gyroscope data to subtract gravity from the equation.

### Why this approach is still useful:
It proves the telemetry pipeline is instantaneous and that the math engine works. The sliders on the dashboard allow you to aggressively filter out the noise (via the Deadband slider) and simulate friction (via the Decay slider) to get a rough approximation of movement for testing purposes.

---

## 📝 To-Do / Next Steps for Jay-sun

1. **RSSI Investigation:** Jay-sun is taking over the project to research **RSSI (Received Signal Strength Indicator)** between the two UWB radios.
2. **Sensor Fusion:** Once Jay-sun determines how to accurately calculate the distance between the two radios using RSSI, that distance data can be fused with the IMU heading data.
3. **The Ultimate Fix:** By using RSSI to provide an "Absolute Reference" distance, the system will no longer rely purely on Dead Reckoning, completely eliminating the IMU drift problem!
