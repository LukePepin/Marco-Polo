using UnityEngine;

public class UwbVisualizer : MonoBehaviour
{
    [Header("Network Connection")]
    public MqttTelemetryReceiver mqttReceiver;
    
    [Header("Visualizer Object")]
    [Tooltip("The transparent sphere representing the UWB ping radius.")]
    public Transform radarSphere;

    [Header("UI Polish")]
    [Tooltip("Drag the UwbHudManager script here so we can update the UI text.")]
    public UwbHudManager hudManager;

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
                
                // Only update the sphere if we got a valid, successful filtered reading (> 0).
                // This ensures the sphere holds its previous size if a packet drops or fails!
                if (telemetry.distance_m_filtered > 0)
                {
                    float diameter = (telemetry.distance_m_filtered * 2f) * scaleMultiplier;
                    radarSphere.localScale = new Vector3(diameter, diameter, diameter);

                    // Update the UI Canvas
                    if (hudManager != null)
                    {
                        hudManager.UpdateDistanceText(telemetry.distance_m_filtered);
                    }
                }
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[Visualizer] Failed to parse JSON: {e.Message}. Payload was: {jsonMessage}");
            }
        }
    }
}
