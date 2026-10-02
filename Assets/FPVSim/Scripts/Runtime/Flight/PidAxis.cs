using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// One axis of the rate controller: P + I + D (on measurement) + feedforward.
    ///
    /// Units: setpoint and measurement are angular rates in rad/s. The output is a desired angular
    /// acceleration in rad/s^2. <see cref="FlightController"/> turns that into a normalized mixer command using
    /// the airframe's inertia and torque authority, so these gains behave the same when you change mass,
    /// thrust or arm length (like a well-tuned flight controller would).
    ///
    /// Gain scaling (Betaflight-like number -> physical meaning):
    ///   P  * 1.0   = rate-loop bandwidth in 1/s       (P 45 -> error corrected with a ~22 ms time constant)
    ///   I  * 2.0   = integral gain in 1/s^2
    ///   D  * 0.012 = damping on measured angular acceleration (dimensionless)
    ///   FF * 0.006 = fraction of the ideal "setpoint acceleration" feedforward
    /// </summary>
    public sealed class PidAxis
    {
        public const float PScale = 1.0f;
        public const float IScale = 2.0f;
        public const float DScale = 0.012f;
        public const float FFScale = 0.006f;

        private float integral;          // rad
        private float previousMeasurement;
        private float previousSetpoint;
        private float dFiltered;         // rad/s^2
        private float ffFiltered;        // rad/s^2
        private bool hasHistory;

        /// <summary>Last individual term outputs, rad/s^2 (for debugging / future blackbox).</summary>
        public float LastP { get; private set; }
        public float LastI { get; private set; }
        public float LastD { get; private set; }
        public float LastFF { get; private set; }

        public void Reset()
        {
            integral = 0f;
            dFiltered = 0f;
            ffFiltered = 0f;
            hasHistory = false;
            LastP = LastI = LastD = LastFF = 0f;
        }

        public void ResetIntegral()
        {
            integral = 0f;
        }

        /// <param name="integrate">False freezes and clears the I term (e.g. sitting on the ground at low throttle).</param>
        public float Step(float setpoint, float measurement, in PidGains gains, float dCutoffHz, float ffCutoffHz,
            float iTermLimit, float dt, bool integrate)
        {
            if (dt <= 0f)
            {
                return 0f;
            }

            if (!hasHistory)
            {
                // Avoid a derivative kick on the first step after a reset.
                previousMeasurement = measurement;
                previousSetpoint = setpoint;
                hasHistory = true;
            }

            float error = setpoint - measurement;

            float pTerm = gains.p * PScale * error;

            float iGain = gains.i * IScale;
            if (integrate && iGain > 0f)
            {
                integral += error * dt;
                float integralLimit = iTermLimit / iGain;
                integral = Mathf.Clamp(integral, -integralLimit, integralLimit);
            }
            else
            {
                integral = 0f;
            }

            float iTerm = iGain * integral;

            // Derivative on measurement (not on error) so stick movements don't cause a D kick.
            float dRaw = (measurement - previousMeasurement) / dt;
            dFiltered += (dRaw - dFiltered) * LowPassAlpha(dCutoffHz, dt);
            float dTerm = -gains.d * DScale * dFiltered;

            // Feedforward on the setpoint derivative, filtered because sticks are sampled at frame rate
            // while this loop runs at physics rate (the setpoint arrives as a staircase).
            float ffRaw = (setpoint - previousSetpoint) / dt;
            ffFiltered += (ffRaw - ffFiltered) * LowPassAlpha(ffCutoffHz, dt);
            float ffTerm = gains.ff * FFScale * ffFiltered;

            previousMeasurement = measurement;
            previousSetpoint = setpoint;

            LastP = pTerm;
            LastI = iTerm;
            LastD = dTerm;
            LastFF = ffTerm;
            return pTerm + iTerm + dTerm + ffTerm;
        }

        /// <summary>Smoothing factor of a first-order low-pass filter for one step of length dt.</summary>
        public static float LowPassAlpha(float cutoffHz, float dt)
        {
            if (cutoffHz <= 0f)
            {
                return 1f;
            }

            return 1f - Mathf.Exp(-2f * Mathf.PI * cutoffHz * dt);
        }
    }
}
