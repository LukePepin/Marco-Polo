using UnityEngine;

public class UwbVisualizer : MonoBehaviour
{
    [Header("Network Connection")]
    public MqttTelemetryReceiver mqttReceiver;
    
    [Header("Visualizer Object")]
    [Tooltip("The transparent sphere representing the UWB ping radius.")]
    public Transform radarSphere;

    [Header("Scale Multiplier")]
    [Tooltip("If 1 unit in Unity = 1 meter, leave as 1. Adjust if your room is scaled differently.")]
    public float scaleMultiplier = 1.0f;

    void Update()
    {
        // Safety check to ensure we hooked everything up in the Unity Inspector
        if (mqttReceiver == null || radarSphere == null) return;

        // Process all messages currently waiting in the thread-safe queue
        while (mqttReceiver.messageQueue.TryDequeue(out string jsonMessage))
        {
            try
            {
                // Parse the raw JSON text into our C# Data Class
                UwbTelemetryData telemetry = JsonUtility.FromJson<UwbTelemetryData>(jsonMessage);
                
                // The UWB gives us a radius (dist_m). 
                // Unity's LocalScale adjusts the total diameter of the sphere.
                // Diameter = Radius * 2
                float diameter = (telemetry.dist_m * 2f) * scaleMultiplier;
                
                // Instantly snap the sphere to perfectly match the ping distance
                radarSphere.localScale = new Vector3(diameter, diameter, diameter);
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[Visualizer] Failed to parse JSON: {e.Message}. Payload was: {jsonMessage}");
            }
        }
    }
}
