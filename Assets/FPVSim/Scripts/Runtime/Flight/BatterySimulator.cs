using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Cosmetic LiPo model for the OSD: current draw from motor output, capacity used, resting voltage from a
    /// typical LiPo discharge curve and voltage sag from internal resistance. It does not limit thrust (yet).
    /// </summary>
    public sealed class BatterySimulator
    {
        // Resting cell voltage versus state of charge for a typical LiPo.
        private static readonly float[] SocPoints = { 0f, 0.05f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1f };
        private static readonly float[] CellVolts = { 3.30f, 3.50f, 3.62f, 3.70f, 3.74f, 3.77f, 3.80f, 3.84f, 3.88f, 3.95f, 4.05f, 4.20f };

        private float consumedMah;
        private int cells = 4;

        /// <summary>0..1 remaining charge.</summary>
        public float StateOfCharge { get; private set; } = 1f;

        /// <summary>Pack voltage under load, V.</summary>
        public float Voltage { get; private set; }

        /// <summary>Total current draw, A.</summary>
        public float CurrentAmps { get; private set; }

        public float ConsumedMah => consumedMah;

        public float CellVoltage => cells > 0 ? Voltage / cells : 0f;

        public void Reset(DroneTuning tuning)
        {
            consumedMah = 0f;
            StateOfCharge = 1f;
            Step(0f, tuning, 0f);
        }

        /// <param name="averageMotorOutput">Mean motor output 0..1 (fraction of max thrust).</param>
        public void Step(float averageMotorOutput, DroneTuning tuning, float dt)
        {
            if (tuning == null)
            {
                return;
            }

            cells = Mathf.Max(1, tuning.batteryCells);

            // Electrical power grows faster than thrust (roughly thrust^1.5 for a fixed prop).
            float output = Mathf.Clamp01(averageMotorOutput);
            CurrentAmps = tuning.idleCurrentAmps + tuning.maxCurrentAmps * Mathf.Pow(output, 1.5f);

            consumedMah += CurrentAmps * dt * (1000f / 3600f);
            StateOfCharge = Mathf.Clamp01(1f - consumedMah / Mathf.Max(1f, tuning.batteryCapacityMah));

            float restingPerCell = RestingCellVoltage(StateOfCharge);
            float sagPerCell = CurrentAmps * tuning.cellInternalResistance;
            Voltage = Mathf.Max(0f, (restingPerCell - sagPerCell) * cells);
        }

        private static float RestingCellVoltage(float stateOfCharge)
        {
            for (int i = 1; i < SocPoints.Length; i++)
            {
                if (stateOfCharge <= SocPoints[i])
                {
                    float t = Mathf.InverseLerp(SocPoints[i - 1], SocPoints[i], stateOfCharge);
                    return Mathf.Lerp(CellVolts[i - 1], CellVolts[i], t);
                }
            }

            return CellVolts[CellVolts.Length - 1];
        }
    }
}
