# Marco Polo URI Kit: Final Master Checklist

This is the ultimate roadmap for the remainder of the Systems Siege. It incorporates the massive pivot to **True 3D Trilateration** using a distributed network of Raspberry Pis, the final Unity environment polish, and the presentation synthesis.

---

## 📈 Phase 3: Unity Environment Polish
*Focus: Finalize the graphics and user experience of the Digital Twin.*

* [x] **3.1 Graphics & Lighting:** Fix the ceiling light emission/baking so the room is properly illuminated, and add Unity Post-Processing (Bloom, Ambient Occlusion).
* [x] **3.2 HUD & UI:** Add a sleek UI Canvas Overlay (HUD) to print the live `distance_m` text and a green/red connection status indicator.
* [x] **3.3 Audio Cues:** Add a sonar "ping" sound effect that triggers upon receiving valid MQTT telemetry.
* [x] **3.4 Isometric View:** Add a keyboard toggle for a top-down 2D Isometric view (automatically disabling the ceiling mesh but preserving baked lighting).
* [x] **3.5 Camera Fix:** Fix the First Person Controller so the camera always starts facing the correct, desired direction.

---

## 🛰️ Phase 4: True 3D Tracking (Trilateration Pivot)
*Focus: Deploy 3 independent Raspberry Pi/Seeker nodes to mathematically pinpoint the Hider in 3D space.*

* [ ] **4.1 Distributed Hardware Deployment:** Build and place 3 identical Seeker systems (Arduino + Raspberry Pi) in the corners of the room.
* [ ] **4.2 Python Script Networking:** Update `polo_seeker_node.py` so that each Pi publishes to the main broker (`fox-hunt-pi.local`) under unique IDs (e.g., `seeker_1`, `seeker_2`, `seeker_3`).
* [ ] **4.3 The C# Trilateration Engine:** Write `TrilaterationManager.cs` in Unity to parse the 3 incoming distances, run the Non-Linear Least Squares intersection math, and move a 3D target to the exact physical X, Y, Z coordinate in the room.

---

## 🚀 Phase 5: Validation & Presentation
*Focus: Prove the system works, record it, and synthesize the narrative.*

* [ ] **5.1 Ground-Truth Testing:** Power all 3 Pis on the isolated network. Move the Hider to 3 known locations in the room and verify the Unity target accurately snaps to those 3D coordinates.
* [ ] **5.2 End-to-End Demo Run:** Run the full cold-start sequence. Verify all 3 Pis boot, connect to MQTT, and Unity tracks the target instantly.
* [ ] **5.3 Presentation Video Recording:** Record a split-screen video: one camera showing you walking the physical Hider around the room, and a screen-recording showing the Unity Digital Twin tracking you in real-time.
* [ ] **5.4 Presentation Synthesis:** Frame the URI kit perfectly alongside the FL Kit. Show how the LoRa/GPS stack handles the outdoor approach, and how your Distributed UWB Pi-Network takes over seamlessly indoors where GPS fails.
* [ ] **5.5 Project Cleanup:** Clean up the repository, organize the docs, and officially conclude the Systems Siege!
