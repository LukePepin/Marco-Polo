# Marco Polo: Roadmap & TODO (Weeks 4-8)

As of the end of Week 3, the foundational UWB link (6.8 Mbps) and live hardware diagnostics are fully operational. This document outlines the focus areas for the remaining 5 weeks of development leading up to the final presentation in Week 10.

## Week 4: Distance Calculation & Motion Detection (Part 1)

**Goal:** Transition to distance measurement and integrate the IMU for smart triggering.

- [ ] Implement Two-Way Ranging (TWR) logic in the Arduino firmware to calculate distance (Seeker initiates, Hider responds).
- [ ] Utilize the Arduino Nano 33 BLE's built-in IMU (Inertial Measurement Unit) on the Hider node.
- [ ] Write logic so that the Hider only triggers/responds to pings when physical movement is detected.

## Week 5: Data Filtering & Motion Detection (Part 2)

**Goal:** Stabilize the distance readings and finalize the movement-triggered ping logic.

- [ ] Refine the IMU thresholding so the Hider reliably sleeps when stationary and wakes upon movement.
- [ ] Implement data smoothing on the Raspberry Pi (e.g., moving average or median filters) to clean up noisy UWB distance readings.
- [ ] Ensure the Python `seeker_node.py` script reliably outputs clean, filtered distance data.

## Week 6: Data Pipeline & Storage (Node-RED + NoSQL)

**Goal:** Build a robust backend to record and route tracking data.

- [ ] Install and configure **Node-RED** on the Raspberry Pi to act as the central data broker.
- [ ] Route the filtered Python distance data into Node-RED.
- [ ] Set up a **NoSQL Database** (e.g., MongoDB or CouchDB) to permanently record the historical location/distance data.

## Week 7: Unity 2D Visualization (Part 1)

**Goal:** Begin developing a professional front-end dashboard using the Unity game engine.

- [ ] Set up a 2D Unity project for visualizing the asset's location.
- [ ] Establish a communication bridge between Node-RED (or the NoSQL DB) and the Unity application (e.g., MQTT or WebSockets).
- [ ] Create the basic UI layout for the tracking dashboard.

## Week 8: Unity 2D Visualization (Part 2) & Final Polish

**Goal:** Complete the visualizer and harden the system for presentation.

- [ ] Map the incoming distance/ranging data to 2D coordinates in the Unity visualizer.
- [ ] Stress-test the entire pipeline: IMU Movement -> UWB Ping -> Pi Filter -> Node-RED -> NoSQL -> Unity.
- [ ] Finalize documentation, architecture diagrams, and prepare for edge-cases (e.g., node resets, network drops).

## Week 10: Presentation

- [ ] No active development. Focus entirely on live demonstration logistics and the final report.
- [ ] sudo nmcli dev wifi connect "ISECapstone" password "j0shf1dh"

