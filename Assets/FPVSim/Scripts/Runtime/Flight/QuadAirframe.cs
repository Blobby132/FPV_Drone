using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Quad-X geometry and the derived physical quantities needed by the mixer, the motor forces and the
    /// flight controller's inverse model. Cheap to compute; rebuilt every physics step so tuning changes
    /// apply live.
    ///
    /// Motor order follows Betaflight's Quad-X: 0 = rear right, 1 = front right, 2 = rear left, 3 = front left.
    /// Default prop direction is Betaflight "props in": rear-right and front-left spin clockwise (seen from
    /// above), front-right and rear-left spin counter-clockwise.
    /// </summary>
    public readonly struct QuadAirframe
    {
        public const int MotorCount = 4;

        /// <summary>Motor position signs (x = right, y = forward) in units of <see cref="motorOffset"/>.</summary>
        public static readonly Vector2[] MotorSigns =
        {
            new Vector2(+1f, -1f), // 0: rear right
            new Vector2(+1f, +1f), // 1: front right
            new Vector2(-1f, -1f), // 2: rear left
            new Vector2(-1f, +1f), // 3: front left
        };

        /// <summary>
        /// +1 for props spinning counter-clockwise seen from above. Their drag torque on the frame is clockwise,
        /// i.e. it yaws the quad to the right (+Y in Unity).
        /// </summary>
        public static readonly float[] SpinDirection = { -1f, +1f, +1f, -1f };

        /// <summary>Maximum thrust of one motor, N.</summary>
        public readonly float maxThrustPerMotor;

        /// <summary>Distance of each motor from the center along X and along Z, m (armLength * cos 45).</summary>
        public readonly float motorOffset;

        /// <summary>Fraction of max thrust that the mixer controls on top of idle: (1 - idle).</summary>
        public readonly float throttleSpan;

        /// <summary>Torque produced by a unit mixer command on each axis (roll, pitch, yaw), N*m.</summary>
        public readonly Vector3 torquePerCommandRpy;

        /// <summary>Moment of inertia per axis (roll, pitch, yaw), kg*m^2.</summary>
        public readonly Vector3 inertiaRpy;

        /// <summary>Weight force, N.</summary>
        public readonly float weight;

        public QuadAirframe(DroneTuning tuning, float gravity)
        {
            weight = tuning.massKg * gravity;
            maxThrustPerMotor = tuning.thrustToWeight * weight / MotorCount;
            motorOffset = tuning.armLength * 0.70710678f;
            throttleSpan = 1f - Mathf.Clamp(tuning.motorIdle, 0f, 0.5f);

            // A unit roll/pitch command moves each motor by +/-1 (in mixer units), i.e. by
            // throttleSpan * maxThrust newtons, at a lever arm of motorOffset: 4 motors * F * a.
            float leverTorque = MotorCount * maxThrustPerMotor * throttleSpan * motorOffset;
            float yawTorque = MotorCount * maxThrustPerMotor * throttleSpan * tuning.yawTorqueCoefficient;
            torquePerCommandRpy = new Vector3(leverTorque, leverTorque, yawTorque);
            inertiaRpy = new Vector3(tuning.inertiaRoll, tuning.inertiaPitch, tuning.inertiaYaw);
        }

        /// <summary>Local position of motor <paramref name="index"/> relative to the center of mass.</summary>
        public Vector3 MotorLocalPosition(int index)
        {
            Vector2 s = MotorSigns[index];
            return new Vector3(s.x * motorOffset, 0f, s.y * motorOffset);
        }

        /// <summary>Converts a desired angular acceleration (RPY, rad/s^2) into normalized mixer commands.</summary>
        public Vector3 AccelerationToCommand(Vector3 accelerationRpy)
        {
            return new Vector3(
                accelerationRpy.x * inertiaRpy.x / Mathf.Max(torquePerCommandRpy.x, 1e-6f),
                accelerationRpy.y * inertiaRpy.y / Mathf.Max(torquePerCommandRpy.y, 1e-6f),
                accelerationRpy.z * inertiaRpy.z / Mathf.Max(torquePerCommandRpy.z, 1e-6f));
        }
    }
}
