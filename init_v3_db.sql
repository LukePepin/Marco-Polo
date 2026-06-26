-- Initialize V3 Asset Tracking Database with Visualization Tables

-- 1. Main Telemetry Log (All raw data + IMU vectors)
CREATE TABLE IF NOT EXISTS telemetry_v3 (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    gps_valid BOOLEAN,
    latitude REAL,
    longitude REAL,
    acc_x REAL,
    acc_y REAL,
    acc_z REAL,
    gyr_x REAL,
    gyr_y REAL,
    gyr_z REAL,
    mag_x REAL,
    mag_y REAL,
    mag_z REAL,
    raw_gps TEXT
);

-- 2. Hider Specific Visualization Table
CREATE TABLE IF NOT EXISTS hider_visualization (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    latitude REAL,
    longitude REAL,
    acc_x REAL,
    acc_y REAL,
    acc_z REAL,
    gyr_x REAL,
    gyr_y REAL,
    gyr_z REAL,
    mag_x REAL,
    mag_y REAL,
    mag_z REAL
);

-- 3. Seeker Specific Visualization Table
CREATE TABLE IF NOT EXISTS seeker_visualization (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    latitude REAL,
    longitude REAL,
    acc_x REAL,
    acc_y REAL,
    acc_z REAL,
    gyr_x REAL,
    gyr_y REAL,
    gyr_z REAL,
    mag_x REAL,
    mag_y REAL,
    mag_z REAL
);
