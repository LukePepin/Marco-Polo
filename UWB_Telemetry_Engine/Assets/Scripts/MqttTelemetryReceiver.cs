using UnityEngine;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using System.Text;
using System.Collections.Concurrent;

public class MqttTelemetryReceiver : MonoBehaviour
{
    [Header("MQTT Settings")]
    public string brokerIpAddress = "fox-hunt-pi.local"; // Replace with your Pi's IP or "localhost"
    public int brokerPort = 1883;
    public string topic = "marcopolo/telemetry/seeker";

    private MqttClient client;
    
    // Thread-safe queue to pass messages from MQTT background thread to Unity main thread
    public ConcurrentQueue<string> messageQueue = new ConcurrentQueue<string>();

    void Start()
    {
        try
        {
            // Clean up the IP address just in case there are invisible spaces
            string cleanIp = brokerIpAddress.Trim();
            
            // Initialize the MQTT Client
            client = new MqttClient(cleanIp, brokerPort, false, null, null, MqttSslProtocols.None);
            
            // Register to the event when a message is received
            client.MqttMsgPublishReceived += Client_MqttMsgPublishReceived;
            
            // Connect to the broker with a unique ID
            string clientId = System.Guid.NewGuid().ToString();
            client.Connect(clientId);
            
            // Subscribe to the telemetry topic
            client.Subscribe(new string[] { topic }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });
            
            Debug.Log($"[MQTT] Successfully connected to {brokerIpAddress}:{brokerPort}. Listening on {topic}");
        }
        catch (System.Exception e)
        {
            Debug.LogError($"[MQTT] Connection failed: {e.Message}. Make sure the IP is correct and Node-RED/Mosquitto is running!");
        }
    }

    private void Client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
    {
        // This fires on a background network thread. We cannot directly move Unity objects here.
        // Instead, we decode the message to a string and push it into a thread-safe queue.
        string message = Encoding.UTF8.GetString(e.Message);
        messageQueue.Enqueue(message);
    }

    void OnDestroy()
    {
        // Clean up connection when Unity stops playing
        if (client != null && client.IsConnected)
        {
            client.Disconnect();
            Debug.Log("[MQTT] Disconnected cleanly.");
        }
    }
}
