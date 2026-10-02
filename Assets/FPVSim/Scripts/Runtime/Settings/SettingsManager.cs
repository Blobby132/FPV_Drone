using FPVSim.Flight;
using UnityEngine;

namespace FPVSim.Settings
{
    /// <summary>
    /// Owns the runtime copies of <see cref="DroneTuning"/> and <see cref="PilotSettings"/>. The project assets
    /// stay untouched (they are your defaults); everything at runtime reads and writes the copies.
    /// </summary>
    [DefaultExecutionOrder(-200)]
    [DisallowMultipleComponent]
    public sealed class SettingsManager : MonoBehaviour
    {
        [Tooltip("Default drone tuning (project asset). Copied at startup.")]
        [SerializeField] private DroneTuning defaultTuning;

        [Tooltip("Default pilot settings (project asset). Copied at startup.")]
        [SerializeField] private PilotSettings defaultPilotSettings;

        private DroneTuning tuning;
        private PilotSettings pilot;

        /// <summary>Runtime drone tuning. Select it in the Inspector during Play mode to tweak live.</summary>
        public DroneTuning Tuning
        {
            get
            {
                EnsureInitialized();
                return tuning;
            }
        }

        /// <summary>Runtime pilot settings.</summary>
        public PilotSettings Pilot
        {
            get
            {
                EnsureInitialized();
                return pilot;
            }
        }

        public DroneTuning DefaultTuning => defaultTuning;
        public PilotSettings DefaultPilotSettings => defaultPilotSettings;

        /// <summary>Assign the default assets (used by the editor scene builder).</summary>
        public void SetDefaults(DroneTuning droneDefaults, PilotSettings pilotDefaults)
        {
            defaultTuning = droneDefaults;
            defaultPilotSettings = pilotDefaults;
        }

        private void Awake()
        {
            EnsureInitialized();
        }

        private void EnsureInitialized()
        {
            if (tuning != null && pilot != null)
            {
                return;
            }

            tuning = defaultTuning != null ? Instantiate(defaultTuning) : ScriptableObject.CreateInstance<DroneTuning>();
            tuning.name = "DroneTuning (runtime)";
            tuning.Sanitize();

            pilot = defaultPilotSettings != null
                ? Instantiate(defaultPilotSettings)
                : ScriptableObject.CreateInstance<PilotSettings>();
            pilot.name = "PilotSettings (runtime)";
            pilot.Sanitize();
        }

        private void OnDestroy()
        {
            if (tuning != null)
            {
                Destroy(tuning);
            }

            if (pilot != null)
            {
                Destroy(pilot);
            }
        }
    }
}
