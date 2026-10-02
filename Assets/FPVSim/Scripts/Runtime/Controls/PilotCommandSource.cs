using FPVSim.Flight;
using FPVSim.Settings;
using UnityEngine;

namespace FPVSim.Controls
{
    /// <summary>
    /// Turns raw thumbstick values into a <see cref="PilotCommand"/>:
    /// invert -> deadzone -> expo (per physical stick) -> Mode 2 / Mode 1 mapping -> smoothing -> throttle mode.
    ///
    /// Runs inside FixedUpdate with the fixed timestep so the stateful parts (smoothing, latched throttle) are
    /// frame-rate independent and deterministic for a given input sequence.
    /// </summary>
    public sealed class PilotCommandSource : IPilotCommandSource
    {
        private readonly IStickInput sticks;
        private readonly PilotSettings settings;

        private float smoothedRoll;
        private float smoothedPitch;
        private float smoothedYaw;
        private float smoothedThrottleAxis;
        private float latchedThrottle;

        public PilotCommandSource(IStickInput sticks, PilotSettings settings)
        {
            this.sticks = sticks;
            this.settings = settings;
        }

        /// <summary>Current held throttle in latched mode (0..1 stick position).</summary>
        public float LatchedThrottle => latchedThrottle;

        public void ResetState()
        {
            smoothedRoll = smoothedPitch = smoothedYaw = smoothedThrottleAxis = 0f;
            latchedThrottle = 0f;
        }

        public PilotCommand ReadCommand(float dt, FlightMode mode)
        {
            if (sticks == null || settings == null)
            {
                return PilotCommand.Idle;
            }

            Vector2 left = sticks.LeftStick;
            Vector2 right = sticks.RightStick;

            StickSettings ls = settings.leftStick;
            StickSettings rs = settings.rightStick;
            float lx = StickShaping.Shape(left.x, ls.deadzone, ls.expo, ls.invertX);
            float ly = StickShaping.Shape(left.y, ls.deadzone, ls.expo, ls.invertY);
            float rx = StickShaping.Shape(right.x, rs.deadzone, rs.expo, rs.invertX);
            float ry = StickShaping.Shape(right.y, rs.deadzone, rs.expo, rs.invertY);

            // Yaw is always left X and roll always right X; throttle and pitch swap between modes.
            float yaw = lx;
            float roll = rx;
            float pitch;
            float throttleAxis;
            if (settings.stickMode == StickMode.Mode1)
            {
                pitch = ly;
                throttleAxis = ry;
            }
            else
            {
                throttleAxis = ly;
                pitch = ry;
            }

            float smoothing = mode == FlightMode.Acro ? settings.smoothingAcro : settings.smoothingAngle;
            smoothedRoll = StickShaping.Smooth(smoothedRoll, roll, smoothing, dt);
            smoothedPitch = StickShaping.Smooth(smoothedPitch, pitch, smoothing, dt);
            smoothedYaw = StickShaping.Smooth(smoothedYaw, yaw, smoothing, dt);
            smoothedThrottleAxis = StickShaping.Smooth(smoothedThrottleAxis, throttleAxis, smoothing, dt);

            float throttle;
            if (settings.throttleMode == ThrottleMode.Latched)
            {
                // Stick deflection is a ramp rate; releasing the stick (spring to center) holds the value.
                latchedThrottle = Mathf.Clamp01(
                    latchedThrottle + smoothedThrottleAxis * settings.latchedThrottleRampSpeed * dt);
                throttle = latchedThrottle;
            }
            else
            {
                // Hover-centered: center = 0.5 = the throttle curve's mid point (hover by default).
                throttle = Mathf.Clamp01(0.5f + 0.5f * smoothedThrottleAxis);
                latchedThrottle = throttle; // switching to latched mode keeps the current throttle
            }

            return new PilotCommand
            {
                roll = smoothedRoll,
                pitch = smoothedPitch,
                yaw = smoothedYaw,
                throttle = throttle,
            };
        }
    }
}
