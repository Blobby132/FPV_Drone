using System;
using UnityEngine;
using UnityEngine.InputSystem;

namespace FPVSim.Controls
{
    /// <summary>
    /// Owns the game's input actions, exposes raw stick values and raises events for the utility buttons.
    /// Sticks are read in FixedUpdate by <see cref="PilotCommandSource"/>; buttons are polled here once per
    /// frame (Update) so presses are never missed or doubled by the physics rate.
    /// </summary>
    [DefaultExecutionOrder(-100)]
    [DisallowMultipleComponent]
    public sealed class PilotInputReader : MonoBehaviour, IStickInput
    {
        private InputActionAsset actions;
        private InputActionMap flightMap;
        private InputActionMap systemMap;
        private InputAction leftStick;
        private InputAction rightStick;
        private InputAction toggleFlightMode;
        private InputAction respawn;
        private InputAction toggleCamera;
        private InputAction cameraTiltUp;
        private InputAction cameraTiltDown;
        private InputAction pause;

        /// <summary>Triangle by default.</summary>
        public event Action FlightModeTogglePressed;

        /// <summary>Circle by default.</summary>
        public event Action RespawnPressed;

        /// <summary>Square by default.</summary>
        public event Action CameraTogglePressed;

        /// <summary>D-pad up (+1) / down (-1) by default.</summary>
        public event Action<int> CameraTiltStepPressed;

        /// <summary>Options by default. Works while the flight controls are disabled.</summary>
        public event Action PausePressed;

        /// <summary>The runtime action asset (used for rebinding and saving binding overrides).</summary>
        public InputActionAsset Actions
        {
            get
            {
                EnsureCreated();
                return actions;
            }
        }

        public bool FlightControlsEnabled => flightMap != null && flightMap.enabled;

        public Vector2 LeftStick => FlightControlsEnabled ? leftStick.ReadValue<Vector2>() : Vector2.zero;

        public Vector2 RightStick => FlightControlsEnabled ? rightStick.ReadValue<Vector2>() : Vector2.zero;

        /// <summary>The gamepad that most recently sent input, or null (used for rumble).</summary>
        public Gamepad ActiveGamepad => Gamepad.current;

        private void Awake()
        {
            DisableBuiltInStickDeadzone();
            EnsureCreated();
        }

        private void OnEnable()
        {
            EnsureCreated();
            actions.Enable();
        }

        private void OnDisable()
        {
            if (actions != null)
            {
                actions.Disable();
            }
        }

        private void OnDestroy()
        {
            if (actions != null)
            {
                Destroy(actions);
                actions = null;
            }
        }

        private void Update()
        {
            if (pause.WasPressedThisFrame())
            {
                PausePressed?.Invoke();
            }

            if (!FlightControlsEnabled)
            {
                return;
            }

            if (toggleFlightMode.WasPressedThisFrame())
            {
                FlightModeTogglePressed?.Invoke();
            }

            if (respawn.WasPressedThisFrame())
            {
                RespawnPressed?.Invoke();
            }

            if (toggleCamera.WasPressedThisFrame())
            {
                CameraTogglePressed?.Invoke();
            }

            if (cameraTiltUp.WasPressedThisFrame())
            {
                CameraTiltStepPressed?.Invoke(+1);
            }

            if (cameraTiltDown.WasPressedThisFrame())
            {
                CameraTiltStepPressed?.Invoke(-1);
            }
        }

        /// <summary>Enables or disables the flight map (sticks + flight buttons). The pause button stays active.</summary>
        public void SetFlightControlsEnabled(bool enabledState)
        {
            EnsureCreated();
            if (enabledState)
            {
                flightMap.Enable();
            }
            else
            {
                flightMap.Disable();
            }
        }

        /// <summary>Finds a rebindable action by name in any map.</summary>
        public InputAction FindAction(string actionName)
        {
            return Actions.FindAction(actionName, false);
        }

        private void EnsureCreated()
        {
            if (actions != null)
            {
                return;
            }

            actions = FpvInputActions.Create();
            flightMap = actions.FindActionMap(FpvInputActions.FlightMapName, true);
            systemMap = actions.FindActionMap(FpvInputActions.SystemMapName, true);
            leftStick = flightMap.FindAction(FpvInputActions.LeftStick, true);
            rightStick = flightMap.FindAction(FpvInputActions.RightStick, true);
            toggleFlightMode = flightMap.FindAction(FpvInputActions.ToggleFlightMode, true);
            respawn = flightMap.FindAction(FpvInputActions.Respawn, true);
            toggleCamera = flightMap.FindAction(FpvInputActions.ToggleCamera, true);
            cameraTiltUp = flightMap.FindAction(FpvInputActions.CameraTiltUp, true);
            cameraTiltDown = flightMap.FindAction(FpvInputActions.CameraTiltDown, true);
            pause = systemMap.FindAction(FpvInputActions.Pause, true);
        }

        /// <summary>
        /// Gamepad sticks carry a built-in "stickDeadzone" processor that uses the global Input System defaults
        /// (0.125 min / 0.925 max). We want the raw value so our own configurable deadzone (default 0.05) is the
        /// only one applied. Setting the global defaults to 0 / 1 turns that processor into a pass-through.
        /// A copy of the settings object is used so a project settings asset (if you have one) is not modified.
        /// </summary>
        private static void DisableBuiltInStickDeadzone()
        {
            InputSettings current = InputSystem.settings;
            if (current == null)
            {
                return;
            }

            // ReSharper disable CompareOfFloatsByEqualityOperator
            if (current.defaultDeadzoneMin == 0f && current.defaultDeadzoneMax == 1f)
            {
                return;
            }
            // ReSharper restore CompareOfFloatsByEqualityOperator

            InputSettings copy = Instantiate(current);
            copy.name = current.name + " (FPV Sim runtime)";
            copy.defaultDeadzoneMin = 0f;
            copy.defaultDeadzoneMax = 1f;
            InputSystem.settings = copy;
        }
    }
}
