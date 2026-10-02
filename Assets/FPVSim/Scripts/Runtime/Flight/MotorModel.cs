using System.Collections.Generic;
using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Simple brushless motor + prop model. Each motor's output (fraction of max thrust) follows its command
    /// with a first-order lag, using separate spin-up and spin-down time constants. Thrust is linear in output,
    /// which is what a flight controller with thrust linearization effectively gives you.
    /// </summary>
    public sealed class MotorModel
    {
        private readonly float[] outputs = new float[QuadAirframe.MotorCount];

        /// <summary>Current motor outputs, 0..1 of maximum thrust (idle included).</summary>
        public IReadOnlyList<float> Outputs => outputs;

        public float AverageOutput
        {
            get
            {
                float sum = 0f;
                for (int i = 0; i < outputs.Length; i++)
                {
                    sum += outputs[i];
                }

                return sum / outputs.Length;
            }
        }

        public void Reset()
        {
            for (int i = 0; i < outputs.Length; i++)
            {
                outputs[i] = 0f;
            }
        }

        /// <param name="commands">Mixer commands 0..1 (ignored when not <paramref name="running"/>).</param>
        /// <param name="running">False = disarmed: motors spin down to zero.</param>
        public void Step(float[] commands, float idle, float spinUpTime, float spinDownTime, float dt, bool running)
        {
            float span = 1f - idle;
            for (int i = 0; i < outputs.Length; i++)
            {
                float target = running ? idle + span * Mathf.Clamp01(commands[i]) : 0f;
                float timeConstant = target > outputs[i] ? spinUpTime : spinDownTime;
                float alpha = timeConstant > 0f ? 1f - Mathf.Exp(-dt / timeConstant) : 1f;
                outputs[i] += (target - outputs[i]) * alpha;
            }
        }
    }
}
