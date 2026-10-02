using UnityEngine;
using UnityEngine.InputSystem;

namespace FPVSim.Controls
{
    /// <summary>
    /// Builds the game's <see cref="InputActionAsset"/> in code so there is no asset to wire up and every binding
    /// is visible in one place. Bindings use the generic <c>&lt;Gamepad&gt;</c> layout, which the Input System's
    /// DualSense / DualShock layouts inherit from, so a PS5 controller over USB or Bluetooth works out of the box.
    ///
    /// DualSense names for the generic gamepad controls:
    ///   buttonSouth = Cross, buttonEast = Circle, buttonWest = Square, buttonNorth = Triangle,
    ///   start = Options, select = Create, leftShoulder = L1, rightShoulder = R1.
    /// </summary>
    public static class FpvInputActions
    {
        public const string AssetName = "FPVControls";

        public const string FlightMapName = "Flight";
        public const string SystemMapName = "System";

        public const string LeftStick = "LeftStick";
        public const string RightStick = "RightStick";
        public const string ToggleFlightMode = "ToggleFlightMode";
        public const string Respawn = "Respawn";
        public const string ToggleCamera = "ToggleCamera";
        public const string CameraTiltUp = "CameraTiltUp";
        public const string CameraTiltDown = "CameraTiltDown";
        public const string Pause = "Pause";

        public const string GamepadGroup = "Gamepad";
        public const string KeyboardGroup = "Keyboard";

        /// <summary>Button actions shown in the rebinding menu (gamepad binding only).</summary>
        public static readonly string[] RebindableActions =
        {
            ToggleFlightMode, Respawn, ToggleCamera, CameraTiltUp, CameraTiltDown, Pause,
        };

        public static InputActionAsset Create()
        {
            var asset = ScriptableObject.CreateInstance<InputActionAsset>();
            asset.name = AssetName;

            InputActionMap flight = asset.AddActionMap(FlightMapName);

            // Sticks. Mode 1 / Mode 2 mapping happens in PilotCommandSource, not in the bindings, so the
            // mapping can change at runtime without touching bindings. The keyboard composites are a debug
            // fallback only and go through the same mapping (WASD = left stick, arrows = right stick).
            InputAction left = flight.AddAction(LeftStick, InputActionType.Value, expectedControlLayout: "Vector2");
            left.AddBinding("<Gamepad>/leftStick", groups: GamepadGroup);
            left.AddCompositeBinding("2DVector(mode=1)")
                .With("Up", "<Keyboard>/w", KeyboardGroup)
                .With("Down", "<Keyboard>/s", KeyboardGroup)
                .With("Left", "<Keyboard>/a", KeyboardGroup)
                .With("Right", "<Keyboard>/d", KeyboardGroup);

            InputAction right = flight.AddAction(RightStick, InputActionType.Value, expectedControlLayout: "Vector2");
            right.AddBinding("<Gamepad>/rightStick", groups: GamepadGroup);
            right.AddCompositeBinding("2DVector(mode=1)")
                .With("Up", "<Keyboard>/upArrow", KeyboardGroup)
                .With("Down", "<Keyboard>/downArrow", KeyboardGroup)
                .With("Left", "<Keyboard>/leftArrow", KeyboardGroup)
                .With("Right", "<Keyboard>/rightArrow", KeyboardGroup);

            // Utility buttons (never used for flying).
            AddButton(flight, ToggleFlightMode, "<Gamepad>/buttonNorth", "<Keyboard>/m");
            AddButton(flight, Respawn, "<Gamepad>/buttonEast", "<Keyboard>/r");
            AddButton(flight, ToggleCamera, "<Gamepad>/buttonWest", "<Keyboard>/c");
            AddButton(flight, CameraTiltUp, "<Gamepad>/dpad/up", "<Keyboard>/pageUp");
            AddButton(flight, CameraTiltDown, "<Gamepad>/dpad/down", "<Keyboard>/pageDown");

            // The pause button lives in its own map because it must keep working while the flight map is
            // disabled (menu open).
            InputActionMap system = asset.AddActionMap(SystemMapName);
            AddButton(system, Pause, "<Gamepad>/start", "<Keyboard>/escape");

            return asset;
        }

        private static void AddButton(InputActionMap map, string name, string gamepadPath, string keyboardPath)
        {
            InputAction action = map.AddAction(name, InputActionType.Button);
            action.AddBinding(gamepadPath, groups: GamepadGroup);
            action.AddBinding(keyboardPath, groups: KeyboardGroup);
        }
    }
}
