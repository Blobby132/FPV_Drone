using FPVSim.Flight;
using FPVSim.Settings;
using UnityEngine;

namespace FPVSim.Feedback
{
    /// <summary>
    /// Turns drone impacts into controller rumble: light bumps give a short soft buzz, hard hits a strong long one,
    /// a crash a heavy thud. Toggle and strength come from <see cref="PilotSettings"/>.
    /// </summary>
    [DisallowMultipleComponent]
    public sealed class RumbleFeedback : MonoBehaviour
    {
        [Tooltip("Impacts slower than this (m/s) don't rumble.")]
        [SerializeField] private float minImpactSpeed = 1.5f;

        [Tooltip("Impacts at or above this speed (m/s) rumble at full strength.")]
        [SerializeField] private float maxImpactSpeed = 14f;

        private DroneController drone;
        private PilotSettings settings;

        /// <summary>Where effects are sent. Replace to target other devices or add adaptive-trigger effects.</summary>
        public IHapticsOutput Output { get; set; } = new GamepadHaptics();

        public void Initialize(DroneController target, PilotSettings pilotSettings)
        {
            Unsubscribe();
            drone = target;
            settings = pilotSettings;
            if (drone != null)
            {
                drone.Impacted += OnImpacted;
                drone.Crashed += OnCrashed;
            }
        }

        public void StopAll()
        {
            Output?.Stop();
        }

        private bool Allowed => isActiveAndEnabled && settings != null && settings.rumbleEnabled && Output != null;

        private void OnImpacted(DroneImpact impact)
        {
            if (!Allowed)
            {
                return;
            }

            float k = Mathf.InverseLerp(minImpactSpeed, maxImpactSpeed, impact.speed);
            if (k <= 0f)
            {
                return;
            }

            float strength = settings.rumbleStrength;
            Output.Pulse(k * strength, k * k * strength, Mathf.Lerp(0.08f, 0.4f, k));
        }

        private void OnCrashed()
        {
            if (Allowed)
            {
                Output.Pulse(settings.rumbleStrength, settings.rumbleStrength * 0.7f, 0.6f);
            }
        }

        private void Update()
        {
            Output?.Tick(Time.unscaledTime);
        }

        private void OnDisable()
        {
            StopAll();
        }

        private void OnApplicationFocus(bool hasFocus)
        {
            if (!hasFocus)
            {
                StopAll();
            }
        }

        private void OnDestroy()
        {
            Unsubscribe();
            StopAll();
        }

        private void Unsubscribe()
        {
            if (drone != null)
            {
                drone.Impacted -= OnImpacted;
                drone.Crashed -= OnCrashed;
            }
        }
    }
}
