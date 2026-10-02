using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// Sun, procedural sky, ambient light, distance fog and a light post-processing stack. No baking needed:
    /// ambient uses trilight colors and the sun is a realtime light, so the scene looks right immediately.
    /// </summary>
    internal static class LightingBuilder
    {
        private static readonly Color HorizonColor = new Color(0.71f, 0.79f, 0.87f);

        public static GameObject Build()
        {
            var root = new GameObject("Lighting");

            var sun = new GameObject("Sun").AddComponent<Light>();
            sun.transform.SetParent(root.transform, false);
            sun.type = LightType.Directional;
            sun.intensity = 1.35f;
            sun.color = new Color(1f, 0.95f, 0.86f);
            sun.shadows = LightShadows.Soft;
            sun.shadowStrength = 0.85f;
            sun.transform.rotation = Quaternion.Euler(42f, -38f, 0f);
            RenderSettings.sun = sun;

            Shader skyShader = Shader.Find("Skybox/Procedural");
            if (skyShader != null)
            {
                var sky = new Material(skyShader);
                sky.SetFloat("_SunDisk", 2f);
                sky.SetFloat("_SunSize", 0.035f);
                sky.SetFloat("_SunSizeConvergence", 6f);
                sky.SetFloat("_AtmosphereThickness", 0.85f);
                sky.SetColor("_SkyTint", new Color(0.48f, 0.55f, 0.66f));
                sky.SetColor("_GroundColor", new Color(0.4f, 0.41f, 0.4f));
                sky.SetFloat("_Exposure", 1.2f);
                RenderSettings.skybox = AssetUtility.SaveGenerated(sky, EditorPaths.Materials + "/Sky.mat");
            }
            else
            {
                Debug.LogWarning("[FPV Sim] Skybox/Procedural shader not found; keeping the default sky.");
            }

            RenderSettings.ambientMode = AmbientMode.Trilight;
            RenderSettings.ambientSkyColor = new Color(0.56f, 0.66f, 0.82f);
            RenderSettings.ambientEquatorColor = new Color(0.5f, 0.55f, 0.58f);
            RenderSettings.ambientGroundColor = new Color(0.24f, 0.24f, 0.21f);
            RenderSettings.ambientIntensity = 1f;

            RenderSettings.fog = true;
            RenderSettings.fogMode = FogMode.Linear;
            RenderSettings.fogColor = HorizonColor;
            RenderSettings.fogStartDistance = 280f;
            RenderSettings.fogEndDistance = 1700f;

            BuildPostProcessing(root.transform);
            return root;
        }

        /// <summary>Global volume: neutral tonemapping, gentle bloom (rings / beacon), slight vignette.</summary>
        private static void BuildPostProcessing(Transform parent)
        {
            VolumeProfile profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(EditorPaths.VolumeProfile);
            if (profile == null)
            {
                AssetUtility.EnsureFolder(EditorPaths.Rendering);
                profile = ScriptableObject.CreateInstance<VolumeProfile>();
                AssetDatabase.CreateAsset(profile, EditorPaths.VolumeProfile);

                var tonemapping = profile.Add<Tonemapping>(true);
                tonemapping.mode.Override(TonemappingMode.Neutral);

                var bloom = profile.Add<Bloom>(true);
                bloom.threshold.Override(1.1f);
                bloom.intensity.Override(0.35f);

                var vignette = profile.Add<Vignette>(true);
                vignette.intensity.Override(0.2f);

                var colorAdjustments = profile.Add<ColorAdjustments>(true);
                colorAdjustments.postExposure.Override(0.1f);
                colorAdjustments.contrast.Override(8f);
                colorAdjustments.saturation.Override(8f);

                // Volume components are sub-assets of the profile and must be saved with it.
                foreach (VolumeComponent component in profile.components)
                {
                    AssetDatabase.AddObjectToAsset(component, profile);
                }

                EditorUtility.SetDirty(profile);
            }

            var volumeObject = new GameObject("PostProcessVolume");
            volumeObject.transform.SetParent(parent, false);
            var volume = volumeObject.AddComponent<Volume>();
            volume.isGlobal = true;
            volume.priority = 1f;
            volume.sharedProfile = profile;
        }
    }
}
