using UnityEngine;
using UnityEngine.UI;

public class UwbHudManager : MonoBehaviour
{
    [Header("UI Text Elements")]
    [Tooltip("The text UI element that displays the live distance.")]
    public Text distanceText;
    
    [Tooltip("The text UI element that displays the connection status.")]
    public Text connectionStatusText;

    [Header("Connection Timeout")]
    [Tooltip("If we don't receive a message for this many seconds, show Disconnected.")]
    public float timeoutSeconds = 3.0f;
    private float lastMessageTime = 0f;

    void Start()
    {
        if (connectionStatusText != null)
        {
            connectionStatusText.text = "Waiting for Telemetry...";
            connectionStatusText.color = Color.yellow;
        }
    }

    void Update()
    {
        // Check if we haven't received a message recently (Simulating a dropped connection)
        if (Time.time - lastMessageTime > timeoutSeconds)
        {
            if (connectionStatusText != null)
            {
                connectionStatusText.text = "DISCONNECTED";
                connectionStatusText.color = Color.red;
            }
        }
    }

    // This is called by UwbVisualizer.cs every time a valid JSON packet arrives
    public void UpdateDistanceText(float distance)
    {
        lastMessageTime = Time.time; // Reset the timeout timer

        if (distanceText != null)
        {
            distanceText.text = $"Target Distance: {distance.ToString("F2")}m";
        }

        if (connectionStatusText != null)
        {
            connectionStatusText.text = "CONNECTED";
            connectionStatusText.color = Color.green;
        }
    }
}
