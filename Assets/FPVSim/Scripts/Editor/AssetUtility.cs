using System.IO;
using UnityEditor;
using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>Small AssetDatabase helpers used by the builders.</summary>
    internal static class AssetUtility
    {
        /// <summary>Creates a folder (and its parents) under Assets if needed. Path uses forward slashes.</summary>
        public static void EnsureFolder(string folder)
        {
            folder = folder.TrimEnd('/');
            if (AssetDatabase.IsValidFolder(folder))
            {
                return;
            }

            string parent = Path.GetDirectoryName(folder)?.Replace('\\', '/');
            string leaf = Path.GetFileName(folder);
            if (!string.IsNullOrEmpty(parent))
            {
                EnsureFolder(parent);
            }

            AssetDatabase.CreateFolder(parent, leaf);
        }

        /// <summary>Loads a ScriptableObject asset or creates it with default values (never overwrites).</summary>
        public static T LoadOrCreate<T>(string path) where T : ScriptableObject
        {
            T asset = AssetDatabase.LoadAssetAtPath<T>(path);
            if (asset != null)
            {
                return asset;
            }

            EnsureFolder(Path.GetDirectoryName(path)?.Replace('\\', '/'));
            asset = ScriptableObject.CreateInstance<T>();
            AssetDatabase.CreateAsset(asset, path);
            return asset;
        }

        /// <summary>
        /// Saves a freshly generated object as an asset. If an asset already exists at the path, its contents are
        /// replaced in place so its GUID (and every reference to it) stays the same.
        /// </summary>
        public static T SaveGenerated<T>(T generated, string path) where T : Object
        {
            EnsureFolder(Path.GetDirectoryName(path)?.Replace('\\', '/'));
            T existing = AssetDatabase.LoadAssetAtPath<T>(path);
            if (existing != null && existing != generated)
            {
                EditorUtility.CopySerialized(generated, existing);
                existing.name = Path.GetFileNameWithoutExtension(path);
                Object.DestroyImmediate(generated);
                EditorUtility.SetDirty(existing);
                return existing;
            }

            generated.name = Path.GetFileNameWithoutExtension(path);
            AssetDatabase.CreateAsset(generated, path);
            return generated;
        }
    }
}
