-- Initialize the Asset Tracking Database

-- 1. Main Telemetry Log (All raw data for post-mission analysis)
CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    gps_valid BOOLEAN,
    latitude REAL,
    longitude REAL,
    lora_rssi INTEGER,
    ble_rssi INTEGER,
    motion_detected BOOLEAN,
    battery_v REAL
);

-- 2. Latest Known Update (Constantly updated, used for dashboard recovery)
-- This table stores only one row per device_id
CREATE TABLE IF NOT EXISTS latest_telemetry (
    device_id TEXT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    gps_valid BOOLEAN,
    latitude REAL,
    longitude REAL,
    lora_rssi INTEGER,
    ble_rssi INTEGER,
    motion_detected BOOLEAN,
    battery_v REAL
);

-- 3. Last Stable Update (Only updated when GPS is valid and movement is detected, or good signal)
-- This table stores only one row per device_id, representing the last "good" ping
CREATE TABLE IF NOT EXISTS stable_telemetry (
    device_id TEXT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    battery_v REAL
);
