using System;
using FPVSim.Controls;
using FPVSim.Flight;
using FPVSim.Settings;
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

            [Tooltip("A component implementing IGameMode. Defaults to FreeFlyMode.")]
            public MonoBehaviour gameMode;
        }

        [SerializeField] private SceneReferences references = new SceneReferences();

        private PilotCommandSource commandSource;
        private IGameMode gameMode;
        private int appliedPhysicsRate;

        /// <summary>Scene references (assigned by the editor scene builder, or found at runtime).</summary>
        public SceneReferences References => references;

        public SettingsManager Settings => references.settings;
        public PilotInputReader PilotInput => references.input;
        public DroneController Drone => references.drone;
        public IGameMode GameMode => gameMode;
        public PilotCommandSource CommandSource => commandSource;

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

            DroneTuning tuning = references.settings.Tuning;
            PilotSettings pilot = references.settings.Pilot;

            Time.maximumDeltaTime = 0.1f; // avoid a physics "spiral of death" after hitches
            ApplyPhysicsRate(tuning);

            commandSource = new PilotCommandSource(references.input, pilot);
            references.drone.Initialize(tuning, commandSource, pilot.startFlightMode);

            references.input.RespawnPressed += OnRespawnPressed;
            references.input.FlightModeTogglePressed += OnFlightModeTogglePressed;
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
            gameMode?.Tick(Time.deltaTime);
        }

        private void OnDestroy()
        {
            if (references.input != null)
            {
                references.input.RespawnPressed -= OnRespawnPressed;
                references.input.FlightModeTogglePressed -= OnFlightModeTogglePressed;
            }

            gameMode?.End();
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
        }
    }
}
