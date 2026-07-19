using System;

[Serializable]
public class UwbTelemetryData
{
    // These names must exactly match the JSON keys coming from your Python script
    public float distance_m;
    public float distance_m_raw;
    public float distance_m_filtered;
    public float rssi_dbm;
}
