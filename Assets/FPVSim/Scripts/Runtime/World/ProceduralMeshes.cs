using System;
using UnityEngine;
using UnityEngine.Rendering;

namespace FPVSim.World
{
    /// <summary>
    /// Mesh generators for the test environment (terrain, rings, tree cones). Deterministic and dependency-free
    /// so level tools can reuse them later. Triangles are wound clockwise (Unity's front face).
    /// </summary>
    public static class ProceduralMeshes
    {
        /// <summary>
        /// Square heightfield centered on the origin.
        /// </summary>
        /// <param name="size">Edge length in meters.</param>
        /// <param name="cells">Quads per edge.</param>
        /// <param name="height">Height function (world x, world z) -> y.</param>
        /// <param name="uvTileSize">World size of one texture repeat, in meters.</param>
        public static Mesh Heightfield(float size, int cells, Func<float, float, float> height, float uvTileSize)
        {
            cells = Mathf.Max(1, cells);
            int perSide = cells + 1;
            var vertices = new Vector3[perSide * perSide];
            var uvs = new Vector2[vertices.Length];
            float half = size * 0.5f;
            float step = size / cells;

            for (int z = 0; z < perSide; z++)
            {
                for (int x = 0; x < perSide; x++)
                {
                    float wx = -half + x * step;
                    float wz = -half + z * step;
                    int i = z * perSide + x;
                    vertices[i] = new Vector3(wx, height(wx, wz), wz);
                    uvs[i] = new Vector2(wx / uvTileSize, wz / uvTileSize);
                }
            }

            var triangles = new int[cells * cells * 6];
            int t = 0;
            for (int z = 0; z < cells; z++)
            {
                for (int x = 0; x < cells; x++)
                {
                    int i0 = z * perSide + x;
                    int i1 = i0 + 1;
                    int i2 = i0 + perSide;
                    int i3 = i2 + 1;
                    triangles[t++] = i0;
                    triangles[t++] = i2;
                    triangles[t++] = i1;
                    triangles[t++] = i1;
                    triangles[t++] = i2;
                    triangles[t++] = i3;
                }
            }

            var mesh = new Mesh { name = "Heightfield" };
            mesh.indexFormat = vertices.Length > 65000 ? IndexFormat.UInt32 : IndexFormat.UInt16;
            mesh.vertices = vertices;
            mesh.uv = uvs;
            mesh.triangles = triangles;
            mesh.RecalculateNormals();
            mesh.RecalculateTangents();
            mesh.RecalculateBounds();
            return mesh;
        }

        /// <summary>
        /// Torus lying in the local XY plane, so you fly through it along Z.
        /// </summary>
        public static Mesh Torus(float majorRadius, float minorRadius, int majorSegments = 48, int minorSegments = 16)
        {
            int rows = majorSegments + 1;
            int columns = minorSegments + 1;
            var vertices = new Vector3[rows * columns];
            var normals = new Vector3[vertices.Length];
            var uvs = new Vector2[vertices.Length];

            for (int i = 0; i < rows; i++)
            {
                float u = (float)i / majorSegments * Mathf.PI * 2f;
                var center = new Vector3(Mathf.Cos(u) * majorRadius, Mathf.Sin(u) * majorRadius, 0f);
                for (int j = 0; j < columns; j++)
                {
                    float v = (float)j / minorSegments * Mathf.PI * 2f;
                    var normal = new Vector3(Mathf.Cos(u) * Mathf.Cos(v), Mathf.Sin(u) * Mathf.Cos(v), Mathf.Sin(v));
                    int index = i * columns + j;
                    vertices[index] = center + normal * minorRadius;
                    normals[index] = normal;
                    uvs[index] = new Vector2((float)i / majorSegments, (float)j / minorSegments);
                }
            }

            var triangles = new int[majorSegments * minorSegments * 6];
            int t = 0;
            for (int i = 0; i < majorSegments; i++)
            {
                for (int j = 0; j < minorSegments; j++)
                {
                    int a = i * columns + j;
                    int b = (i + 1) * columns + j;
                    int c = (i + 1) * columns + j + 1;
                    int d = i * columns + j + 1;
                    triangles[t++] = a;
                    triangles[t++] = b;
                    triangles[t++] = c;
                    triangles[t++] = a;
                    triangles[t++] = c;
                    triangles[t++] = d;
                }
            }

            var mesh = new Mesh { name = "Torus" };
            mesh.vertices = vertices;
            mesh.normals = normals;
            mesh.uv = uvs;
            mesh.triangles = triangles;
            mesh.RecalculateTangents();
            mesh.RecalculateBounds();
            return mesh;
        }

        /// <summary>Closed cone with its base at y = 0 and the apex at y = height.</summary>
        public static Mesh Cone(float radius, float height, int segments = 16)
        {
            segments = Mathf.Max(3, segments);
            // Side: one base vertex per segment edge (seam duplicated) plus one apex per segment for smooth normals.
            int sideBase = segments + 1;
            var vertices = new Vector3[sideBase * 2 + segments + 1];
            var normals = new Vector3[vertices.Length];
            var uvs = new Vector2[vertices.Length];
            float slant = Mathf.Sqrt(radius * radius + height * height);
            float normalY = radius / slant;
            float normalXZ = height / slant;

            for (int i = 0; i <= segments; i++)
            {
                float angle = (float)i / segments * Mathf.PI * 2f;
                float cos = Mathf.Cos(angle);
                float sin = Mathf.Sin(angle);
                vertices[i] = new Vector3(cos * radius, 0f, sin * radius);
                normals[i] = new Vector3(cos * normalXZ, normalY, sin * normalXZ);
                uvs[i] = new Vector2((float)i / segments, 0f);

                float midAngle = (i + 0.5f) / segments * Mathf.PI * 2f;
                vertices[sideBase + i] = new Vector3(0f, height, 0f);
                normals[sideBase + i] = new Vector3(Mathf.Cos(midAngle) * normalXZ, normalY, Mathf.Sin(midAngle) * normalXZ);
                uvs[sideBase + i] = new Vector2((i + 0.5f) / segments, 1f);
            }

            // Bottom cap: ring + center.
            int capStart = sideBase * 2;
            for (int i = 0; i < segments; i++)
            {
                float angle = (float)i / segments * Mathf.PI * 2f;
                vertices[capStart + i] = new Vector3(Mathf.Cos(angle) * radius, 0f, Mathf.Sin(angle) * radius);
                normals[capStart + i] = Vector3.down;
                uvs[capStart + i] = new Vector2(Mathf.Cos(angle) * 0.5f + 0.5f, Mathf.Sin(angle) * 0.5f + 0.5f);
            }

            int center = capStart + segments;
            vertices[center] = Vector3.zero;
            normals[center] = Vector3.down;
            uvs[center] = new Vector2(0.5f, 0.5f);

            var triangles = new int[segments * 6];
            int t = 0;
            for (int i = 0; i < segments; i++)
            {
                // Side (outward facing).
                triangles[t++] = i;
                triangles[t++] = sideBase + i;
                triangles[t++] = i + 1;

                // Bottom cap (facing down).
                triangles[t++] = center;
                triangles[t++] = capStart + i;
                triangles[t++] = capStart + (i + 1) % segments;
            }

            var mesh = new Mesh { name = "Cone" };
            mesh.vertices = vertices;
            mesh.normals = normals;
            mesh.uv = uvs;
            mesh.triangles = triangles;
            mesh.RecalculateTangents();
            mesh.RecalculateBounds();
            return mesh;
        }
    }
}
