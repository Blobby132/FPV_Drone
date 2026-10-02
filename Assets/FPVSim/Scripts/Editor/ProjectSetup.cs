using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// One-click project configuration for a fresh checkout: creates and assigns a URP pipeline asset, switches
    /// to linear color space and sets the physics timestep. Safe to run repeatedly.
    /// </summary>
    public static class ProjectSetup
    {
        public const float DefaultFixedTimestep = 1f / 500f;

        [MenuItem("FPV Sim/Setup Project (URP, Physics, Color Space)", priority = 10)]
        public static void SetupFromMenu()
        {
            bool ok = EnsureProjectConfigured();
            AssetDatabase.SaveAssets();
            EditorUtility.DisplayDialog("FPV Sim",
                ok
                    ? "Project configured:\n\n- URP pipeline asset assigned\n- Linear color space\n- Fixed timestep 0.002 s (500 Hz)"
                    : "Project setup finished with warnings. See the Console for details.",
                "OK");
        }

        /// <returns>False if something needs manual attention (details are logged).</returns>
        public static bool EnsureProjectConfigured()
        {
            if (PlayerSettings.colorSpace != ColorSpace.Linear)
            {
                PlayerSettings.colorSpace = ColorSpace.Linear;
                Debug.Log("[FPV Sim] Switched color space to Linear.");
            }

            // The GameSession also sets this at runtime from the drone tuning; setting it here keeps the project
            // setting consistent with what actually runs.
            if (!Mathf.Approximately(Time.fixedDeltaTime, DefaultFixedTimestep))
            {
                Time.fixedDeltaTime = DefaultFixedTimestep;
                Debug.Log("[FPV Sim] Fixed timestep set to 0.002 s (500 Hz).");
            }

            return EnsureUrp() != null;
        }

        /// <summary>
        /// Uses the URP asset already assigned in Graphics settings if there is one; otherwise creates
        /// FPVSim_URP + renderer under Assets/FPVSim/Settings/Rendering and assigns it.
        /// </summary>
        public static UniversalRenderPipelineAsset EnsureUrp()
        {
            if (GraphicsSettings.defaultRenderPipeline is UniversalRenderPipelineAsset existingDefault)
            {
                return existingDefault;
            }

            UniversalRenderPipelineAsset asset = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(EditorPaths.UrpAsset);
            if (asset == null)
            {
                AssetUtility.EnsureFolder(EditorPaths.Rendering);

                UniversalRendererData rendererData =
                    AssetDatabase.LoadAssetAtPath<UniversalRendererData>(EditorPaths.UrpRenderer);
                if (rendererData == null)
                {
                    rendererData = ScriptableObject.CreateInstance<UniversalRendererData>();
                    PostProcessData postProcessData =
                        AssetDatabase.LoadAssetAtPath<PostProcessData>(EditorPaths.UrpDefaultPostProcessData);
                    if (postProcessData != null)
                    {
                        rendererData.postProcessData = postProcessData;
                    }
                    else
                    {
                        Debug.LogWarning("[FPV Sim] Could not find URP's default PostProcessData; post-processing " +
                                         "will be disabled. Assign it on " + EditorPaths.UrpRenderer + " if needed.");
                    }

                    AssetDatabase.CreateAsset(rendererData, EditorPaths.UrpRenderer);
                }

                asset = UniversalRenderPipelineAsset.Create(rendererData);
                if (asset == null)
                {
                    Debug.LogError("[FPV Sim] Failed to create a URP asset. Create one manually via " +
                                   "Assets > Create > Rendering > URP Asset (with Universal Renderer) and assign it " +
                                   "in Project Settings > Graphics.");
                    return null;
                }

                AssetDatabase.CreateAsset(asset, EditorPaths.UrpAsset);
                Debug.Log("[FPV Sim] Created URP asset at " + EditorPaths.UrpAsset);
            }

            // Settings suited to fast flying over a large open area.
            asset.shadowDistance = 160f;
            asset.shadowCascadeCount = 4;
            asset.mainLightShadowmapResolution = 2048;
            asset.msaaSampleCount = 4;
            asset.supportsHDR = true;
            EditorUtility.SetDirty(asset);

            GraphicsSettings.defaultRenderPipeline = asset;
            if (QualitySettings.renderPipeline != null && QualitySettings.renderPipeline != asset)
            {
                // The active quality level overrides the default pipeline; point it at ours too.
                QualitySettings.renderPipeline = asset;
            }

            Debug.Log("[FPV Sim] URP asset assigned in Graphics settings.");
            return asset;
        }
    }
}
