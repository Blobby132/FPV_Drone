using FPVSim.Controls;
using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>Sensor data the flight controller sees each step.</summary>
    public struct FlightState
    {
        /// <summary>World attitude of the airframe.</summary>
        public Quaternion attitude;

        /// <summary>Body rates (the "gyro") as an RPY vector, rad/s.</summary>
        public Vector3 bodyRatesRpy;

        /// <summary>True while touching something (used to hold the I term on the ground).</summary>
        public bool inContact;
    }

    /// <summary>What the flight controller sends to the mixer.</summary>
    public readonly struct FlightOutput
    {
        /// <summary>Collective throttle 0..1 after the throttle curve.</summary>
        public readonly float throttle;

        /// <summary>Normalized roll / pitch / yaw mixer commands.</summary>
        public readonly Vector3 axisCommandRpy;

        /// <summary>Rate setpoint that was tracked, RPY rad/s.</summary>
        public readonly Vector3 setpointRpy;

        public FlightOutput(float throttle, Vector3 axisCommandRpy, Vector3 setpointRpy)
        {
            this.throttle = throttle;
            this.axisCommandRpy = axisCommandRpy;
            this.setpointRpy = setpointRpy;
        }
    }

    /// <summary>
    /// The simulated flight controller: throttle curve, setpoint generation (angle mode outer loop or acro
    /// rates), and the rate PID. Pure C# with no Unity component state, so it is deterministic and easy to test.
    /// </summary>
    public sealed class FlightController
    {
        private readonly RateController rateController = new RateController();

        public void Reset()
        {
            rateController.Reset();
        }

        /// <summary>Call when the flight mode changes so the I term doesn't carry over.</summary>
        public void OnModeChanged()
        {
            rateController.ResetIntegrals();
        }

        public FlightOutput Step(in PilotCommand command, in FlightState state, FlightMode mode,
            DroneTuning tuning, in QuadAirframe airframe, float dt)
        {
            // 1) Throttle: virtual stick -> curve (mid point = hover by default).
            float mid = tuning.EffectiveThrottleMid;
            float throttle = ThrottleCurve.Evaluate(command.throttle, mid, tuning.throttleExpo);
            if (mode == FlightMode.Angle)
            {
                throttle = ApplyTiltCompensation(throttle, state.attitude, tuning, airframe);
            }

            // 2) Rate setpoint.
            Vector3 setpointRpy = AngleController.ComputeSetpoint(command, state.attitude, tuning);

            // 3) Rate PID. Hold the I term while resting on something at low throttle so it can't wind up
            //    against the ground (Betaflight does the same below a throttle threshold).
            bool integrate = !(state.inContact && throttle < mid * 0.75f);
            Vector3 accelerationRpy = rateController.Step(setpointRpy, state.bodyRatesRpy, tuning, dt, integrate);

            // 4) Inverse model: angular acceleration -> normalized mixer commands.
            Vector3 axisCommand = airframe.AccelerationToCommand(accelerationRpy);
            return new FlightOutput(throttle, axisCommand, setpointRpy);
        }

        /// <summary>
        /// Angle mode helper: raise thrust by 1/cos(tilt) so that a centered throttle stick keeps roughly the same
        /// vertical thrust while tilted. Done in motor-output space because of motor idle.
        /// </summary>
        private static float ApplyTiltCompensation(float throttle, Quaternion attitude, DroneTuning tuning,
            in QuadAirframe airframe)
        {
            float amount = tuning.angleThrottleCompensation;
            float cosTilt = AngleController.TiltCosine(attitude);
            if (amount <= 0f || cosTilt <= 0.3f)
            {
                return throttle;
            }

            float idle = 1f - airframe.throttleSpan;
            float output = idle + airframe.throttleSpan * throttle;
            float boosted = output * Mathf.Lerp(1f, 1f / cosTilt, amount);
            return Mathf.Clamp01((boosted - idle) / airframe.throttleSpan);
        }
    }
}
