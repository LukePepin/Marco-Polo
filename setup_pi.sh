#!/bin/bash
# Setup script for Marco-Polo Node-RED and Data Pipeline on Raspberry Pi

echo "==========================================="
echo "   Marco-Polo Raspberry Pi Setup Script    "
echo "==========================================="

echo "[1/6] Updating APT repositories..."
sudo apt update
sudo apt upgrade -y

echo "[2/6] Installing dependencies (Git, Python3, venv)..."
sudo apt install -y git python3-pip python3-venv curl build-essential

echo "[3/6] Installing Mosquitto (MQTT Broker)..."
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto

echo "[4/6] Installing SQLite3..."
sudo apt install -y sqlite3

echo "[5/6] Installing Node.js and Node-RED..."
# Using the official Node-RED install script for Raspberry Pi
bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered) --confirm-root --confirm-install --skip-pi

echo "[6/6] Starting Node-RED and installing nodes..."
sudo systemctl enable nodered.service
sudo systemctl start nodered.service

# Wait a few seconds for Node-RED to create directories
sleep 5

# Install SQLite and Worldmap nodes for Node-RED
cd ~/.node-red
npm install node-red-node-sqlite node-red-contrib-web-worldmap node-red-dashboard

# Restart Node-RED to load nodes
sudo systemctl restart nodered.service

echo "==========================================="
echo "Setup Complete!"
echo "- Mosquitto running on port 1883"
echo "- Node-RED running on port 1880"
echo "Next steps: cd into Marco-Polo, setup python venv, and run python scripts."
echo "==========================================="
