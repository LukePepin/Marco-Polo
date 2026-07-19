using UnityEngine;

public class TrilaterationManager : MonoBehaviour
{
    [Header("The Seeker Spheres")]
    [Tooltip("Drag all 4 of your Seeker-Radar spheres here")]
    public Transform[] seekerSpheres;

    [Header("The Hider Target")]
    [Tooltip("Create a 3D object (like a red cube) to represent the Hider, and drag it here")]
    public Transform targetHider;

    [Header("Algorithm Settings")]
    [Tooltip("Match this to the scale multiplier on your UwbVisualizer scripts!")]
    public float scaleMultiplier = 1.0f;
    
    [Tooltip("How hard the math tries to find the perfect intersection. 100 is great for realtime.")]
    public int solverIterations = 100;
    
    [Tooltip("Learning rate for the gradient descent. 0.1 is very stable.")]
    public float learningRate = 0.1f;

    [Header("Confidence / Uncertainty Range")]
    [Tooltip("Drag a translucent sphere here (child of Hider Target) to visualize the predictive error range.")]
    public Transform uncertaintySphere;
    [Tooltip("Slider to adjust how large the uncertainty range displays based on math error.")]
    [Range(0.1f, 5.0f)]
    public float uncertaintyMultiplier = 1.0f;

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
        targetHider.position = Vector3.Lerp(targetHider.position, bestGuess, Time.deltaTime * 5f);

        // 5. Visualize the "Predictive Range" if the math is uncertain (spheres not overlapping)
        if (uncertaintySphere != null && activeSpheres > 0)
        {
            float averageError = totalError / activeSpheres;
            // The worse the math overlap is, the bigger this confidence sphere grows!
            float visualRange = (averageError * 2f * scaleMultiplier) * uncertaintyMultiplier;
            // Clamp it so it doesn't shrink to invisible if the math is absolutely perfect
            visualRange = Mathf.Max(visualRange, 0.5f); 
            
            Vector3 targetScale = new Vector3(visualRange, visualRange, visualRange);
            uncertaintySphere.localScale = Vector3.Lerp(uncertaintySphere.localScale, targetScale, Time.deltaTime * 5f);
        }
    }
}
