# Marco-Polo Handoff Guide for Jay-sun

Welcome to the Marco-Polo tracking project! The infrastructure is split into three main parts: 
1. **The Arduino Hardware** (Sends UWB data)
2. **The Python Listener & Node-RED** (Routes data into the SQLite database)
3. **The Streamlit Dashboard** (Visualizes the dead-reckoning on a map)

Everything runs off the central Raspberry Pi (`fox-hunt-pi.local`). Here is your cheat sheet to get the entire system booted up from a cold start.

---

## 🚀 How to Start the System

You will need to open **two** SSH terminal windows to the Pi. Log in to both terminals using this exact command:
```bash
ssh hunter@fox-hunt-pi.local
```
*(When prompted for the password, it is whatever password was set for the `hunter` account)*

**Important First Step:** Ensure the Pi has the latest code by pulling from the repository:
```bash
cd ~/Documents/Marco-Polo
git pull
```

### Terminal 1: The Hardware Listener
Plug the Seeker Arduino into the Pi's USB port, then run the listener script. This script pulls data from the Arduino and pushes it to the MQTT broker so Node-RED can log it.
```bash
cd ~/Documents/Marco-Polo
python3 polo_seeker_node.py
```
*(Leave this terminal running! You should see `[MQTT Published]` printing every second).*

### Terminal 2: The Map Dashboard
Open a second SSH terminal and launch the predictive tracking dashboard.
```bash
cd ~/Documents/Marco-Polo
~/.local/bin/streamlit run dashboard.py
```
*(Leave this running!)*

---

## 🌐 Web Interfaces
Once the two commands above are running, you can access the system from any browser on the same Wi-Fi network:

- **The Predictive Dashboard:** [http://fox-hunt-pi.local:8501](http://fox-hunt-pi.local:8501)
  *Use the "Physics Tuning" sliders here to adjust the deadband and scale factor of the dead-reckoning movement.*
- **The Node-RED Flow Editor:** [http://fox-hunt-pi.local:1880](http://fox-hunt-pi.local:1880)
  *This is where the background database routing happens. If you need to debug the raw JSON, open the right-hand sidebar and click the Bug icon.*

---

## 🔧 Troubleshooting Commands

If things break, use these commands to figure out why:

**1. "The Python script can't find the Arduino"**
Sometimes the Pi assigns the USB port a different name. Run this to see all connected USB devices:
```bash
ls /dev/ttyACM* /dev/ttyUSB*
```
*(If it shows `/dev/ttyACM1`, update the `ARDUINO_PORT` variable at the top of `polo_seeker_node.py`)*

**2. "Data isn't showing up in the Database / Node-RED crashed"**
Node-RED runs invisibly in the background. To view its live error logs, run:
```bash
sudo journalctl -u nodered -n 50 -f
```
*(Press `Ctrl+C` to exit the log viewer)*

**3. "I need to restart Node-RED"**
```bash
sudo systemctl restart nodered
```

**4. "The Arduino is silent (Terminal 1 hangs at 'Listening...')"**
Unplug the Arduino USB cable, plug it back in, and restart `polo_seeker_node.py`. The Arduino hardware just froze and needed a hard reset.
