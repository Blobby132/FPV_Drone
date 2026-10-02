using FPVSim.Controls;
using FPVSim.Flight;
using UnityEngine;

namespace FPVSim.Settings
{
    /// <summary>Which camera is active.</summary>
    public enum CameraViewMode
    {
        Fpv = 0,
        Chase = 1,
    }

    /// <summary>
    /// Player preferences: stick setup, throttle handling, camera, feedback, OSD. Like <see cref="DroneTuning"/>,
    /// the game edits a runtime copy and saves it to JSON.
    /// </summary>
    [CreateAssetMenu(fileName = "PilotSettings", menuName = "FPV Sim/Pilot Settings")]
    public sealed class PilotSettings : ScriptableObject
    {
        [Tooltip("Bumped when the meaning of saved values changes.")]
        public int version = 1;

        [Header("Sticks")]
        [Tooltip("Mode 2: left = throttle/yaw, right = pitch/roll. Mode 1: left = pitch/yaw, right = throttle/roll.")]
        public StickMode stickMode = StickMode.Mode2;

        public StickSettings leftStick = new StickSettings(0.05f, 0.10f);
        public StickSettings rightStick = new StickSettings(0.05f, 0.20f);

        [Header("Throttle")]
        [Tooltip("Hover-centered: stick center = hover. Latched: stick ramps a held throttle value.")]
        public ThrottleMode throttleMode = ThrottleMode.HoverCentered;

        [Tooltip("Latched mode: throttle change per second at full stick deflection (1 = 0 to 100% in one second).")]
        [Range(0.1f, 3f)] public float latchedThrottleRampSpeed = 0.8f;

        [Header("Input smoothing (time constant in seconds, 0 = off)")]
        [Range(0f, 0.25f)] public float smoothingAngle = 0.03f;
        [Range(0f, 0.25f)] public float smoothingAcro = 0f;

        [Header("Flight")]
        public FlightMode startFlightMode = FlightMode.Angle;

        [Header("FPV camera")]
        [Tooltip("Camera uptilt in degrees (positive = looking up relative to the frame).")]
        [Range(-10f, 70f)] public float cameraUptilt = 25f;

        [Tooltip("Horizontal field of view in degrees, like an FPV camera lens.")]
        [Range(60f, 170f)] public float cameraFov = 120f;

        [Tooltip("Degrees per D-pad press.")]
        [Range(1f, 15f)] public float cameraTiltStep = 5f;

        public CameraViewMode startView = CameraViewMode.Fpv;

        [Header("Chase camera (debug)")]
        [Range(1f, 15f)] public float chaseDistance = 3.5f;
        [Range(-2f, 6f)] public float chaseHeight = 1.0f;
        [Range(40f, 140f)] public float chaseFov = 90f;

        [Header("Feedback")]
        public bool rumbleEnabled = true;
        [Range(0f, 1f)] public float rumbleStrength = 0.8f;

        [Header("OSD")]
        public bool showOsd = true;
        [Tooltip("Show speed in mph and altitude in feet.")]
        public bool imperialUnits;

        public void CopyFrom(PilotSettings other)
        {
            if (other == null || other == this)
            {
                return;
            }

            JsonUtility.FromJsonOverwrite(JsonUtility.ToJson(other), this);
        }

        public void Sanitize()
        {
            leftStick = SanitizeStick(leftStick);
            rightStick = SanitizeStick(rightStick);
            if (stickMode != StickMode.Mode1 && stickMode != StickMode.Mode2)
            {
                stickMode = StickMode.Mode2;
            }

            if (throttleMode != ThrottleMode.HoverCentered && throttleMode != ThrottleMode.Latched)
            {
                throttleMode = ThrottleMode.HoverCentered;
            }

            if (startFlightMode != FlightMode.Angle && startFlightMode != FlightMode.Acro)
            {
                startFlightMode = FlightMode.Angle;
            }

            if (startView != CameraViewMode.Fpv && startView != CameraViewMode.Chase)
            {
                startView = CameraViewMode.Fpv;
            }

            latchedThrottleRampSpeed = Mathf.Clamp(latchedThrottleRampSpeed, 0.05f, 10f);
            smoothingAngle = Mathf.Clamp(smoothingAngle, 0f, 1f);
            smoothingAcro = Mathf.Clamp(smoothingAcro, 0f, 1f);
            cameraUptilt = Mathf.Clamp(cameraUptilt, -30f, 80f);
            cameraFov = Mathf.Clamp(cameraFov, 30f, 170f);
            cameraTiltStep = Mathf.Clamp(cameraTiltStep, 0.5f, 30f);
            chaseDistance = Mathf.Clamp(chaseDistance, 0.5f, 50f);
            chaseHeight = Mathf.Clamp(chaseHeight, -10f, 20f);
            chaseFov = Mathf.Clamp(chaseFov, 20f, 170f);
            rumbleStrength = Mathf.Clamp01(rumbleStrength);
        }

        private static StickSettings SanitizeStick(StickSettings stick)
        {
            stick.deadzone = Mathf.Clamp(stick.deadzone, 0f, 0.5f);
            stick.expo = Mathf.Clamp01(stick.expo);
            return stick;
        }

        private void OnValidate()
        {
            Sanitize();
        }
    }
}
