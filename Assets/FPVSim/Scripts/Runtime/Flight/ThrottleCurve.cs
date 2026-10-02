using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Throttle curve with a configurable mid point and expo, similar to Betaflight's thr_mid / thr_expo.
    /// The curve always passes through (0, 0), (0.5, mid) and (1, 1). Expo flattens the curve around the
    /// mid point, which gives finer altitude control near hover.
    /// </summary>
    public static class ThrottleCurve
    {
        /// <param name="stick">Virtual throttle stick position, 0..1.</param>
        /// <param name="mid">Output throttle at half stick (usually the hover throttle).</param>
        /// <param name="expo">0 = linear on both halves, 1 = maximum flattening around mid.</param>
        public static float Evaluate(float stick, float mid, float expo)
        {
            stick = Mathf.Clamp01(stick);
            mid = Mathf.Clamp(mid, 0.01f, 0.99f);
            expo = Mathf.Clamp01(expo);

            // Map stick to [-1, 1] around the mid point, shape it, then map each half separately.
            float u = (stick - 0.5f) * 2f;
            float shaped = u * (1f - expo) + expo * u * u * u;
            return shaped >= 0f
                ? mid + shaped * (1f - mid)
                : mid + shaped * mid;
        }

        /// <summary>
        /// Mixer throttle (0..1, before motor idle is added) at which total thrust equals weight when level.
        /// Motor output = idle + (1 - idle) * throttle and thrust is linear in motor output, so
        /// hover output = 1 / TWR.
        /// </summary>
        public static float HoverThrottle(float thrustToWeight, float motorIdle)
        {
            float hoverOutput = 1f / Mathf.Max(thrustToWeight, 1.01f);
            float idle = Mathf.Clamp(motorIdle, 0f, 0.5f);
            return Mathf.Clamp01((hoverOutput - idle) / (1f - idle));
        }
    }
}
