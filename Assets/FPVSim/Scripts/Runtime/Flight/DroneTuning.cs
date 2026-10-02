using UnityEngine;

namespace FPVSim.Flight
{
    /// <summary>
    /// Everything that defines how the quad flies: airframe, motors, aerodynamics, rates, PIDs, throttle curve.
    ///
    /// At runtime the game works on a copy of this asset (see <c>SettingsManager</c>), so the asset in the
    /// project keeps your defaults while the in-game menu, the Inspector (select the drone during Play mode and
    /// open its runtime tuning) and the saved JSON file edit the copy. Values are read every physics step, so
    /// changes apply immediately.
    /// </summary>
    [CreateAssetMenu(fileName = "DroneTuning", menuName = "FPV Sim/Drone Tuning")]
    public sealed class DroneTuning : ScriptableObject
    {
        [Tooltip("Bumped when the meaning of saved values changes.")]
        public int version = 1;

        [Header("Airframe (defaults: typical 5\" freestyle quad)")]
        [Tooltip("All-up weight including battery, in kg.")]
        [Range(0.15f, 2f)] public float massKg = 0.65f;

        [Tooltip("Distance from the center of the frame to each motor, in meters (X frame).")]
        [Range(0.04f, 0.3f)] public float armLength = 0.11f;

        [Tooltip("Moment of inertia about the roll (forward) axis, kg*m^2.")]
        [Range(0.0005f, 0.05f)] public float inertiaRoll = 0.0030f;

        [Tooltip("Moment of inertia about the pitch (right) axis, kg*m^2.")]
        [Range(0.0005f, 0.05f)] public float inertiaPitch = 0.0035f;

        [Tooltip("Moment of inertia about the yaw (up) axis, kg*m^2.")]
        [Range(0.0005f, 0.05f)] public float inertiaYaw = 0.0060f;

        [Header("Motors / props")]
        [Tooltip("Total maximum thrust divided by weight.")]
        [Range(1.5f, 15f)] public float thrustToWeight = 5f;

        [Tooltip("Motor output while armed at zero throttle (fraction of max thrust).")]
        [Range(0f, 0.2f)] public float motorIdle = 0.045f;

        [Tooltip("First-order time constant when a motor speeds up, in seconds.")]
        [Range(0.002f, 0.2f)] public float motorSpinUpTime = 0.020f;

        [Tooltip("First-order time constant when a motor slows down, in seconds. Usually slower than spin-up.")]
        [Range(0.002f, 0.3f)] public float motorSpinDownTime = 0.035f;

        [Tooltip("Propeller drag torque per newton of thrust (m). Determines yaw authority.")]
        [Range(0.001f, 0.1f)] public float yawTorqueCoefficient = 0.03f;

        [Header("Aerodynamics")]
        [Tooltip("Quadratic drag per body axis in N/(m/s)^2: x = sideways, y = vertical (through the props), z = forward.")]
        public Vector3 quadraticDrag = new Vector3(0.020f, 0.045f, 0.016f);

        [Tooltip("Linear 'rotor drag' in the body's horizontal plane, N/(m/s). Makes the quad bleed speed like a real one.")]
        [Range(0f, 1f)] public float rotorDrag = 0.06f;

        [Tooltip("Multiplier applied to all linear drag (menu: Drag).")]
        [Range(0f, 3f)] public float dragMultiplier = 1f;

        [Tooltip("Angular damping torque per rad/s (N*m*s). Mostly noticeable when disarmed / tumbling.")]
        [Range(0f, 0.05f)] public float angularDrag = 0.0015f;

        [Header("Rates (Betaflight rates)")]
        public AxisRates rollRates = new AxisRates(1.0f, 0.70f, 0.0f);
        public AxisRates pitchRates = new AxisRates(1.0f, 0.70f, 0.0f);
        public AxisRates yawRates = new AxisRates(1.0f, 0.70f, 0.0f);

        [Header("Rate PID (Betaflight-like numbers, scaled internally)")]
        public PidGains rollPid = new PidGains(45f, 80f, 30f, 120f);
        public PidGains pitchPid = new PidGains(47f, 84f, 34f, 125f);
        public PidGains yawPid = new PidGains(45f, 80f, 0f, 120f);

        [Tooltip("Low-pass cutoff for the D term, in Hz.")]
        [Range(10f, 250f)] public float dTermCutoffHz = 90f;

        [Tooltip("Low-pass cutoff for the feedforward term, in Hz. Smooths stick steps from frame-rate input sampling.")]
        [Range(5f, 100f)] public float feedforwardCutoffHz = 25f;

        [Tooltip("Maximum I-term contribution, in rad/s^2 of angular acceleration.")]
        [Range(0f, 1000f)] public float iTermLimit = 250f;

        [Tooltip("Airmode keeps full attitude authority at zero throttle (standard for freestyle).")]
        public bool airmode = true;

        [Header("Angle mode")]
        [Tooltip("Maximum tilt angle per axis at full stick, in degrees.")]
        [Range(10f, 80f)] public float angleMaxDeg = 45f;

        [Tooltip("How hard the quad pulls towards the target angle (1/s). Higher = snappier levelling.")]
        [Range(1f, 20f)] public float angleStrength = 7f;

        [Tooltip("Rotation-rate limit used while levelling, in deg/s.")]
        [Range(90f, 1000f)] public float angleMaxLevelRate = 450f;

        [Tooltip("Angle mode only: boosts thrust when tilted so stick-centered still roughly holds altitude. 0 = off (realistic).")]
        [Range(0f, 1f)] public float angleThrottleCompensation = 1f;

        [Header("Throttle curve")]
        [Tooltip("Use the hover throttle computed from thrust-to-weight as the curve mid point.")]
        public bool autoHoverThrottle = true;

        [Tooltip("Mid point of the throttle curve when Auto Hover Throttle is off (mixer throttle at half stick).")]
        [Range(0.05f, 0.9f)] public float throttleMid = 0.3f;

        [Tooltip("Flattens the throttle curve around the mid point for finer altitude control.")]
        [Range(0f, 1f)] public float throttleExpo = 0.3f;

        [Header("Crashes")]
        [Tooltip("Cut the motors after a hard impact until the drone is reset.")]
        public bool disarmOnCrash = true;

        [Tooltip("Impact speed along the contact normal that counts as a crash, in m/s.")]
        [Range(2f, 40f)] public float crashSpeed = 11f;

        [Header("Simulation")]
        [Tooltip("Physics / flight-controller loop rate. The rate PID needs a high rate to be stable; 500 Hz is a good default.")]
        [Range(200, 1000)] public int physicsRateHz = 500;

        [Header("Battery (cosmetic)")]
        [Range(1, 8)] public int batteryCells = 4;
        [Range(200f, 6000f)] public float batteryCapacityMah = 1300f;
        [Tooltip("Total current at full throttle, in amps.")]
        [Range(5f, 300f)] public float maxCurrentAmps = 110f;
        [Range(0f, 10f)] public float idleCurrentAmps = 1.5f;
        [Tooltip("Internal resistance per cell, in ohms (voltage sag).")]
        [Range(0f, 0.05f)] public float cellInternalResistance = 0.008f;

        /// <summary>Throttle-curve mid point actually in use.</summary>
        public float EffectiveThrottleMid =>
            autoHoverThrottle ? ThrottleCurve.HoverThrottle(thrustToWeight, motorIdle) : throttleMid;

        /// <summary>Mixer throttle that balances gravity when level.</summary>
        public float HoverThrottle => ThrottleCurve.HoverThrottle(thrustToWeight, motorIdle);

        /// <summary>Rigidbody inertia tensor in Unity local axes (x = pitch, y = yaw, z = roll).</summary>
        public Vector3 InertiaTensorLocal => new Vector3(inertiaPitch, inertiaYaw, inertiaRoll);

        public ref PidGains GetPid(FlightAxis axis)
        {
            switch (axis)
            {
                case FlightAxis.Roll: return ref rollPid;
                case FlightAxis.Pitch: return ref pitchPid;
                default: return ref yawPid;
            }
        }

        public ref AxisRates GetRates(FlightAxis axis)
        {
            switch (axis)
            {
                case FlightAxis.Roll: return ref rollRates;
                case FlightAxis.Pitch: return ref pitchRates;
                default: return ref yawRates;
            }
        }

        /// <summary>Copies every serialized value from another tuning asset.</summary>
        public void CopyFrom(DroneTuning other)
        {
            if (other == null || other == this)
            {
                return;
            }

            JsonUtility.FromJsonOverwrite(JsonUtility.ToJson(other), this);
        }

        /// <summary>Clamps values loaded from JSON or typed in the Inspector into safe ranges.</summary>
        public void Sanitize()
        {
            massKg = Mathf.Clamp(massKg, 0.05f, 10f);
            armLength = Mathf.Clamp(armLength, 0.02f, 1f);
            inertiaRoll = Mathf.Max(0.0001f, inertiaRoll);
            inertiaPitch = Mathf.Max(0.0001f, inertiaPitch);
            inertiaYaw = Mathf.Max(0.0001f, inertiaYaw);
            thrustToWeight = Mathf.Clamp(thrustToWeight, 1.05f, 30f);
            motorIdle = Mathf.Clamp(motorIdle, 0f, 0.3f);
            motorSpinUpTime = Mathf.Clamp(motorSpinUpTime, 0.001f, 1f);
            motorSpinDownTime = Mathf.Clamp(motorSpinDownTime, 0.001f, 1f);
            yawTorqueCoefficient = Mathf.Clamp(yawTorqueCoefficient, 0.0005f, 0.5f);
            quadraticDrag = Vector3.Max(quadraticDrag, Vector3.zero);
            rotorDrag = Mathf.Max(0f, rotorDrag);
            dragMultiplier = Mathf.Max(0f, dragMultiplier);
            angularDrag = Mathf.Max(0f, angularDrag);
            SanitizeRates(ref rollRates);
            SanitizeRates(ref pitchRates);
            SanitizeRates(ref yawRates);
            SanitizePid(ref rollPid);
            SanitizePid(ref pitchPid);
            SanitizePid(ref yawPid);
            dTermCutoffHz = Mathf.Clamp(dTermCutoffHz, 1f, 1000f);
            feedforwardCutoffHz = Mathf.Clamp(feedforwardCutoffHz, 1f, 1000f);
            iTermLimit = Mathf.Max(0f, iTermLimit);
            angleMaxDeg = Mathf.Clamp(angleMaxDeg, 5f, 85f);
            angleStrength = Mathf.Clamp(angleStrength, 0.1f, 50f);
            angleMaxLevelRate = Mathf.Clamp(angleMaxLevelRate, 10f, 2000f);
            angleThrottleCompensation = Mathf.Clamp01(angleThrottleCompensation);
            throttleMid = Mathf.Clamp(throttleMid, 0.01f, 0.99f);
            throttleExpo = Mathf.Clamp01(throttleExpo);
            crashSpeed = Mathf.Max(0.5f, crashSpeed);
            physicsRateHz = Mathf.Clamp(physicsRateHz, 100, 2000);
            batteryCells = Mathf.Clamp(batteryCells, 1, 12);
            batteryCapacityMah = Mathf.Max(50f, batteryCapacityMah);
            maxCurrentAmps = Mathf.Max(0f, maxCurrentAmps);
            idleCurrentAmps = Mathf.Max(0f, idleCurrentAmps);
            cellInternalResistance = Mathf.Max(0f, cellInternalResistance);
        }

        private static void SanitizeRates(ref AxisRates rates)
        {
            rates.rcRate = Mathf.Clamp(rates.rcRate, 0.01f, 3f);
            rates.superRate = Mathf.Clamp(rates.superRate, 0f, 0.99f);
            rates.expo = Mathf.Clamp01(rates.expo);
        }

        private static void SanitizePid(ref PidGains gains)
        {
            gains.p = Mathf.Clamp(gains.p, 0f, 500f);
            gains.i = Mathf.Clamp(gains.i, 0f, 500f);
            gains.d = Mathf.Clamp(gains.d, 0f, 500f);
            gains.ff = Mathf.Clamp(gains.ff, 0f, 1000f);
        }

        private void OnValidate()
        {
            Sanitize();
        }
    }
}
