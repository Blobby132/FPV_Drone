using FPVSim.Controls;
using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Angle (self-level) mode outer loop. Turns the right stick into a target tilt, compares it with the
    /// current attitude and returns a body-rate setpoint for the rate PID. Yaw stays rate based.
    /// </summary>
    public static class AngleController
    {
        /// <returns>Rate setpoint as an RPY vector, rad/s.</returns>
        public static Vector3 ComputeSetpoint(in PilotCommand command, Quaternion attitude, DroneTuning tuning)
        {
            float maxAngle = Mathf.Clamp(tuning.angleMaxDeg, 5f, 85f) * Mathf.Deg2Rad;
            float rollTarget = Mathf.Clamp(command.roll, -1f, 1f) * maxAngle;
            float pitchTarget = Mathf.Clamp(command.pitch, -1f, 1f) * maxAngle;

            Vector3 up = attitude * Vector3.up;
            Quaternion heading = HeadingOf(attitude);

            // Target "up" vector in the heading frame. Using tan() per axis means each axis tilts by exactly its
            // own target angle when the other is zero (no Euler-order artifacts). +roll tilts up towards +X
            // (right), +pitch tilts up towards +Z (forward, nose down).
            Vector3 targetUpHeading = new Vector3(Mathf.Tan(rollTarget), 1f, Mathf.Tan(pitchTarget)).normalized;
            Vector3 targetUp = heading * targetUpHeading;

            // Rotation vector (world space) that takes the current up vector onto the target up vector.
            Vector3 axis = Vector3.Cross(up, targetUp);
            float sinAngle = axis.magnitude;
            float cosAngle = Vector3.Dot(up, targetUp);
            Vector3 errorLocal;
            if (sinAngle > 1e-5f)
            {
                float angle = Mathf.Atan2(sinAngle, cosAngle);
                errorLocal = Quaternion.Inverse(attitude) * (axis * (angle / sinAngle));
            }
            else if (cosAngle > 0f)
            {
                errorLocal = Vector3.zero; // already there
            }
            else
            {
                errorLocal = new Vector3(0f, 0f, Mathf.PI); // exactly upside down: roll over
            }

            // Only roll and pitch are levelled (the error has no yaw component by construction).
            Vector3 errorRpy = BodyAxes.LocalToRpy(errorLocal);
            Vector3 levelRate = new Vector3(errorRpy.x, errorRpy.y, 0f) * tuning.angleStrength;
            levelRate = Vector3.ClampMagnitude(levelRate, tuning.angleMaxLevelRate * Mathf.Deg2Rad);

            // Yaw about the world vertical (expressed in body axes) so yawing while tilted doesn't fight the
            // levelling loop.
            float yawRate = BetaflightRates.RateDegPerSec(command.yaw, tuning.yawRates) * Mathf.Deg2Rad;
            Vector3 worldUpLocal = Quaternion.Inverse(attitude) * Vector3.up;
            Vector3 yawRpy = BodyAxes.LocalToRpy(worldUpLocal * yawRate);

            return levelRate + yawRpy;
        }

        /// <summary>
        /// Yaw-only rotation describing where the quad is heading. Robust when the nose points straight up or
        /// down (falls back to the body's up/down vector projected on the ground).
        /// </summary>
        public static Quaternion HeadingOf(Quaternion attitude)
        {
            Vector3 forward = attitude * Vector3.forward;
            Vector3 flat = new Vector3(forward.x, 0f, forward.z);
            if (flat.sqrMagnitude < 0.01f)
            {
                // Nose up: the body's up vector points backwards, so heading = -up. Nose down: heading = +up.
                Vector3 up = attitude * Vector3.up;
                Vector3 alternative = forward.y > 0f ? -up : up;
                flat = new Vector3(alternative.x, 0f, alternative.z);
            }

            if (flat.sqrMagnitude < 1e-6f)
            {
                flat = Vector3.forward;
            }

            return Quaternion.LookRotation(flat.normalized, Vector3.up);
        }

        /// <summary>Cosine of the tilt angle between body up and world up (1 = level, 0 = on its side).</summary>
        public static float TiltCosine(Quaternion attitude)
        {
            return Vector3.Dot(attitude * Vector3.up, Vector3.up);
        }
    }
}
