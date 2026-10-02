using System.Linq;
using FPVSim.Controls;
using FPVSim.Core;
using FPVSim.Flight;
using FPVSim.Settings;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
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

            BuildLighting();

            Progress("Environment", 0.5f);
            BuildGround();

            Progress("Gameplay objects", 0.8f);
            var spawn = new GameObject("SpawnPoint").AddComponent<SpawnPoint>();
            spawn.transform.SetPositionAndRotation(new Vector3(0f, 0.1f, 0f), Quaternion.identity);

            var drone = (GameObject)PrefabUtility.InstantiatePrefab(dronePrefab, scene);
            drone.transform.SetPositionAndRotation(spawn.Pose.position, spawn.Pose.rotation);

            BuildCamera(drone.GetComponent<DroneController>());
            BuildSession(tuning, pilot, drone.GetComponent<DroneController>(), spawn);

            Progress("Saving", 0.95f);
            AssetUtility.EnsureFolder(EditorPaths.Scenes);
            EditorSceneManager.MarkSceneDirty(scene);
            EditorSceneManager.SaveScene(scene, EditorPaths.TestScene);
            AddSceneToBuildSettings(EditorPaths.TestScene);
            AssetDatabase.SaveAssets();

            Selection.activeGameObject = drone;
            Debug.Log("[FPV Sim] Test scene built: " + EditorPaths.TestScene + ". Press Play to fly.");
        }

        private static void BuildLighting()
        {
            var sun = new GameObject("Sun").AddComponent<Light>();
            sun.type = LightType.Directional;
            sun.intensity = 1.3f;
            sun.color = new Color(1f, 0.96f, 0.88f);
            sun.shadows = LightShadows.Soft;
            sun.transform.rotation = Quaternion.Euler(48f, -35f, 0f);
            RenderSettings.sun = sun;

            RenderSettings.ambientMode = AmbientMode.Trilight;
            RenderSettings.ambientSkyColor = new Color(0.55f, 0.65f, 0.8f);
            RenderSettings.ambientEquatorColor = new Color(0.45f, 0.5f, 0.52f);
            RenderSettings.ambientGroundColor = new Color(0.22f, 0.22f, 0.2f);
        }

        private static void BuildGround()
        {
            Material grass = MaterialLibrary.Lit("Ground_Placeholder", new Color(0.32f, 0.5f, 0.24f), 0.1f);
            GameObject ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
            ground.name = "Ground";
            ground.transform.localScale = new Vector3(200f, 1f, 200f); // Unity's plane is 10 m -> 2 km
            ground.GetComponent<MeshRenderer>().sharedMaterial = grass;
            GameObjectUtility.SetStaticEditorFlags(ground, StaticEditorFlags.BatchingStatic | StaticEditorFlags.OccluderStatic | StaticEditorFlags.OccludeeStatic);
        }

        /// <summary>Temporary FPV camera parented to the drone's mount (replaced by the camera rig later).</summary>
        private static void BuildCamera(DroneController drone)
        {
            var cameraObject = new GameObject("Main Camera");
            cameraObject.tag = "MainCamera";
            var camera = cameraObject.AddComponent<Camera>();
            camera.nearClipPlane = 0.03f;
            camera.farClipPlane = 3000f;
            camera.fieldOfView = 90f;
            cameraObject.AddComponent<AudioListener>();
            cameraObject.transform.SetParent(drone.CameraMount, false);
            cameraObject.transform.localRotation = Quaternion.Euler(-25f, 0f, 0f);
        }

        private static void BuildSession(DroneTuning tuning, PilotSettings pilot, DroneController drone, SpawnPoint spawn)
        {
            var sessionObject = new GameObject("GameSession");
            var settings = sessionObject.AddComponent<SettingsManager>();
            settings.SetDefaults(tuning, pilot);
            var input = sessionObject.AddComponent<PilotInputReader>();
            var freeFly = sessionObject.AddComponent<FreeFlyMode>();
            var session = sessionObject.AddComponent<GameSession>();

            GameSession.SceneReferences refs = session.References;
            refs.settings = settings;
            refs.input = input;
            refs.drone = drone;
            refs.spawnPoint = spawn;
            refs.gameMode = freeFly;
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
