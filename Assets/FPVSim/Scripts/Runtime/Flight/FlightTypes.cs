using System;
using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>Flight controller mode. Toggled at runtime (Triangle by default).</summary>
    public enum FlightMode
    {
        /// <summary>Self-levelling: the right stick commands a tilt angle, yaw is rate based.</summary>
        Angle = 0,

        /// <summary>Rate mode: sticks command angular rates, no self-levelling.</summary>
        Acro = 1,
    }

    /// <summary>The three rotation axes in aviation terms.</summary>
    public enum FlightAxis
    {
        Roll = 0,
        Pitch = 1,
        Yaw = 2,
    }

    /// <summary>Betaflight-style rate curve parameters for one axis.</summary>
    [Serializable]
    public struct AxisRates
    {
        [Tooltip("Betaflight 'RC Rate'. Scales the whole curve (1.0 = 200 deg/s at full stick before super rate).")]
        public float rcRate;

        [Tooltip("Betaflight 'Super Rate' (0..0.99). Increases rate towards the ends of the stick travel.")]
        public float superRate;

        [Tooltip("Betaflight 'RC Expo' (0..1). Softens the response around center stick.")]
        public float expo;

        public AxisRates(float rcRate, float superRate, float expo)
        {
            this.rcRate = rcRate;
            this.superRate = superRate;
            this.expo = expo;
        }
    }

    /// <summary>
    /// Rate-loop gains for one axis. The numbers are deliberately in the same ballpark as Betaflight
    /// defaults; see <see cref="PidAxis"/> for how they map onto physical units.
    /// </summary>
    [Serializable]
    public struct PidGains
    {
        [Tooltip("Proportional gain. Roughly the rate-loop bandwidth in 1/s.")]
        public float p;

        [Tooltip("Integral gain. Removes steady-state rate error.")]
        public float i;

        [Tooltip("Derivative gain (on gyro). Damps overshoot caused by motor lag.")]
        public float d;

        [Tooltip("Feedforward. Reacts to how fast the setpoint (stick) is moving for a snappier response.")]
        public float ff;

        public PidGains(float p, float i, float d, float ff)
        {
            this.p = p;
            this.i = i;
            this.d = d;
            this.ff = ff;
        }
    }

    /// <summary>Physical description of one collision the drone took part in.</summary>
    public readonly struct DroneImpact
    {
        /// <summary>Impact speed along the contact normal, in m/s.</summary>
        public readonly float speed;
        public readonly Vector3 point;
        public readonly Vector3 normal;
        public readonly Collider other;

        public DroneImpact(float speed, Vector3 point, Vector3 normal, Collider other)
        {
            this.speed = speed;
            this.point = point;
            this.normal = normal;
            this.other = other;
        }
    }
}
