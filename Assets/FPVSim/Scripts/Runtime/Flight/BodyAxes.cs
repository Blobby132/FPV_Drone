using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Converts between Unity's local angular axes and the roll/pitch/yaw ("RPY") convention used by the
    /// flight controller.
    ///
    /// Unity local space (left-handed): +X right, +Y up, +Z forward. Positive rotation follows the
    /// left-hand rule, which gives:
    ///   +X rotation = nose down, +Y rotation = yaw right (clockwise seen from above),
    ///   +Z rotation = roll LEFT (right side goes up).
    ///
    /// RPY vectors are stored in a Vector3 as (x = roll, y = pitch, z = yaw) with
    ///   +roll = roll right, +pitch = nose down / forward, +yaw = yaw right.
    /// This matches the stick directions: right stick right = +roll, right stick up = +pitch,
    /// left stick right = +yaw.
    /// </summary>
    public static class BodyAxes
    {
        /// <summary>Unity local angular vector (x, y, z) to RPY (roll, pitch, yaw).</summary>
        public static Vector3 LocalToRpy(Vector3 local)
        {
            return new Vector3(-local.z, local.x, local.y);
        }

        /// <summary>RPY (roll, pitch, yaw) to Unity local angular vector (x, y, z).</summary>
        public static Vector3 RpyToLocal(Vector3 rpy)
        {
            return new Vector3(rpy.y, rpy.z, -rpy.x);
        }
    }
}
