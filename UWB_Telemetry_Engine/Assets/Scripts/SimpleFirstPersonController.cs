using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class SimpleFirstPersonController : MonoBehaviour
{
    [Header("Movement Settings")]
    public float walkSpeed = 5f;
    public float verticalSpeed = 3f;
    
    [Header("Look Settings")]
    public float lookSensitivity = 2f;
    public Camera playerCamera;
    
    [Header("Environment Toggles")]
    public GameObject polycamRoom;
    public GameObject unityBlocksRoom;
    [Tooltip("The ceiling object to hide when in top-down ISO view")]
    public GameObject ceilingObject;
    
    [Header("ISO View Settings")]
    [Tooltip("The exact position the player teleports to when hitting M")]
    public Vector3 isoPosition = new Vector3(0f, 8f, 0f);
    [Tooltip("The exact rotation (X, Y, Z) the camera looks at when hitting M")]
    public Vector3 isoCameraRotation = new Vector3(90f, 0f, 0f);
    
    private CharacterController controller;
    private float verticalRotation = 0f;

    private bool isCameraLocked = false;
    private bool isIsoView = false;
    
    // To remember where we were before switching to ISO
    private Vector3 preIsoPosition;
    private Quaternion preIsoRotation;
    private Quaternion preIsoCameraRotation;
    private float preIsoVerticalRotation;

    void Start()
    {
        // Get the required component
        controller = GetComponent<CharacterController>();
        
        // Lock the mouse cursor to the center of the screen and hide it
        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;
        
        // Fix Camera Start Rotation: Read the exact angle you set in the editor so it doesn't snap backwards!
        if (playerCamera != null)
        {
            verticalRotation = playerCamera.transform.localEulerAngles.x;
            if (verticalRotation > 180f) verticalRotation -= 360f; // Convert 0-360 to -180 to 180
        }
    }

    void Update()
    {
        // Toggle environments with 1 and 2
        if (Input.GetKeyDown(KeyCode.Alpha1) && polycamRoom != null)
        {
            polycamRoom.SetActive(!polycamRoom.activeSelf);
        }
        if (Input.GetKeyDown(KeyCode.Alpha2) && unityBlocksRoom != null)
        {
            unityBlocksRoom.SetActive(!unityBlocksRoom.activeSelf);
        }

        // Toggle Top-Down ISO View with M
        if (Input.GetKeyDown(KeyCode.M))
        {
            isIsoView = !isIsoView;
            
            if (isIsoView)
            {
                // Save current position and jump to the ceiling
                preIsoPosition = transform.position;
                preIsoRotation = transform.rotation;
                if (playerCamera != null) preIsoCameraRotation = playerCamera.transform.localRotation;
                preIsoVerticalRotation = verticalRotation;
                
                // Disable CharacterController temporarily to teleport
                controller.enabled = false;
                transform.position = isoPosition;
                transform.rotation = Quaternion.Euler(0f, 0f, 0f); // Keep player body upright
                
                if (playerCamera != null) 
                {
                    playerCamera.transform.localRotation = Quaternion.Euler(isoCameraRotation);
                    verticalRotation = isoCameraRotation.x; // Sync the mouse look state to the new angle
                }
                
                if (ceilingObject != null) ceilingObject.SetActive(false);
                isCameraLocked = true; // Lock the camera so we can't look around while in map mode
            }
            else
            {
                // Restore previous state
                transform.position = preIsoPosition;
                transform.rotation = preIsoRotation;
                if (playerCamera != null) playerCamera.transform.localRotation = preIsoCameraRotation;
                verticalRotation = preIsoVerticalRotation; // Restore the mouse look state
                
                controller.enabled = true;
                
                if (ceilingObject != null) ceilingObject.SetActive(true);
                isCameraLocked = false;
            }
        }

        // 1. MOUSE LOOK (Looking around)
        if (!isCameraLocked)
        {
            float mouseX = Input.GetAxis("Mouse X") * lookSensitivity;
            float mouseY = Input.GetAxis("Mouse Y") * lookSensitivity;

            // Calculate up/down rotation and clamp it so you don't snap your neck backwards
            verticalRotation -= mouseY;
            verticalRotation = Mathf.Clamp(verticalRotation, -90f, 90f);

            // Apply up/down to the camera, and left/right to the entire player body
            if (playerCamera != null)
            {
                playerCamera.transform.localRotation = Quaternion.Euler(verticalRotation, 0f, 0f);
            }
            transform.Rotate(Vector3.up * mouseX);
        }

        // 2. WASD MOVEMENT (Walking)
        float moveForward = Input.GetAxis("Vertical");   // W / S
        float moveRight = Input.GetAxis("Horizontal"); // A / D

        // Calculate the direction to walk based on where we are currently looking
        Vector3 moveDirection = transform.right * moveRight + transform.forward * moveForward;
        
        // Apply horizontal walk speed
        moveDirection.x *= walkSpeed;
        moveDirection.z *= walkSpeed;
        
        // 3. VERTICAL MOVEMENT (Flying strictly locked to E and Q)
        float moveUp = 0f;
        if (Input.GetKey(KeyCode.E)) moveUp = 1f;
        if (Input.GetKey(KeyCode.Q)) moveUp = -1f;
        
        moveDirection.y = moveUp * verticalSpeed; 

        // Apply the final movement to the Character Controller
        if (controller.enabled)
        {
            controller.Move(moveDirection * Time.deltaTime);
        }
        
        // Unlock cursor if Escape is pressed (helpful for testing)
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
    }
}
