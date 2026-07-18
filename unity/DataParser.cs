using System;

[Serializable]
public class UwbTelemetryData
{
    // These names must exactly match the JSON keys coming from your Python script
    public float dist_m;
    public float rssi;
}
