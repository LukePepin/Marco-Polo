using UnityEngine;

public class UwbVisualizer : MonoBehaviour
{
    [Header("Network Connection")]
    public MqttTelemetryReceiver mqttReceiver;
    
    [Header("Visualizer Object")]
    [Tooltip("The transparent cylinder representing the UWB ping radius.")]
    public Transform radarSphere;

    [Header("UI Polish")]
    [Tooltip("Drag the UwbHudManager script here so we can update the UI text.")]
    public UwbHudManager hudManager;

    [Header("Scale Multiplier")]
    [Tooltip("If 1 unit in Unity = 1 meter, leave as 1. Adjust if your room is scaled differently.")]
    public float scaleMultiplier = 1.0f;

    [Header("2D Floor Ring Settings")]
    public int segments = 50;
    public float lineWidth = 0.05f;
    private LineRenderer circleRenderer;

    void Start()
    {
        if (radarSphere != null)
        {
            // Hide the solid cylinder mesh so it doesn't block the view
            MeshRenderer mesh = radarSphere.GetComponent<MeshRenderer>();
            if (mesh != null) mesh.enabled = false;

            // Add a clean hollow LineRenderer to draw on the floor instead
            circleRenderer = radarSphere.GetComponent<LineRenderer>();
            if (circleRenderer == null)
            {
                circleRenderer = radarSphere.gameObject.AddComponent<LineRenderer>();
            }
            
            circleRenderer.positionCount = segments + 1;
            circleRenderer.useWorldSpace = true; // Draw in global space so line width doesn't scale
            circleRenderer.startWidth = lineWidth;
            circleRenderer.endWidth = lineWidth;
        }
    }

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
                if (telemetry.distance_m_filtered > 0)
                {
                    float diameter = (telemetry.distance_m_filtered * 2f) * scaleMultiplier;
                    
                    // We must continue scaling the invisible mesh so TrilaterationManager can read it
                    radarSphere.localScale = new Vector3(diameter, 0.1f, diameter);

                    // Draw the hollow 2D hoop on the floor!
                    if (circleRenderer != null)
                    {
                        float radius = diameter / 2f;
                        float angle = 0f;
                        for (int i = 0; i < (segments + 1); i++)
                        {
                            float x = radarSphere.position.x + Mathf.Sin(Mathf.Deg2Rad * angle) * radius;
                            float z = radarSphere.position.z + Mathf.Cos(Mathf.Deg2Rad * angle) * radius;
                            // Draw at exactly 0 height on the Y axis as requested
                            circleRenderer.SetPosition(i, new Vector3(x, 1f, z));
                            angle += (360f / segments);
                        }
                    }

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
