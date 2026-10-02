using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// Deterministic height function of the test map: a flat central field (launch pad, rings, gates), flat
    /// pads for the town / bando / container yard / tower, rolling hills around them and a ring of mountains at
    /// the edge. Used both to build the terrain mesh and to place objects on the ground.
    /// </summary>
    internal static class TerrainShape
    {
        public const float Size = 1600f;
        public const int Cells = 250;          // 6.4 m per quad, 251 x 251 vertices (fits 16-bit indices)
        public const float TextureTile = 16f;  // one ground texture repeat = 16 m (two 8 m checker squares)

        public static readonly Rect TownArea = Rect.MinMaxRect(-375f, 50f, -125f, 295f);
        public static readonly Rect BandoArea = Rect.MinMaxRect(205f, -30f, 265f, 30f);
        public static readonly Rect YardArea = Rect.MinMaxRect(85f, -185f, 170f, -105f);
        public static readonly Rect TowerArea = Rect.MinMaxRect(-80f, -240f, -40f, -200f);

        private const float FieldRadius = 170f;
        private const float HillBlend = 160f;

        public static float Height(float x, float z)
        {
            float radius = Mathf.Sqrt(x * x + z * z);

            // 0 where the ground must stay flat, 1 where hills are allowed.
            float hillMask = SmoothStep(FieldRadius, FieldRadius + HillBlend, radius);
            hillMask = Mathf.Min(hillMask, SmoothStep(0f, 70f, DistanceToRect(TownArea, x, z)));
            hillMask = Mathf.Min(hillMask, SmoothStep(0f, 50f, DistanceToRect(BandoArea, x, z)));
            hillMask = Mathf.Min(hillMask, SmoothStep(0f, 50f, DistanceToRect(YardArea, x, z)));
            hillMask = Mathf.Min(hillMask, SmoothStep(0f, 40f, DistanceToRect(TowerArea, x, z)));

            float noise = Mathf.PerlinNoise(x * 0.0045f + 113.7f, z * 0.0045f + 71.3f)
                          + 0.35f * Mathf.PerlinNoise(x * 0.013f + 37.1f, z * 0.013f + 191.9f)
                          + 0.12f * Mathf.PerlinNoise(x * 0.035f + 5.3f, z * 0.035f + 9.1f);
            noise /= 1.47f;
            float hills = Mathf.Max(0f, noise - 0.32f) * 85f;

            float bumps = Bump(x, z, 300f, -260f, 35f, 80f)
                          + Bump(x, z, -330f, -300f, 45f, 100f)
                          + Bump(x, z, 380f, 260f, 30f, 90f);

            float mountains = SmoothStep(560f, 780f, radius) *
                              (70f + 110f * Mathf.PerlinNoise(x * 0.005f + 400f, z * 0.005f + 400f));

            return hillMask * (hills + bumps) + mountains;
        }

        public static float DistanceToRect(Rect rect, float x, float z)
        {
            float dx = Mathf.Max(Mathf.Max(rect.xMin - x, 0f), x - rect.xMax);
            float dz = Mathf.Max(Mathf.Max(rect.yMin - z, 0f), z - rect.yMax);
            return Mathf.Sqrt(dx * dx + dz * dz);
        }

        private static float Bump(float x, float z, float cx, float cz, float height, float sigma)
        {
            float dx = x - cx;
            float dz = z - cz;
            return height * Mathf.Exp(-(dx * dx + dz * dz) / (2f * sigma * sigma));
        }

        private static float SmoothStep(float edge0, float edge1, float value)
        {
            float t = Mathf.Clamp01((value - edge0) / (edge1 - edge0));
            return t * t * (3f - 2f * t);
        }
    }
}
