using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Betaflight "Betaflight rates" model (RC rate, super rate, expo), ported from Betaflight's
    /// <c>applyBetaflightRates()</c>. Converts a stick deflection in [-1, 1] into an angular rate in deg/s.
    /// </summary>
    public static class BetaflightRates
    {
        /// <summary>Betaflight caps every axis at this rate.</summary>
        public const float RateLimitDegPerSec = 1998f;

        private const float RcRateIncremental = 14.54f;

        public static float RateDegPerSec(float stick, in AxisRates rates)
        {
            stick = Mathf.Clamp(stick, -1f, 1f);
            float stickAbs = Mathf.Abs(stick);

            // RC expo: blend between linear and cubic response (Betaflight uses |x|^3 * x).
            float command = stick;
            float expo = Mathf.Clamp01(rates.expo);
            if (expo > 0f)
            {
                command = stick * stickAbs * stickAbs * stickAbs * expo + stick * (1f - expo);
            }

            float rcRate = Mathf.Max(0f, rates.rcRate);
            if (rcRate > 2f)
            {
                rcRate += RcRateIncremental * (rcRate - 2f);
            }

            float angleRate = 200f * rcRate * command;

            // Super rate: 1 / (1 - |stick| * superRate), using the raw (pre-expo) stick magnitude.
            float superRate = Mathf.Clamp(rates.superRate, 0f, 0.99f);
            if (superRate > 0f)
            {
                float superFactor = 1f / Mathf.Clamp(1f - stickAbs * superRate, 0.01f, 1f);
                angleRate *= superFactor;
            }

            return Mathf.Clamp(angleRate, -RateLimitDegPerSec, RateLimitDegPerSec);
        }

        /// <summary>Rate at full stick deflection, in deg/s.</summary>
        public static float MaxRateDegPerSec(in AxisRates rates)
        {
            return RateDegPerSec(1f, rates);
        }
    }
}
