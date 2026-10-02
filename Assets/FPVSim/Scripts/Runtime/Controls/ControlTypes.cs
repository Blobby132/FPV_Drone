using System;
using FPVSim.Flight;
using UnityEngine;

namespace FPVSim.Controls
{
    /// <summary>Which stick controls throttle and pitch (RC transmitter "mode").</summary>
    public enum StickMode
    {
        /// <summary>Left stick: throttle + yaw. Right stick: pitch + roll. (Most common.)</summary>
        Mode2 = 0,

        /// <summary>Left stick: pitch + yaw. Right stick: throttle + roll.</summary>
        Mode1 = 1,
    }

    /// <summary>How a spring-centered gamepad stick is turned into a throttle value.</summary>
    public enum ThrottleMode
    {
        /// <summary>Stick centered = hover throttle, up = more thrust, down = less.</summary>
        HoverCentered = 0,

        /// <summary>Stick up/down ramps a held throttle value that stays put when released.</summary>
        Latched = 1,
    }

    /// <summary>Deadzone, expo and inversion for one physical thumbstick.</summary>
    [Serializable]
    public struct StickSettings
    {
        [Tooltip("Per-axis deadzone (0..0.5). Values inside it read as zero; the rest is rescaled to the full range.")]
        [Range(0f, 0.5f)] public float deadzone;

        [Tooltip("Stick expo (0..1). Softens the response near center for finer control.")]
        [Range(0f, 1f)] public float expo;

        public bool invertX;
        public bool invertY;

        public StickSettings(float deadzone, float expo)
        {
            this.deadzone = deadzone;
            this.expo = expo;
            invertX = false;
            invertY = false;
        }
    }

    /// <summary>
    /// Pilot command for one physics step, after deadzone / expo / mode mapping / throttle handling.
    /// </summary>
    public struct PilotCommand
    {
        /// <summary>-1..1, positive = roll right.</summary>
        public float roll;

        /// <summary>-1..1, positive = pitch forward (nose down).</summary>
        public float pitch;

        /// <summary>-1..1, positive = yaw right.</summary>
        public float yaw;

        /// <summary>
        /// 0..1 virtual throttle stick position (as on an RC transmitter). Goes through the flight
        /// controller's throttle curve, whose mid point (0.5) is the hover throttle by default.
        /// </summary>
        public float throttle;

        public static PilotCommand Idle => default;
    }

    /// <summary>Raw stick values from whatever device is in use (gamepad or keyboard fallback).</summary>
    public interface IStickInput
    {
        /// <summary>Left stick, -1..1 per axis, up = +Y.</summary>
        Vector2 LeftStick { get; }

        /// <summary>Right stick, -1..1 per axis, up = +Y.</summary>
        Vector2 RightStick { get; }
    }

    /// <summary>
    /// Anything that can fly the drone: the local pilot today; replays, AI or network pilots later.
    /// </summary>
    public interface IPilotCommandSource
    {
        /// <summary>Called once per physics step (FixedUpdate) with the fixed delta time.</summary>
        PilotCommand ReadCommand(float dt, FlightMode mode);

        /// <summary>Clears internal state (latched throttle, smoothing) e.g. on respawn.</summary>
        void ResetState();
    }
}
