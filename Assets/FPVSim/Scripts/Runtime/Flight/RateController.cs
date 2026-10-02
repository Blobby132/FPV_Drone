using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>Three-axis rate PID. Inputs and outputs are RPY vectors (see <see cref="BodyAxes"/>).</summary>
    public sealed class RateController
    {
        private readonly PidAxis roll = new PidAxis();
        private readonly PidAxis pitch = new PidAxis();
        private readonly PidAxis yaw = new PidAxis();

        /// <param name="setpointRpy">Desired body rates, rad/s.</param>
        /// <param name="measuredRpy">Measured body rates (the "gyro"), rad/s.</param>
        /// <returns>Desired angular acceleration per axis, rad/s^2.</returns>
        public Vector3 Step(Vector3 setpointRpy, Vector3 measuredRpy, DroneTuning tuning, float dt, bool integrate)
        {
            float dCut = tuning.dTermCutoffHz;
            float ffCut = tuning.feedforwardCutoffHz;
            float iLimit = tuning.iTermLimit;
            return new Vector3(
                roll.Step(setpointRpy.x, measuredRpy.x, tuning.rollPid, dCut, ffCut, iLimit, dt, integrate),
                pitch.Step(setpointRpy.y, measuredRpy.y, tuning.pitchPid, dCut, ffCut, iLimit, dt, integrate),
                yaw.Step(setpointRpy.z, measuredRpy.z, tuning.yawPid, dCut, ffCut, iLimit, dt, integrate));
        }

        public void Reset()
        {
            roll.Reset();
            pitch.Reset();
            yaw.Reset();
        }

        public void ResetIntegrals()
        {
            roll.ResetIntegral();
            pitch.ResetIntegral();
            yaw.ResetIntegral();
        }
    }
}
