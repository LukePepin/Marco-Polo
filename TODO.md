Great call on cutting the physical Pi screen. It was officially marked as an optional stretch goal anyway, and shedding that hardware overhead frees you up to focus on the software pipeline and the Digital Twin, which is the real standout feature of the URI Kit.

Regarding the Ultimate GPS Breakout v3: Let's hold off on researching or swapping any new hardware right now. The entire value proposition of the URI track is precision indoor tracking *without* any GPS dependency. Swapping modules now burns precious time and directly contradicts your goal to maximize software development. If you're thinking of the Florida Kit's tracking, the plan is already set for Phase 2 to upgrade to the T-Beam Supreme, which uses the much better u-blox M10 GPS. Let's leave the hardware alone and lean entirely into your UWB and Python/Unity stack.

Here is your simplified, software-focused schedule for the final 16 hours of the Systems Siege.

# Marco Polo URI Kit: Revised 16-Hour Execution Plan

## 📉 Left Side: Local Network Architecture (1 Hour)

*Focus: Ensure offline stability for the close-range demo.*

* [x] **1.1 Local Network Setup:** Configure a dedicated local router or set up the Pi as a standalone Wi-Fi hotspot. The system must not rely on the university's network.
* [x] **1.2 Static IP & Ports:** Assign a static IP to `fox-hunt-pi` and ensure the MQTT/WebSocket ports (e.g., `1883`, `1880`) are exposed for your home PC to pull the telemetry stream.

---

## 🛠️ Bottom: UWB Firmware & Filtering (5 Hours)

*Focus: Stabilize the absolute reference data before passing it to Unity.*

* [x] **2.1 Implement TWR (2 Hours):** Finalize Two-Way Ranging in the Arduino firmware. The Seeker initiates a poll, the Hider responds with timestamps, and the Seeker computes distance.
* [x] **2.2 Moving Average Filter (1.5 Hours):** Update `seeker_node.py` to buffer the last 5-10 UWB distance readings. Output the median or moving average to smooth out transient multi-path noise before it hits the database.
* [ ] **2.3 Simple IMU Gating (1.5 Hours):** Implement basic motion-triggered pinging in the Hider firmware using the onboard LSM9DS1. Suppress UWB pinging when stationary to save processing overhead, resuming on movement.

---

## 📈 Right Side: Unity Digital Twin Integration (4 Hours)

*Focus: Get the visualizer working from a cold start on your local PC.*

* [ ] **3.1 Unity Environment Setup:** Initialize the 2D Unity project on your PC. Create the basic floorplan overlay and the physical boundary limits for the digital room.
* [ ] **3.2 MQTT/WebSocket Bridge:** Configure a C# script in Unity (using a library like M2Mqtt or NativeWebSockets) to subscribe directly to the Raspberry Pi's Node-RED data stream over the local network.
* [ ] **3.3 Position Mapping:** Write the logic to parse the incoming filtered UWB distance JSON and map it to the transform coordinates of the Hider asset within the Unity scene.

---

## 🚀 Top Right: Validation & Presentation (6 Hours)

*Focus: Prove the system works and synthesize the narrative.*

* [ ] **4.1 Ground-Truth Testing (2 Hours):** Power the system on the isolated network. Place the Hider exactly 1.0m, 2.0m, and 5.0m away. Verify the Unity asset accurately snaps to those distances based on the filtered data.
* [ ] **4.2 End-to-End Demo Run (2 Hours):** Run the full cold-start sequence. Verify the Python listener and Node-RED pipeline boot correctly on the Pi, and the Unity twin on your PC connects instantly without manual debugging.
* [ ] **4.3 Presentation Synthesis (2 Hours):** Frame the URI kit perfectly alongside the FL Kit. Show how the LoRa/GPS stack handles the outdoor 1.7-mile approach, and how your UWB edge-architecture takes over seamlessly indoors where GPS fails.
* [ ] **4.4 Project Cleanup (NIL Hours):** Clean up all Lab materials, repository organziation and conclude project (perhaps a LinkedIN post or Project) to formally end project.

---

For the Unity integration in Phase 3, are you planning to pull the JSON data via MQTT directly from the broker, or are you having Node-RED push it out over a WebSocket node?

---

## 🔬 Historical Notes: Dead Reckoning Analysis

*(Note: We have successfully pivoted to UWB distance tracking. These notes remain for historical context on why the original Streamlit IMU approach was abandoned).*

The original dashboard visualized movement using **Dead Reckoning**—specifically, by taking the raw IMU accelerometer values and applying Euler integration (Acceleration → Velocity → Position). 

### Why this approach failed:
1. **Exponential Drift:** Cheap IMU accelerometers have inherent electrical noise. Because we have to integrate the data twice to find the position, any tiny bit of noise is mathematically magnified. Over time, this causes "drift" where the dashboard dot will slowly slide across the room even if the hardware is sitting perfectly still on a desk.
2. **No Absolute Reference:** Unlike UWB (which provides absolute distance), the IMU only measures *relative* changes. If the system loses power or drops a packet, it has no idea where it actually is in the room. This is why we had to manually input the starting X/Y coordinates and heading on the dashboard.
3. **Tilt Interference:** Gravity is an acceleration of 1G. If the user tilts the Arduino slightly downward, the IMU will read gravity as "forward acceleration" and the dashboard will think the user is sprinting across the room. We had no Kalman Filter to fuse the Gyroscope data to subtract gravity from the equation.
