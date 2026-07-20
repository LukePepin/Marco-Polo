#!/bin/bash
echo "Wiping all telemetry data from asset_tracking.db..."
sqlite3 asset_tracking.db "DELETE FROM telemetry_v3; DELETE FROM hider_visualization; DELETE FROM seeker_visualization;"
echo "Database successfully wiped! Ready for fresh testing."
