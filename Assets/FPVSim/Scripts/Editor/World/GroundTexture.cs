using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// Generates a tileable grass texture with a faint 8 m checker and grid lines. FPV pilots judge speed and
    /// height from ground texture, so a plain color would make the sim feel floaty.
    /// </summary>
    internal static class GroundTexture
    {
        private const int Resolution = 512;

        public static Texture2D Create(string path)
        {
            var texture = new Texture2D(Resolution, Resolution, TextureFormat.RGBA32, true, false)
            {
                wrapMode = TextureWrapMode.Repeat,
                filterMode = FilterMode.Trilinear,
                anisoLevel = 8,
            };

            var rng = new System.Random(7);
            float[] coarse = Lattice(rng, 8);
            float[] medium = Lattice(rng, 32);
            float[] fine = Lattice(rng, 128);

            var grassA = new Color(0.27f, 0.42f, 0.17f);
            var grassB = new Color(0.38f, 0.53f, 0.23f);
            int halfTile = Resolution / 2;
            var pixels = new Color32[Resolution * Resolution];

            for (int y = 0; y < Resolution; y++)
            {
                for (int x = 0; x < Resolution; x++)
                {
                    float u = (float)x / Resolution;
                    float v = (float)y / Resolution;
                    float n = 0.55f * ValueNoise(coarse, 8, u, v)
                              + 0.30f * ValueNoise(medium, 32, u, v)
                              + 0.15f * ValueNoise(fine, 128, u, v);
                    Color color = Color.Lerp(grassA, grassB, n);

                    bool darkSquare = ((x / halfTile) + (y / halfTile)) % 2 == 1;
                    if (darkSquare)
                    {
                        color *= 0.92f;
                    }

                    if (x % halfTile < 2 || y % halfTile < 2)
                    {
                        color *= 0.8f;
                    }

                    color.a = 1f;
                    pixels[y * Resolution + x] = color;
                }
            }

            texture.SetPixels32(pixels);
            texture.Apply(true);
            return AssetUtility.SaveGenerated(texture, path);
        }

        private static float[] Lattice(System.Random rng, int period)
        {
            var values = new float[period * period];
            for (int i = 0; i < values.Length; i++)
            {
                values[i] = (float)rng.NextDouble();
            }

            return values;
        }

        /// <summary>Smooth value noise that wraps every <paramref name="period"/> cells (tileable).</summary>
        private static float ValueNoise(float[] lattice, int period, float u, float v)
        {
            float fx = u * period;
            float fy = v * period;
            int x0 = Mathf.FloorToInt(fx);
            int y0 = Mathf.FloorToInt(fy);
            float tx = fx - x0;
            float ty = fy - y0;
            tx = tx * tx * (3f - 2f * tx);
            ty = ty * ty * (3f - 2f * ty);

            int x1 = (x0 + 1) % period;
            int y1 = (y0 + 1) % period;
            x0 %= period;
            y0 %= period;

            float a = lattice[y0 * period + x0];
            float b = lattice[y0 * period + x1];
            float c = lattice[y1 * period + x0];
            float d = lattice[y1 * period + x1];
            return Mathf.Lerp(Mathf.Lerp(a, b, tx), Mathf.Lerp(c, d, tx), ty);
        }
    }
}
