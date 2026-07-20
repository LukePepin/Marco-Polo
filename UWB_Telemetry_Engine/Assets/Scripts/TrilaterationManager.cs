using UnityEngine;

public class TrilaterationManager : MonoBehaviour
{
    [Header("The Seeker Spheres")]
    [Tooltip("Drag all 4 of your Seeker-Radar spheres here")]
    public Transform[] seekerSpheres;

    [Header("The Hider Target")]
    [Tooltip("Create a 3D object (like a red cube) to represent the Hider, and drag it here")]
    public Transform targetHider;

    [Header("UI Polish")]
    [Tooltip("Drag your HUD Manager here to display coordinates.")]
    public UwbHudManager hudManager;

    [Header("MQTT Publisher")]
    [Tooltip("Drag the MQTT_Manager here so we can publish the coordinates back to Node-RED")]
    public MqttTelemetryReceiver mqttManager;

    [Header("Algorithm Settings")]
    [Tooltip("Match this to the scale multiplier on your UwbVisualizer scripts!")]
    public float scaleMultiplier = 1.0f;
    
    [Tooltip("How hard the math tries to find the perfect intersection. 100 is great for realtime.")]
    public int solverIterations = 100;
    
    [Tooltip("Learning rate for the gradient descent. 0.1 is very stable.")]
    public float learningRate = 0.1f;



    void Update()
    {
        if (seekerSpheres.Length < 3 || targetHider == null) return;

        // 1. Start our mathematical guess at the Hider's current position
        Vector3 bestGuess = targetHider.position;

        // 2. Run a Gradient Descent (Non-Linear Least Squares) algorithm to find the exact 3D intersection!
        for (int step = 0; step < solverIterations; step++)
        {
            Vector3 gradient = Vector3.zero;

            for (int i = 0; i < seekerSpheres.Length; i++)
            {
                if (seekerSpheres[i] == null) continue;
                
                // We cleverly read the radius directly from the glowing spheres you already set up!
                // (Since UwbVisualizer set the scale to diameter, we divide by 2)
                float targetRadius = seekerSpheres[i].localScale.x / (2f * scaleMultiplier);
                
                // Skip if this specific sphere hasn't received valid data (radius is 0)
                if (targetRadius <= 0.01f) continue;

                // Calculate the error between our guess and the sphere's actual radius
                Vector3 seekerPos = seekerSpheres[i].position;
                Vector3 diff = bestGuess - seekerPos;
                float currentDist = diff.magnitude;

                if (currentDist > 0.001f)
                {
                    float error = currentDist - targetRadius;
                    gradient += (diff / currentDist) * error;
                }
            }

            // Move our guess closer to the true intersection point
            bestGuess -= gradient * learningRate;
        }

        // 3. Calculate the final "Uncertainty / Error" of our best guess
        float totalError = 0f;
        int activeSpheres = 0;
        for (int i = 0; i < seekerSpheres.Length; i++)
        {
            if (seekerSpheres[i] == null || seekerSpheres[i].localScale.x <= 0.01f) continue;
            float targetRadius = seekerSpheres[i].localScale.x / (2f * scaleMultiplier);
            float currentDist = Vector3.Distance(bestGuess, seekerSpheres[i].position);
            totalError += Mathf.Abs(currentDist - targetRadius);
            activeSpheres++;
        }

        // 4. Move the physical 3D object to the mathematically solved coordinate!
        Vector3 finalPos = Vector3.Lerp(targetHider.position, bestGuess, Time.deltaTime * 5f);
        finalPos.y = 1.5f; // Lock the hider target height to exactly 1.5
        targetHider.position = finalPos;

        // 5. Update the HUD
        if (hudManager != null)
        {
            hudManager.UpdateLocationText(finalPos);
        }

        // 6. Publish the final coordinate back to the MQTT Broker for Node-RED/SQLite
        if (mqttManager != null)
        {
            // Only publish occasionally to avoid spamming the database (e.g. 10 times a sec)
            if (Time.frameCount % 6 == 0)
            {
                // Format timestamp in milliseconds like the Python script does
                long unixTimeMs = System.DateTimeOffset.Now.ToUnixTimeMilliseconds();
                string jsonPayload = $"{{\"device_id\": \"unity_digital_twin\", \"timestamp\": {unixTimeMs}, \"x\": {finalPos.x.ToString("F3")}, \"y\": {finalPos.y.ToString("F3")}, \"z\": {finalPos.z.ToString("F3")}}}";
                mqttManager.PublishMessage("marcopolo/telemetry/location", jsonPayload);
            }
        }
    }
}
