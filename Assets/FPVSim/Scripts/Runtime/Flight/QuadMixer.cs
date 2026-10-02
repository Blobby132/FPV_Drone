using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Betaflight Quad-X mixer with airmode-style desaturation.
    ///
    /// Each motor command = throttle + roll * rollMix + pitch * pitchMix + yaw * yawMix, where the mix table is
    /// Betaflight's (motor order: rear right, front right, rear left, front left):
    ///   roll  right  -> left motors up, right motors down
    ///   pitch down   -> rear motors up, front motors down
    ///   yaw   right  -> counter-clockwise props up, clockwise props down
    /// </summary>
    public sealed class QuadMixer
    {
        private static readonly float[] RollMix = { -1f, -1f, +1f, +1f };
        private static readonly float[] PitchMix = { +1f, -1f, +1f, -1f };
        private static readonly float[] YawMix = { -1f, +1f, +1f, -1f };

        private readonly float[] axisPart = new float[QuadAirframe.MotorCount];

        /// <summary>True if the last mix had to scale the attitude commands down to fit the motor range.</summary>
        public bool Saturated { get; private set; }

        /// <param name="throttle">Collective throttle 0..1 (before motor idle).</param>
        /// <param name="axisRpy">Normalized roll / pitch / yaw commands.</param>
        /// <param name="airmode">Shift throttle so attitude commands always fit (full authority at zero throttle).</param>
        /// <param name="motorCommands">Output: four motor commands in 0..1.</param>
        public void Mix(float throttle, Vector3 axisRpy, bool airmode, float[] motorCommands)
        {
            float min = float.MaxValue;
            float max = float.MinValue;
            for (int i = 0; i < QuadAirframe.MotorCount; i++)
            {
                float m = RollMix[i] * axisRpy.x + PitchMix[i] * axisRpy.y + YawMix[i] * axisRpy.z;
                axisPart[i] = m;
                min = Mathf.Min(min, m);
                max = Mathf.Max(max, m);
            }

            // If the requested differential exceeds the motor range, scale all attitude commands down
            // together so the ratio between axes is preserved.
            float range = max - min;
            float scale = 1f;
            Saturated = range > 1f;
            if (Saturated)
            {
                scale = 1f / range;
                min *= scale;
                max *= scale;
            }

            float t = Mathf.Clamp01(throttle);
            if (airmode)
            {
                // Move the collective so every motor stays within [0, 1]. range <= 1 guarantees -min <= 1 - max.
                t = Mathf.Clamp(t, -min, 1f - max);
            }

            for (int i = 0; i < QuadAirframe.MotorCount; i++)
            {
                motorCommands[i] = Mathf.Clamp01(t + axisPart[i] * scale);
            }
        }
    }
}
