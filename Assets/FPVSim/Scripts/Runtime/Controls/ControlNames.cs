using System;
using UnityEngine.InputSystem;

namespace FPVSim.Controls
{
    /// <summary>Human-readable, PlayStation-style names for gamepad bindings ("Triangle", "L1", "D-Pad Up").</summary>
    public static class ControlNames
    {
        /// <summary>Name of the gamepad binding of <paramref name="action"/> (or "-" if it has none).</summary>
        public static string ForAction(InputAction action)
        {
            if (action == null)
            {
                return "-";
            }

            int index = action.GetBindingIndex(group: FpvInputActions.GamepadGroup);
            return index < 0 ? "-" : PlayStationName(action.bindings[index].effectivePath);
        }

        /// <summary>
        /// Converts a control path such as "&lt;Gamepad&gt;/buttonNorth" or "&lt;DualSenseGamepadHID&gt;/dpad/up"
        /// into a PlayStation button name.
        /// </summary>
        public static string PlayStationName(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return "-";
            }

            switch (ControlPart(path).ToLowerInvariant())
            {
                case "buttonsouth": return "Cross";
                case "buttoneast": return "Circle";
                case "buttonwest": return "Square";
                case "buttonnorth": return "Triangle";
                case "start": return "Options";
                case "select": return "Create";
                case "leftshoulder": return "L1";
                case "rightshoulder": return "R1";
                case "lefttrigger": return "L2";
                case "righttrigger": return "R2";
                case "leftstickpress": return "L3";
                case "rightstickpress": return "R3";
                case "dpad/up": return "D-Pad Up";
                case "dpad/down": return "D-Pad Down";
                case "dpad/left": return "D-Pad Left";
                case "dpad/right": return "D-Pad Right";
                case "touchpadbutton": return "Touchpad";
                case "systembutton": return "PS";
                case "micbutton": return "Mute";
                default:
                    return InputControlPath.ToHumanReadableString(path, InputControlPath.HumanReadableStringOptions.OmitDevice);
            }
        }

        /// <summary>The part of a control path after the device, e.g. "dpad/up".</summary>
        public static string ControlPart(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return string.Empty;
            }

            int deviceEnd = path.IndexOf('>');
            int slash = path.IndexOf('/', deviceEnd < 0 ? 0 : deviceEnd);
            return slash < 0 ? path : path.Substring(slash + 1);
        }

        /// <summary>True if two paths address the same control regardless of the device layout prefix.</summary>
        public static bool SameControl(string pathA, string pathB)
        {
            return string.Equals(ControlPart(pathA), ControlPart(pathB), StringComparison.OrdinalIgnoreCase);
        }
    }
}
