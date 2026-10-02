using System;
using FPVSim.Cameras;
using FPVSim.Controls;
using FPVSim.Feedback;
using FPVSim.Flight;
using FPVSim.Settings;
using FPVSim.UserInterface;
using UnityEngine;

namespace FPVSim.Core
{
    /// <summary>
    /// Composition root of a flying scene. Creates the runtime services, wires input to the drone and routes
    /// utility buttons. Holds the only serialized cross-references; every other component is initialized from
    /// here, so scenes stay easy to assemble (the editor builder fills <see cref="References"/>).
    /// </summary>
    [DefaultExecutionOrder(-50)]
    [DisallowMultipleComponent]
    public sealed class GameSession : MonoBehaviour
    {
        [Serializable]
        public sealed class SceneReferences
        {
            public SettingsManager settings;
            public PilotInputReader input;
            public DroneController drone;
            public SpawnPoint spawnPoint;
            public CameraRig cameraRig;
            public OsdView osd;
            public PauseMenu pauseMenu;
            public RumbleFeedback rumble;

            [Tooltip("A component implementing IGameMode. Defaults to FreeFlyMode.")]
            public MonoBehaviour gameMode;
        }

        [SerializeField] private SceneReferences references = new SceneReferences();

        private PilotCommandSource commandSource;
        private IGameMode gameMode;
        private int appliedPhysicsRate;
        private bool paused;

        /// <summary>Scene references (assigned by the editor scene builder, or found at runtime).</summary>
        public SceneReferences References => references;

        public SettingsManager Settings => references.settings;
        public PilotInputReader PilotInput => references.input;
        public DroneController Drone => references.drone;
        public CameraRig CameraRig => references.cameraRig;
        public OsdView Osd => references.osd;
        public IGameMode GameMode => gameMode;
        public PilotCommandSource CommandSource => commandSource;
        public bool IsPaused => paused;

        public Pose SpawnPose =>
            references.spawnPoint != null
                ? references.spawnPoint.Pose
                : new Pose(new Vector3(0f, 0.2f, 0f), Quaternion.identity);

        private void Awake()
        {
            ResolveReferences();
            if (references.drone == null)
            {
                Debug.LogError("[FPV Sim] GameSession: no DroneController in the scene.", this);
                enabled = false;
                return;
            }

            SettingsManager settings = references.settings;
            DroneTuning tuning = settings.Tuning;
            PilotSettings pilot = settings.Pilot;
            settings.BindInput(references.input.Actions); // loads saved binding overrides

            Time.maximumDeltaTime = 0.1f; // avoid a physics "spiral of death" after hitches
            ApplyPhysicsRate(tuning);

            commandSource = new PilotCommandSource(references.input, pilot);
            references.drone.Initialize(tuning, commandSource, pilot.startFlightMode);
            references.drone.Respawned += OnDroneRespawned;

            if (references.cameraRig != null)
            {
                references.cameraRig.Initialize(references.drone, pilot);
            }
            else
            {
                Debug.LogWarning("[FPV Sim] GameSession: no CameraRig found; the view will not follow the drone.", this);
            }

            if (references.osd != null)
            {
                references.osd.Initialize(references.drone, references.cameraRig, pilot);
            }

            if (references.rumble != null)
            {
                references.rumble.Initialize(references.drone, pilot);
            }

            if (references.pauseMenu != null)
            {
                references.pauseMenu.Initialize(new PauseMenu.Context
                {
                    settings = settings,
                    input = references.input,
                    quit = Quit,
                    notify = ShowToast,
                });
                references.pauseMenu.Closed += OnMenuClosed;
            }

            PilotInputReader input = references.input;
            input.RespawnPressed += OnRespawnPressed;
            input.FlightModeTogglePressed += OnFlightModeTogglePressed;
            input.CameraTogglePressed += OnCameraTogglePressed;
            input.CameraTiltStepPressed += OnCameraTiltStepPressed;
            input.PausePressed += OnPausePressed;
            UpdateResetHint();
        }

        private void Start()
        {
            gameMode = references.gameMode as IGameMode;
            if (gameMode == null)
            {
                gameMode = gameObject.AddComponent<FreeFlyMode>();
            }

            gameMode.Begin(this);
        }

        private void Update()
        {
            ApplyPhysicsRate(references.settings.Tuning);
            if (!paused)
            {
                gameMode?.Tick(Time.deltaTime);
            }
        }

        private void OnDestroy()
        {
            if (references.input != null)
            {
                references.input.RespawnPressed -= OnRespawnPressed;
                references.input.FlightModeTogglePressed -= OnFlightModeTogglePressed;
                references.input.CameraTogglePressed -= OnCameraTogglePressed;
                references.input.CameraTiltStepPressed -= OnCameraTiltStepPressed;
                references.input.PausePressed -= OnPausePressed;
            }

            if (references.drone != null)
            {
                references.drone.Respawned -= OnDroneRespawned;
            }

            if (references.pauseMenu != null)
            {
                references.pauseMenu.Closed -= OnMenuClosed;
            }

            if (paused)
            {
                Time.timeScale = 1f;
            }

            gameMode?.End();
        }

        /// <summary>Pauses or resumes the simulation (time scale 0, flight controls off, rumble stopped).</summary>
        public void SetPaused(bool value)
        {
            paused = value;
            Time.timeScale = value ? 0f : 1f;
            references.input.SetFlightControlsEnabled(!value);
            if (value && references.rumble != null)
            {
                references.rumble.StopAll();
            }
        }

        private void OnPausePressed()
        {
            PauseMenu menu = references.pauseMenu;
            if (menu == null)
            {
                return;
            }

            if (menu.IsOpen)
            {
                menu.Close(); // resumes via OnMenuClosed
                return;
            }

            SetPaused(true);
            menu.Open();
            if (!menu.IsOpen)
            {
                SetPaused(false);
            }
        }

        private void OnMenuClosed()
        {
            SetPaused(false);
            if (references.settings.IsDirty && references.settings.Save())
            {
                ShowToast("SETTINGS SAVED");
            }

            UpdateResetHint();
        }

        private void Quit()
        {
            if (references.settings.IsDirty)
            {
                references.settings.Save();
            }

#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }

        private void ShowToast(string message)
        {
            if (references.osd != null)
            {
                references.osd.ShowToast(message);
            }
        }

        private void UpdateResetHint()
        {
            if (references.osd != null)
            {
                references.osd.SetResetButtonName(
                    ControlNames.ForAction(references.input.FindAction(FpvInputActions.Respawn)));
            }
        }

        /// <summary>Puts the drone back on the spawn point (used by game modes).</summary>
        public void RespawnDroneAtSpawn()
        {
            if (references.drone != null)
            {
                references.drone.Respawn(SpawnPose);
            }
        }

        private void OnRespawnPressed()
        {
            gameMode?.OnRespawnRequested();
        }

        private void OnFlightModeTogglePressed()
        {
            references.drone.ToggleFlightMode();
        }

        private void OnCameraTogglePressed()
        {
            if (references.cameraRig != null)
            {
                references.cameraRig.ToggleView();
            }
        }

        private void OnCameraTiltStepPressed(int direction)
        {
            if (references.cameraRig != null)
            {
                references.cameraRig.StepUptilt(direction);
                references.settings.MarkDirty();
            }
        }

        private void OnDroneRespawned()
        {
            if (references.cameraRig != null)
            {
                references.cameraRig.SnapChase();
            }
        }

        private void ApplyPhysicsRate(DroneTuning tuning)
        {
            int rate = Mathf.Clamp(tuning.physicsRateHz, 100, 2000);
            if (rate == appliedPhysicsRate)
            {
                return;
            }

            Time.fixedDeltaTime = 1f / rate;
            appliedPhysicsRate = rate;
        }

        private void ResolveReferences()
        {
            if (references.settings == null)
            {
                references.settings = FindFirstObjectByType<SettingsManager>();
                if (references.settings == null)
                {
                    references.settings = gameObject.AddComponent<SettingsManager>();
                }
            }

            if (references.input == null)
            {
                references.input = FindFirstObjectByType<PilotInputReader>();
                if (references.input == null)
                {
                    references.input = gameObject.AddComponent<PilotInputReader>();
                }
            }

            if (references.drone == null)
            {
                references.drone = FindFirstObjectByType<DroneController>();
            }

            if (references.spawnPoint == null)
            {
                references.spawnPoint = FindFirstObjectByType<SpawnPoint>();
            }

            if (references.gameMode == null)
            {
                references.gameMode = GetComponent<FreeFlyMode>();
            }

            if (references.cameraRig == null)
            {
                references.cameraRig = FindFirstObjectByType<CameraRig>();
            }

            if (references.osd == null)
            {
                references.osd = FindFirstObjectByType<OsdView>();
            }

            if (references.pauseMenu == null)
            {
                references.pauseMenu = FindFirstObjectByType<PauseMenu>();
            }

            if (references.rumble == null)
            {
                references.rumble = FindFirstObjectByType<RumbleFeedback>();
            }
        }
    }
}
