using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>Creates URP Lit materials and saves them under Generated/Materials.</summary>
    internal static class MaterialLibrary
    {
        private const string LitShaderName = "Universal Render Pipeline/Lit";

        private static readonly int BaseColorId = Shader.PropertyToID("_BaseColor");
        private static readonly int BaseMapId = Shader.PropertyToID("_BaseMap");
        private static readonly int SmoothnessId = Shader.PropertyToID("_Smoothness");
        private static readonly int MetallicId = Shader.PropertyToID("_Metallic");
        private static readonly int EmissionColorId = Shader.PropertyToID("_EmissionColor");

        public static Material Lit(string name, Color color, float smoothness = 0.25f, float metallic = 0f,
            Texture2D baseMap = null, Vector2? tiling = null)
        {
            var material = new Material(FindLitShader());
            material.color = color;
            if (material.HasProperty(BaseColorId))
            {
                material.SetColor(BaseColorId, color);
            }

            if (material.HasProperty(SmoothnessId))
            {
                material.SetFloat(SmoothnessId, smoothness);
            }

            if (material.HasProperty(MetallicId))
            {
                material.SetFloat(MetallicId, metallic);
            }

            if (baseMap != null)
            {
                material.mainTexture = baseMap;
                if (material.HasProperty(BaseMapId))
                {
                    material.SetTexture(BaseMapId, baseMap);
                    material.SetTextureScale(BaseMapId, tiling ?? Vector2.one);
                }
            }

            material.enableInstancing = true;
            return AssetUtility.SaveGenerated(material, EditorPaths.Materials + "/" + name + ".mat");
        }

        public static Material Emissive(string name, Color color, Color emission, float smoothness = 0.4f)
        {
            var material = new Material(FindLitShader());
            material.color = color;
            if (material.HasProperty(BaseColorId))
            {
                material.SetColor(BaseColorId, color);
            }

            if (material.HasProperty(SmoothnessId))
            {
                material.SetFloat(SmoothnessId, smoothness);
            }

            material.EnableKeyword("_EMISSION");
            if (material.HasProperty(EmissionColorId))
            {
                material.SetColor(EmissionColorId, emission);
            }

            material.globalIlluminationFlags = MaterialGlobalIlluminationFlags.RealtimeEmissive;
            material.enableInstancing = true;
            return AssetUtility.SaveGenerated(material, EditorPaths.Materials + "/" + name + ".mat");
        }

        private static Shader FindLitShader()
        {
            Shader shader = Shader.Find(LitShaderName);
            if (shader == null)
            {
                Debug.LogWarning("[FPV Sim] URP Lit shader not found (is the URP package installed?). " +
                                 "Falling back to the Standard shader.");
                shader = Shader.Find("Standard");
            }

            return shader;
        }
    }
}
