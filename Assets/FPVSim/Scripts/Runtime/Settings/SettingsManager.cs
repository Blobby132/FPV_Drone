using System;
using System.IO;
using FPVSim.Flight;
using UnityEngine;
using UnityEngine.InputSystem;

namespace FPVSim.Settings
{
    /// <summary>
    /// Owns the runtime copies of <see cref="DroneTuning"/> and <see cref="PilotSettings"/> and saves / loads them
    /// (plus gamepad binding overrides) as JSON under <c>Application.persistentDataPath/FPVSim/</c>:
    /// drone_tuning.json, pilot_settings.json and input_bindings.json.
    ///
    /// The project assets stay untouched (they are your defaults); everything at runtime reads and writes the
    /// copies, so loading saved settings in the editor never overwrites the assets.
    /// </summary>
    [DefaultExecutionOrder(-200)]
    [DisallowMultipleComponent]
    public sealed class SettingsManager : MonoBehaviour
    {
        private const string FolderName = "FPVSim";
        private const string TuningFile = "drone_tuning.json";
        private const string PilotFile = "pilot_settings.json";
        private const string BindingsFile = "input_bindings.json";

        [Tooltip("Default drone tuning (project asset). Copied at startup.")]
        [SerializeField] private DroneTuning defaultTuning;

        [Tooltip("Default pilot settings (project asset). Copied at startup.")]
        [SerializeField] private PilotSettings defaultPilotSettings;

        [Tooltip("Load the saved JSON settings at startup. Turn off to always start from the default assets.")]
        [SerializeField] private bool loadSavedSettings = true;

        [Header("Runtime copies (Play mode only: double-click to tweak live)")]
        [Tooltip("The drone tuning the simulation is using right now. Created at startup; not saved in the scene.")]
        [SerializeField] private DroneTuning tuning;

        [Tooltip("The pilot settings the game is using right now. Created at startup; not saved in the scene.")]
        [SerializeField] private PilotSettings pilot;

        private InputActionAsset boundActions;
        private bool initialized;

        /// <summary>Raised after settings were loaded, reverted or reset (UI should refresh).</summary>
        public event Action SettingsReplaced;

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

        /// <summary>True when something changed since the last save or load.</summary>
        public bool IsDirty { get; private set; }

        public string SettingsFolder => Path.Combine(Application.persistentDataPath, FolderName);

        /// <summary>Assign the default assets (used by the editor scene builder).</summary>
        public void SetDefaults(DroneTuning droneDefaults, PilotSettings pilotDefaults)
        {
            defaultTuning = droneDefaults;
            defaultPilotSettings = pilotDefaults;
        }

        /// <summary>Connects the input actions so binding overrides are loaded and saved with the settings.</summary>
        public void BindInput(InputActionAsset actions)
        {
            boundActions = actions;
            if (loadSavedSettings && actions != null)
            {
                LoadBindings();
            }
        }

        public void MarkDirty()
        {
            IsDirty = true;
        }

        /// <returns>True if everything was written.</returns>
        public bool Save()
        {
            EnsureInitialized();
            try
            {
                Directory.CreateDirectory(SettingsFolder);
                File.WriteAllText(PathOf(TuningFile), JsonUtility.ToJson(tuning, true));
                File.WriteAllText(PathOf(PilotFile), JsonUtility.ToJson(pilot, true));
                if (boundActions != null)
                {
                    File.WriteAllText(PathOf(BindingsFile), boundActions.SaveBindingOverridesAsJson());
                }

                IsDirty = false;
                Debug.Log("[FPV Sim] Settings saved to " + SettingsFolder);
                return true;
            }
            catch (Exception exception)
            {
                Debug.LogWarning("[FPV Sim] Could not save settings: " + exception.Message);
                return false;
            }
        }

        /// <summary>Re-reads the saved JSON files (missing files keep current values).</summary>
        public void RevertToSaved()
        {
            EnsureInitialized();
            LoadInto(PathOf(TuningFile), tuning);
            LoadInto(PathOf(PilotFile), pilot);
            tuning.Sanitize();
            pilot.Sanitize();
            LoadBindings();
            IsDirty = false;
            SettingsReplaced?.Invoke();
        }

        /// <summary>Restores the default assets' values and default bindings (not saved until Save()).</summary>
        public void ResetToDefaults()
        {
            EnsureInitialized();
            CopyDefaults();
            if (boundActions != null)
            {
                boundActions.RemoveAllBindingOverrides();
            }

            IsDirty = true;
            SettingsReplaced?.Invoke();
        }

        private void Awake()
        {
            EnsureInitialized();
        }

        private void OnApplicationQuit()
        {
            if (IsDirty)
            {
                Save();
            }
        }

        private void OnDestroy()
        {
            if (!initialized)
            {
                return;
            }

            if (tuning != null)
            {
                Destroy(tuning);
            }

            if (pilot != null)
            {
                Destroy(pilot);
            }
        }

#if UNITY_EDITOR
        /// <summary>Editor helper: keep the values you tuned in Play mode by copying them into the default assets.</summary>
        [ContextMenu("Write runtime values to default assets")]
        private void WriteRuntimeValuesToDefaults()
        {
            if (!Application.isPlaying || !initialized)
            {
                Debug.LogWarning("[FPV Sim] Enter Play mode first; this copies the live runtime values.");
                return;
            }

            if (defaultTuning != null)
            {
                defaultTuning.CopyFrom(tuning);
                UnityEditor.EditorUtility.SetDirty(defaultTuning);
            }

            if (defaultPilotSettings != null)
            {
                defaultPilotSettings.CopyFrom(pilot);
                UnityEditor.EditorUtility.SetDirty(defaultPilotSettings);
            }

            UnityEditor.AssetDatabase.SaveAssets();
            Debug.Log("[FPV Sim] Runtime settings written to the default assets.");
        }
#endif

        private void EnsureInitialized()
        {
            if (initialized && tuning != null && pilot != null)
            {
                return;
            }

            // Always start from fresh copies (ignore anything left in the serialized fields).
            initialized = true;

            tuning = ScriptableObject.CreateInstance<DroneTuning>();
            tuning.name = "DroneTuning (runtime)";
            pilot = ScriptableObject.CreateInstance<PilotSettings>();
            pilot.name = "PilotSettings (runtime)";
            CopyDefaults();

            if (loadSavedSettings)
            {
                LoadInto(PathOf(TuningFile), tuning);
                LoadInto(PathOf(PilotFile), pilot);
            }

            tuning.Sanitize();
            pilot.Sanitize();
        }

        private void CopyDefaults()
        {
            if (defaultTuning != null)
            {
                tuning.CopyFrom(defaultTuning);
            }
            else
            {
                CopyFromFreshInstance(tuning);
            }

            if (defaultPilotSettings != null)
            {
                pilot.CopyFrom(defaultPilotSettings);
            }
            else
            {
                CopyFromFreshInstance(pilot);
            }

            tuning.Sanitize();
            pilot.Sanitize();
        }

        /// <summary>Resets an object to its code defaults by copying a freshly created instance.</summary>
        private static void CopyFromFreshInstance<T>(T target) where T : ScriptableObject
        {
            T fresh = ScriptableObject.CreateInstance<T>();
            JsonUtility.FromJsonOverwrite(JsonUtility.ToJson(fresh), target);
            Destroy(fresh);
        }

        private void LoadBindings()
        {
            if (boundActions == null)
            {
                return;
            }

            string path = PathOf(BindingsFile);
            if (!File.Exists(path))
            {
                return;
            }

            try
            {
                boundActions.LoadBindingOverridesFromJson(File.ReadAllText(path));
            }
            catch (Exception exception)
            {
                Debug.LogWarning("[FPV Sim] Ignoring unreadable binding file " + path + ": " + exception.Message);
            }
        }

        private static void LoadInto(string path, ScriptableObject target)
        {
            if (!File.Exists(path))
            {
                return;
            }

            try
            {
                JsonUtility.FromJsonOverwrite(File.ReadAllText(path), target);
            }
            catch (Exception exception)
            {
                Debug.LogWarning("[FPV Sim] Ignoring unreadable settings file " + path + ": " + exception.Message);
            }
        }

        private string PathOf(string fileName)
        {
            return Path.Combine(SettingsFolder, fileName);
        }
    }
}
