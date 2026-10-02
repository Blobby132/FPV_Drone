using UnityEngine;

namespace FPVSim.Controls
{
    /// <summary>Stateless helpers for turning raw thumbstick values into clean pilot input.</summary>
    public static class StickShaping
    {
        /// <summary>Per-axis scaled deadzone: |v| below <paramref name="deadzone"/> is zero, the rest maps to 0..1.</summary>
        public static float ApplyDeadzone(float value, float deadzone)
        {
            deadzone = Mathf.Clamp(deadzone, 0f, 0.95f);
            float magnitude = Mathf.Abs(value);
            if (magnitude <= deadzone)
            {
                return 0f;
            }

            return Mathf.Sign(value) * Mathf.Clamp01((magnitude - deadzone) / (1f - deadzone));
        }

        /// <summary>Classic RC expo: blend between linear and cubic response.</summary>
        public static float ApplyExpo(float value, float expo)
        {
            expo = Mathf.Clamp01(expo);
            return value * (1f - expo) + expo * value * value * value;
        }

        /// <summary>Invert, clamp, deadzone and expo in one go.</summary>
        public static float Shape(float value, float deadzone, float expo, bool invert)
        {
            if (invert)
            {
                value = -value;
            }

            value = Mathf.Clamp(value, -1f, 1f);
            value = ApplyDeadzone(value, deadzone);
            return ApplyExpo(value, expo);
        }

        /// <summary>First-order low-pass. A time constant of 0 disables smoothing.</summary>
        public static float Smooth(float current, float target, float timeConstant, float dt)
        {
            if (timeConstant <= 0f)
            {
                return target;
            }

            return current + (target - current) * (1f - Mathf.Exp(-dt / timeConstant));
        }
    }
}
