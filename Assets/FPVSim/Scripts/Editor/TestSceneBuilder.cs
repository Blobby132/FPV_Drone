using System.Linq;
using FPVSim.Cameras;
using FPVSim.Controls;
using FPVSim.Core;
using FPVSim.Feedback;
using FPVSim.Flight;
using FPVSim.Settings;
using FPVSim.UserInterface;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering.Universal;
using UnityEngine.SceneManagement;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// "FPV Sim > Build Test Scene": configures the project, generates materials / meshes / the drone prefab,
    /// builds the test scene and wires every component. Re-running it regenerates everything under
    /// Assets/FPVSim/Generated (your settings assets under Assets/FPVSim/Settings are kept).
    /// </summary>
    public static class TestSceneBuilder
    {
        [MenuItem("FPV Sim/Build Test Scene", priority = 0)]
        public static void BuildFromMenu()
        {
            if (EditorApplication.isPlayingOrWillChangePlaymode)
            {
                EditorUtility.DisplayDialog("FPV Sim", "Exit Play mode before building the test scene.", "OK");
                return;
            }

            if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
            {
                return;
            }

            if (AssetDatabase.LoadAssetAtPath<SceneAsset>(EditorPaths.TestScene) != null &&
                !EditorUtility.DisplayDialog("FPV Sim",
                    "Rebuild the test scene? Everything under " + EditorPaths.Generated +
                    " is regenerated. Your tuning assets in " + EditorPaths.Settings + " are kept.",
                    "Rebuild", "Cancel"))
            {
                return;
            }

            try
            {
                Build();
            }
            finally
            {
                EditorUtility.ClearProgressBar();
            }
        }

        public static void Build()
        {
            Progress("Configuring project", 0.05f);
            ProjectSetup.EnsureProjectConfigured();

            Progress("Settings assets", 0.1f);
            var tuning = AssetUtility.LoadOrCreate<DroneTuning>(EditorPaths.DroneTuning);
            var pilot = AssetUtility.LoadOrCreate<PilotSettings>(EditorPaths.PilotSettings);

            Progress("Drone prefab", 0.2f);
            GameObject dronePrefab = DroneBuilder.BuildPrefab(tuning);

            Progress("Scene", 0.35f);
            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            Progress("Lighting and sky", 0.4f);
            LightingBuilder.Build();

            Progress("Environment (terrain, town, trees, rings)", 0.5f);
            EnvironmentBuilder.Build();

            Progress("Gameplay objects", 0.8f);
            var spawn = new GameObject("SpawnPoint").AddComponent<SpawnPoint>();
            spawn.transform.SetPositionAndRotation(new Vector3(0f, 0.1f, 0f), Quaternion.identity);

            var drone = (GameObject)PrefabUtility.InstantiatePrefab(dronePrefab, scene);
            drone.transform.SetPositionAndRotation(spawn.Pose.position, spawn.Pose.rotation);

            var droneController = drone.GetComponent<DroneController>();
            CameraRig cameraRig = BuildCameraRig(spawn);
            var osd = new GameObject("OSD").AddComponent<OsdView>();
            var pauseMenu = new GameObject("PauseMenu").AddComponent<PauseMenu>();
            BuildSession(tuning, pilot, droneController, spawn, cameraRig, osd, pauseMenu);

            Progress("Saving", 0.95f);
            AssetUtility.EnsureFolder(EditorPaths.Scenes);
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, EditorPaths.TestScene);
            AddSceneToBuildSettings(EditorPaths.TestScene);
            AssetDatabase.SaveAssets();

            Selection.activeGameObject = drone;
            Debug.Log("[FPV Sim] Test scene built: " + EditorPaths.TestScene + ". Press Play to fly.");
        }

        /// <summary>The single scene camera, driven by CameraRig (FPV / chase).</summary>
        private static CameraRig BuildCameraRig(SpawnPoint spawn)
        {
            var cameraObject = new GameObject("CameraRig");
            cameraObject.tag = "MainCamera";
            var camera = cameraObject.AddComponent<Camera>();
            camera.nearClipPlane = 0.02f;
            camera.farClipPlane = 3000f;
            camera.fieldOfView = 88f;
            cameraObject.AddComponent<AudioListener>();

            UniversalAdditionalCameraData urpData = camera.GetUniversalAdditionalCameraData();
            if (urpData != null)
            {
                urpData.renderPostProcessing = true;
                urpData.antialiasing = AntialiasingMode.None; // MSAA 4x is set on the URP asset
            }

            cameraObject.transform.SetPositionAndRotation(spawn.transform.position + new Vector3(0f, 1.2f, -3f), Quaternion.identity);
            return cameraObject.AddComponent<CameraRig>();
        }

        private static void BuildSession(DroneTuning tuning, PilotSettings pilot, DroneController drone, SpawnPoint spawn,
            CameraRig cameraRig, OsdView osd, PauseMenu pauseMenu)
        {
            var sessionObject = new GameObject("GameSession");
            var settings = sessionObject.AddComponent<SettingsManager>();
            settings.SetDefaults(tuning, pilot);
            var input = sessionObject.AddComponent<PilotInputReader>();
            var freeFly = sessionObject.AddComponent<FreeFlyMode>();
            var rumble = sessionObject.AddComponent<RumbleFeedback>();
            var session = sessionObject.AddComponent<GameSession>();

            GameSession.SceneReferences refs = session.References;
            refs.settings = settings;
            refs.input = input;
            refs.drone = drone;
            refs.spawnPoint = spawn;
            refs.gameMode = freeFly;
            refs.cameraRig = cameraRig;
            refs.osd = osd;
            refs.pauseMenu = pauseMenu;
            refs.rumble = rumble;
            EditorUtility.SetDirty(session);
        }

        private static void AddSceneToBuildSettings(string path)
        {
            EditorBuildSettingsScene[] scenes = EditorBuildSettings.scenes;
            if (scenes.Any(s => s.path == path))
            {
                return;
            }

            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(path, true) }.Concat(scenes).ToArray();
        }

        private static void Progress(string step, float progress)
        {
            EditorUtility.DisplayProgressBar("FPV Sim: Building test scene", step, progress);
        }
    }
}
