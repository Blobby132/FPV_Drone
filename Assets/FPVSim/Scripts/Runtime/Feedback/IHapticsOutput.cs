namespace FPVSim.Feedback
{
    /// <summary>
    /// Device-agnostic haptics output. Rumble today; DualSense adaptive triggers or other devices can be added
    /// as further implementations (or extra methods) without touching the gameplay code that triggers effects.
    /// </summary>
    public interface IHapticsOutput
    {
        /// <summary>Runs both rumble motors for <paramref name="duration"/> seconds (unscaled time).</summary>
        void Pulse(float lowFrequency, float highFrequency, float duration);

        /// <summary>Call every frame with <c>Time.unscaledTime</c> so pulses can end on time.</summary>
        void Tick(float unscaledTime);

        /// <summary>Stops all effects immediately.</summary>
        void Stop();
    }
}
