using UnityEngine;
using UnityEngine.InputSystem;

namespace FPVSim.Feedback
{
    /// <summary>Rumble on the current gamepad (DualSense / DualShock / XInput all support SetMotorSpeeds).</summary>
    public sealed class GamepadHaptics : IHapticsOutput
    {
        private Gamepad activePad;
        private float stopAt;
        private bool active;

        public void Pulse(float lowFrequency, float highFrequency, float duration)
        {
            Gamepad pad = Gamepad.current;
            if (pad == null)
            {
                return;
            }

            if (activePad != null && activePad != pad)
            {
                activePad.SetMotorSpeeds(0f, 0f);
            }

            // A stronger pulse may extend or override a weaker running one, never the other way round.
            float end = Time.unscaledTime + Mathf.Max(0.01f, duration);
            if (active && end < stopAt && activePad == pad)
            {
                end = stopAt;
            }

            activePad = pad;
            activePad.SetMotorSpeeds(Mathf.Clamp01(lowFrequency), Mathf.Clamp01(highFrequency));
            stopAt = end;
            active = true;
        }

        public void Tick(float unscaledTime)
        {
            if (active && unscaledTime >= stopAt)
            {
                Stop();
            }
        }

        public void Stop()
        {
            if (activePad != null && activePad.added)
            {
                activePad.SetMotorSpeeds(0f, 0f);
            }

            activePad = null;
            active = false;
        }
    }
}
