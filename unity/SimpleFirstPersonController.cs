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
    
    private CharacterController controller;
    private float verticalRotation = 0f;

    private bool isCameraLocked = false;

    void Start()
    {
        // Get the required component
        controller = GetComponent<CharacterController>();
        
        // Lock the mouse cursor to the center of the screen and hide it
        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;
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

        // Toggle camera lock with R
        if (Input.GetKeyDown(KeyCode.R))
        {
            isCameraLocked = !isCameraLocked;
            
            // If locked, show the cursor so you can click around. If unlocked, hide it again.
            Cursor.lockState = isCameraLocked ? CursorLockMode.None : CursorLockMode.Locked;
            Cursor.visible = isCameraLocked;
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
        controller.Move(moveDirection * Time.deltaTime);
        
        // Unlock cursor if Escape is pressed (helpful for testing)
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
    }
}
