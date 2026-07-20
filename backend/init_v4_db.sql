-- Initialize V4 Asset Tracking Database for Indoor 3D Tracking

-- Main Telemetry Log (Indoor Coordinates from Unity Trilateration)
CREATE TABLE IF NOT EXISTS indoor_tracking (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    x REAL,
    y REAL,
    z REAL
);
