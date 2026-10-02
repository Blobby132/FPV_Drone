using System.Collections.Generic;
using FPVSim.Core;
using FPVSim.World;
using UnityEditor;
using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// Builds the free-fly test map from primitives and procedural meshes: terrain, launch pad, open field with
    /// rings and gates, a small town, an abandoned multi-storey frame ("bando"), a container yard, a radio
    /// tower, a forest and rocks. Everything has colliders and is marked static.
    /// </summary>
    internal static class EnvironmentBuilder
    {
        private const StaticEditorFlags StaticFlags =
            StaticEditorFlags.BatchingStatic | StaticEditorFlags.OccluderStatic | StaticEditorFlags.OccludeeStatic;

        private sealed class Palette
        {
            public Material ground;
            public Material pad;
            public Material padMarking;
            public Material asphalt;
            public Material concrete;
            public Material concreteDark;
            public Material roof;
            public Material[] buildings;
            public Material trunk;
            public Material leavesDark;
            public Material leavesLight;
            public Material rock;
            public Material ringOrange;
            public Material ringCyan;
            public Material gateRed;
            public Material gateWhite;
            public Material[] containers;
            public Material towerWhite;
            public Material towerRed;
            public Material beacon;
        }

        private static Palette palette;
        private static Mesh coneMesh;
        private static Mesh ringMesh;

        /// <summary>Builds everything under a new "Environment" root and returns it.</summary>
        public static GameObject Build()
        {
            palette = CreatePalette();
            coneMesh = AssetUtility.SaveGenerated(ProceduralMeshes.Cone(1f, 1f, 14), EditorPaths.Meshes + "/TreeCone.asset");
            ringMesh = AssetUtility.SaveGenerated(ProceduralMeshes.Torus(2.5f, 0.22f, 48, 14), EditorPaths.Meshes + "/Ring.asset");

            var root = new GameObject("Environment");
            BuildTerrain(root.transform);
            BuildLaunchPad(root.transform);
            BuildTown(Group(root.transform, "Town"));
            BuildBando(Group(root.transform, "Bando"), new Vector3(235f, 0f, 0f));
            BuildContainerYard(Group(root.transform, "ContainerYard"));
            BuildTower(Group(root.transform, "RadioTower"), new Vector3(-60f, 0f, -220f));
            BuildRings(Group(root.transform, "Rings"));
            BuildGates(Group(root.transform, "Gates"));
            BuildForest(Group(root.transform, "Forest"));
            BuildRocks(Group(root.transform, "Rocks"));
            return root;
        }

        // ------------------------------------------------------------------ materials

        private static Palette CreatePalette()
        {
            Texture2D groundTexture = GroundTexture.Create(EditorPaths.Textures + "/Ground.asset");
            return new Palette
            {
                ground = MaterialLibrary.Lit("Ground", Color.white, 0.08f, 0f, groundTexture, Vector2.one),
                pad = MaterialLibrary.Lit("LaunchPad", new Color(0.16f, 0.16f, 0.17f), 0.2f),
                padMarking = MaterialLibrary.Lit("LaunchPadMarking", new Color(0.95f, 0.95f, 0.9f), 0.2f),
                asphalt = MaterialLibrary.Lit("Asphalt", new Color(0.2f, 0.2f, 0.21f), 0.15f),
                concrete = MaterialLibrary.Lit("Concrete", new Color(0.62f, 0.61f, 0.58f), 0.15f),
                concreteDark = MaterialLibrary.Lit("ConcreteWeathered", new Color(0.45f, 0.44f, 0.42f), 0.1f),
                roof = MaterialLibrary.Lit("Roof", new Color(0.3f, 0.3f, 0.32f), 0.2f),
                buildings = new[]
                {
                    MaterialLibrary.Lit("Building_Sand", new Color(0.78f, 0.7f, 0.56f), 0.2f),
                    MaterialLibrary.Lit("Building_Brick", new Color(0.62f, 0.33f, 0.26f), 0.15f),
                    MaterialLibrary.Lit("Building_Grey", new Color(0.55f, 0.58f, 0.62f), 0.3f),
                    MaterialLibrary.Lit("Building_Glass", new Color(0.28f, 0.38f, 0.48f), 0.75f, 0.2f),
                    MaterialLibrary.Lit("Building_White", new Color(0.85f, 0.85f, 0.82f), 0.2f),
                },
                trunk = MaterialLibrary.Lit("TreeTrunk", new Color(0.33f, 0.23f, 0.15f), 0.1f),
                leavesDark = MaterialLibrary.Lit("LeavesDark", new Color(0.12f, 0.3f, 0.12f), 0.1f),
                leavesLight = MaterialLibrary.Lit("LeavesLight", new Color(0.26f, 0.46f, 0.16f), 0.1f),
                rock = MaterialLibrary.Lit("Rock", new Color(0.46f, 0.45f, 0.43f), 0.15f),
                ringOrange = MaterialLibrary.Emissive("RingOrange", new Color(1f, 0.45f, 0.05f), new Color(1f, 0.35f, 0f) * 1.5f),
                ringCyan = MaterialLibrary.Emissive("RingCyan", new Color(0.1f, 0.8f, 1f), new Color(0f, 0.6f, 1f) * 1.5f),
                gateRed = MaterialLibrary.Lit("GateRed", new Color(0.85f, 0.1f, 0.1f), 0.4f),
                gateWhite = MaterialLibrary.Lit("GateWhite", new Color(0.92f, 0.92f, 0.92f), 0.4f),
                containers = new[]
                {
                    MaterialLibrary.Lit("Container_Red", new Color(0.65f, 0.15f, 0.1f), 0.3f, 0.3f),
                    MaterialLibrary.Lit("Container_Blue", new Color(0.12f, 0.3f, 0.6f), 0.3f, 0.3f),
                    MaterialLibrary.Lit("Container_Green", new Color(0.15f, 0.45f, 0.25f), 0.3f, 0.3f),
                    MaterialLibrary.Lit("Container_Yellow", new Color(0.85f, 0.65f, 0.1f), 0.3f, 0.3f),
                },
                towerWhite = MaterialLibrary.Lit("TowerWhite", new Color(0.9f, 0.9f, 0.9f), 0.3f, 0.4f),
                towerRed = MaterialLibrary.Lit("TowerRed", new Color(0.8f, 0.12f, 0.1f), 0.3f, 0.4f),
                beacon = MaterialLibrary.Emissive("Beacon", new Color(1f, 0.1f, 0.05f), new Color(1f, 0.05f, 0f) * 4f),
            };
        }

        // ------------------------------------------------------------------ terrain and pad

        private static void BuildTerrain(Transform parent)
        {
            Mesh mesh = ProceduralMeshes.Heightfield(TerrainShape.Size, TerrainShape.Cells, TerrainShape.Height,
                TerrainShape.TextureTile);
            mesh = AssetUtility.SaveGenerated(mesh, EditorPaths.Meshes + "/Terrain.asset");

            var terrain = new GameObject("Terrain");
            terrain.transform.SetParent(parent, false);
            terrain.AddComponent<MeshFilter>().sharedMesh = mesh;
            terrain.AddComponent<MeshRenderer>().sharedMaterial = palette.ground;
            terrain.AddComponent<MeshCollider>().sharedMesh = mesh;
            GameObjectUtility.SetStaticEditorFlags(terrain, StaticFlags);
        }

        private static void BuildLaunchPad(Transform parent)
        {
            Transform pad = Group(parent, "LaunchPad");
            Box(pad, "Pad", new Vector3(0f, 0.05f, 0f), new Vector3(6f, 0.1f, 6f), Quaternion.identity, palette.pad);
            // "H" marking, slightly above the pad surface.
            Box(pad, "H_Left", new Vector3(-0.9f, 0.105f, 0f), new Vector3(0.45f, 0.012f, 3f), Quaternion.identity, palette.padMarking);
            Box(pad, "H_Right", new Vector3(0.9f, 0.105f, 0f), new Vector3(0.45f, 0.012f, 3f), Quaternion.identity, palette.padMarking);
            Box(pad, "H_Bar", new Vector3(0f, 0.105f, 0f), new Vector3(1.4f, 0.012f, 0.45f), Quaternion.identity, palette.padMarking);
        }

        // ------------------------------------------------------------------ town

        private static void BuildTown(Transform parent)
        {
            Rect area = TerrainShape.TownArea;
            Box(parent, "Plaza", new Vector3(area.center.x, 0.03f, area.center.y),
                new Vector3(area.width, 0.06f, area.height), Quaternion.identity, palette.asphalt);

            var rng = new System.Random(42);
            for (int i = 0; i < 5; i++)
            {
                for (int j = 0; j < 5; j++)
                {
                    float cx = -340f + 45f * i;
                    float cz = 85f + 45f * j;
                    if (i == 2 && j == 2)
                    {
                        // A small park in the middle of town: open space to drop into.
                        for (int k = 0; k < 4; k++)
                        {
                            float angle = k * Mathf.PI * 0.5f + 0.4f;
                            BroadleafTree(parent, new Vector3(cx + Mathf.Cos(angle) * 9f, 0.06f, cz + Mathf.Sin(angle) * 9f), 7f + k);
                        }

                        continue;
                    }

                    if (rng.NextDouble() < 0.3)
                    {
                        // Two narrow buildings with a gap between them (fly-through slot).
                        float h1 = Range(rng, 10f, 30f);
                        float h2 = Range(rng, 10f, 30f);
                        Building(parent, rng, new Vector3(cx - 8f, 0f, cz), new Vector3(11f, h1, Range(rng, 18f, 28f)));
                        Building(parent, rng, new Vector3(cx + 8f, 0f, cz), new Vector3(11f, h2, Range(rng, 18f, 28f)));
                        continue;
                    }

                    // Taller towers towards the middle of town.
                    float centrality = 1f - (Mathf.Abs(i - 2) + Mathf.Abs(j - 2)) / 4f;
                    float height = Range(rng, 8f, 22f) + centrality * Range(rng, 5f, 30f);
                    Building(parent, rng, new Vector3(cx, 0f, cz),
                        new Vector3(Range(rng, 16f, 29f), height, Range(rng, 16f, 29f)));
                }
            }
        }

        private static void Building(Transform parent, System.Random rng, Vector3 footprintCenter, Vector3 size)
        {
            Material body = palette.buildings[rng.Next(palette.buildings.Length)];
            Transform building = Group(parent, "Building");
            building.position = footprintCenter;

            Box(building, "Body", new Vector3(0f, size.y * 0.5f, 0f), size, Quaternion.identity, body);
            Box(building, "Cornice", new Vector3(0f, size.y + 0.3f, 0f), new Vector3(size.x + 0.6f, 0.6f, size.z + 0.6f),
                Quaternion.identity, palette.roof);

            int units = rng.Next(1, 4);
            for (int k = 0; k < units; k++)
            {
                var unitSize = new Vector3(Range(rng, 2f, 5f), Range(rng, 1.5f, 3.5f), Range(rng, 2f, 5f));
                var offset = new Vector3(Range(rng, -0.3f, 0.3f) * size.x, size.y + 0.6f + unitSize.y * 0.5f,
                    Range(rng, -0.3f, 0.3f) * size.z);
                Box(building, "RoofUnit", offset, unitSize, Quaternion.identity, palette.concrete);
            }
        }

        // ------------------------------------------------------------------ bando

        /// <summary>Abandoned 4-storey concrete frame: open sides, columns, a hole in one slab, parapets.</summary>
        private static void BuildBando(Transform parent, Vector3 origin)
        {
            parent.position = origin;
            const float width = 30f;       // x
            const float depth = 22f;       // z
            const float floorHeight = 4.5f;
            const float slab = 0.4f;
            const int floors = 4;
            float[] columnsX = { -14.5f, -5f, 5f, 14.5f };
            float[] columnsZ = { -10.5f, 0f, 10.5f };

            Box(parent, "GroundSlab", new Vector3(0f, 0.2f, 0f), new Vector3(width + 1f, 0.4f, depth + 1f), Quaternion.identity, palette.concreteDark);

            for (int f = 1; f <= floors; f++)
            {
                float y = f * floorHeight;
                if (f == 2)
                {
                    // Slab with a 9 x 7 m hole to dive through (x 2..11, z -3.5..3.5).
                    SlabWithHole(parent, "Slab_" + f, y, width, depth, slab, new Rect(2f, -3.5f, 9f, 7f));
                }
                else
                {
                    Box(parent, "Slab_" + f, new Vector3(0f, y, 0f), new Vector3(width, slab, depth), Quaternion.identity, palette.concrete);
                }

                // Columns between the previous slab and this one.
                float columnHeight = floorHeight - slab;
                float columnY = y - floorHeight * 0.5f;
                foreach (float cx in columnsX)
                {
                    foreach (float cz in columnsZ)
                    {
                        Box(parent, "Column", new Vector3(cx, columnY, cz), new Vector3(0.6f, columnHeight, 0.6f), Quaternion.identity, palette.concreteDark);
                    }
                }
            }

            // Back wall on the first two floors with window openings (sill 1 m, lintel above 3 m).
            for (int f = 0; f < 2; f++)
            {
                float baseY = f * floorHeight + (f == 0 ? 0.4f : slab * 0.5f);
                for (int bay = 0; bay < columnsX.Length - 1; bay++)
                {
                    float x0 = columnsX[bay] + 0.3f;
                    float x1 = columnsX[bay + 1] - 0.3f;
                    float cx = (x0 + x1) * 0.5f;
                    float w = x1 - x0;
                    Box(parent, "WallLow", new Vector3(cx, baseY + 0.5f, depth * 0.5f - 0.15f), new Vector3(w, 1f, 0.3f), Quaternion.identity, palette.concreteDark);
                    Box(parent, "WallHigh", new Vector3(cx, baseY + 3.55f, depth * 0.5f - 0.15f), new Vector3(w, 1.1f, 0.3f), Quaternion.identity, palette.concreteDark);
                }
            }

            // Roof parapet with gaps.
            float roofY = floors * floorHeight + slab * 0.5f + 0.5f;
            Box(parent, "Parapet_N", new Vector3(-6f, roofY, depth * 0.5f - 0.15f), new Vector3(18f, 1f, 0.3f), Quaternion.identity, palette.concreteDark);
            Box(parent, "Parapet_S", new Vector3(6f, roofY, -depth * 0.5f + 0.15f), new Vector3(18f, 1f, 0.3f), Quaternion.identity, palette.concreteDark);
            Box(parent, "Parapet_E", new Vector3(width * 0.5f - 0.15f, roofY, 4f), new Vector3(0.3f, 1f, 12f), Quaternion.identity, palette.concreteDark);
            // Stair / lift core poking out of the roof.
            Box(parent, "Core", new Vector3(-11f, floors * floorHeight + 1.6f, -6f), new Vector3(4f, 3.2f, 5f), Quaternion.identity, palette.concrete);
        }

        private static void SlabWithHole(Transform parent, string name, float y, float width, float depth, float thickness, Rect hole)
        {
            float xMin = -width * 0.5f;
            float xMax = width * 0.5f;
            float zMin = -depth * 0.5f;
            float zMax = depth * 0.5f;
            // Left of the hole (full depth), right of the hole (full depth), then front and back strips.
            BoxMinMax(parent, name + "_W", new Vector3(xMin, y - thickness * 0.5f, zMin), new Vector3(hole.xMin, y + thickness * 0.5f, zMax));
            BoxMinMax(parent, name + "_E", new Vector3(hole.xMax, y - thickness * 0.5f, zMin), new Vector3(xMax, y + thickness * 0.5f, zMax));
            BoxMinMax(parent, name + "_S", new Vector3(hole.xMin, y - thickness * 0.5f, zMin), new Vector3(hole.xMax, y + thickness * 0.5f, hole.yMin));
            BoxMinMax(parent, name + "_N", new Vector3(hole.xMin, y - thickness * 0.5f, hole.yMax), new Vector3(hole.xMax, y + thickness * 0.5f, zMax));
        }

        private static void BoxMinMax(Transform parent, string name, Vector3 min, Vector3 max)
        {
            Box(parent, name, (min + max) * 0.5f, max - min, Quaternion.identity, palette.concrete);
        }

        // ------------------------------------------------------------------ container yard

        private static void BuildContainerYard(Transform parent)
        {
            var rng = new System.Random(5);
            var containerSize = new Vector3(2.45f, 2.6f, 12.2f);
            Vector2 center = TerrainShape.YardArea.center;
            for (int row = 0; row < 4; row++)
            {
                for (int column = 0; column < 5; column++)
                {
                    if (rng.NextDouble() < 0.2)
                    {
                        continue; // leave some gaps
                    }

                    float x = center.x - 28f + column * 14f;
                    float z = center.y - 24f + row * 16f;
                    int stack = rng.Next(1, 4);
                    float yaw = rng.NextDouble() < 0.25 ? 90f : 0f;
                    for (int level = 0; level < stack; level++)
                    {
                        Material material = palette.containers[rng.Next(palette.containers.Length)];
                        var position = new Vector3(x, containerSize.y * (level + 0.5f), z);
                        Box(parent, "Container", position, containerSize, Quaternion.Euler(0f, yaw + Range(rng, -2f, 2f), 0f), material);
                    }
                }
            }
        }

        // ------------------------------------------------------------------ radio tower

        private static void BuildTower(Transform parent, Vector3 origin)
        {
            parent.position = origin;
            const float height = 72f;
            const float half = 2f;
            for (int i = 0; i < 4; i++)
            {
                float sx = i % 2 == 0 ? -half : half;
                float sz = i < 2 ? -half : half;
                Box(parent, "Leg", new Vector3(sx, height * 0.5f, sz), new Vector3(0.45f, height, 0.45f), Quaternion.identity, palette.towerWhite);
            }

            for (int level = 1; level * 8f < height; level++)
            {
                float y = level * 8f;
                Material material = level % 2 == 0 ? palette.towerRed : palette.towerWhite;
                Box(parent, "Brace", new Vector3(0f, y, -half), new Vector3(half * 2f, 0.25f, 0.25f), Quaternion.identity, material);
                Box(parent, "Brace", new Vector3(0f, y, half), new Vector3(half * 2f, 0.25f, 0.25f), Quaternion.identity, material);
                Box(parent, "Brace", new Vector3(-half, y, 0f), new Vector3(0.25f, 0.25f, half * 2f), Quaternion.identity, material);
                Box(parent, "Brace", new Vector3(half, y, 0f), new Vector3(0.25f, 0.25f, half * 2f), Quaternion.identity, material);
            }

            Box(parent, "Platform", new Vector3(0f, 40f, 0f), new Vector3(7f, 0.3f, 7f), Quaternion.identity, palette.towerRed);
            Box(parent, "Top", new Vector3(0f, height + 0.25f, 0f), new Vector3(4.6f, 0.5f, 4.6f), Quaternion.identity, palette.towerRed);
            GameObject beacon = Primitive(parent, PrimitiveType.Sphere, "Beacon", new Vector3(0f, height + 1.2f, 0f), Vector3.one * 1.2f, Quaternion.identity, palette.beacon);
            beacon.GetComponent<Renderer>().shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
        }

        // ------------------------------------------------------------------ rings and gates

        private static void BuildRings(Transform parent)
        {
            // Position (center of the ring) and yaw. Rings face +Z (rotated by yaw).
            var rings = new (Vector3 position, float yaw)[]
            {
                (new Vector3(0f, 3f, 35f), 0f),
                (new Vector3(14f, 5f, 65f), 20f),
                (new Vector3(34f, 8f, 95f), 35f),
                (new Vector3(65f, 12f, 112f), 70f),
                (new Vector3(0f, 32f, 150f), 0f),
                (new Vector3(185f, 7f, 0f), 90f),
                (new Vector3(-272.5f, 7f, 152.5f), 90f),
                (new Vector3(-52f, 50f, -220f), 0f),
            };

            for (int i = 0; i < rings.Length; i++)
            {
                Material material = i % 2 == 0 ? palette.ringOrange : palette.ringCyan;
                Ring(parent, i + 1, rings[i].position, Quaternion.Euler(0f, rings[i].yaw, 0f), material);
            }
        }

        private static void Ring(Transform parent, int index, Vector3 position, Quaternion rotation, Material material)
        {
            var ring = new GameObject("Ring_" + index.ToString("00"));
            ring.transform.SetParent(parent, false);
            ring.transform.SetPositionAndRotation(position, rotation);
            ring.AddComponent<MeshFilter>().sharedMesh = ringMesh;
            var renderer = ring.AddComponent<MeshRenderer>();
            renderer.sharedMaterial = material;
            ring.AddComponent<MeshCollider>().sharedMesh = ringMesh;
            GameObjectUtility.SetStaticEditorFlags(ring, StaticFlags);

            // Trigger inside the opening (inscribed square of the 4.56 m inner circle).
            var triggerObject = new GameObject("PassTrigger");
            triggerObject.transform.SetParent(ring.transform, false);
            var box = triggerObject.AddComponent<BoxCollider>();
            box.isTrigger = true;
            box.size = new Vector3(3.2f, 3.2f, 1f);
            var trigger = triggerObject.AddComponent<PassThroughTrigger>();
            trigger.TriggerId = "ring_" + index.ToString("00");
            trigger.Order = index;

            ring.AddComponent<PassFeedback>().Configure(trigger, new Renderer[] { renderer });

            // Support pole for low rings so they don't look like they float.
            float ground = TerrainShape.Height(position.x, position.z);
            float bottom = position.y - 2.5f - 0.22f;
            if (position.y < 16f && bottom - ground > 0.2f)
            {
                float poleHeight = bottom - ground;
                Box(parent, "RingPole_" + index.ToString("00"), new Vector3(position.x, ground + poleHeight * 0.5f, position.z),
                    new Vector3(0.15f, poleHeight, 0.15f), rotation, palette.gateWhite);
            }
        }

        private static void BuildGates(Transform parent)
        {
            Gate(parent, 1, new Vector3(-25f, 0f, 28f), 0f);
            Gate(parent, 2, new Vector3(-42f, 0f, 70f), -25f);
            Gate(parent, 3, new Vector3(25f, 0f, -40f), 160f);
            Gate(parent, 4, new Vector3(-70f, 0f, -20f), 100f);
        }

        /// <summary>MultiGP-style square gate with a 2 x 2 m opening (prop only for now, with a pass trigger).</summary>
        private static void Gate(Transform parent, int index, Vector3 position, float yaw)
        {
            const float opening = 2f;
            const float bar = 0.15f;
            Transform gate = Group(parent, "Gate_" + index.ToString("00"));
            gate.SetPositionAndRotation(new Vector3(position.x, TerrainShape.Height(position.x, position.z), position.z),
                Quaternion.Euler(0f, yaw, 0f));

            float postHeight = opening + bar;
            Box(gate, "PostLeft", new Vector3(-(opening + bar) * 0.5f, postHeight * 0.5f, 0f), new Vector3(bar, postHeight, bar), Quaternion.identity, palette.gateRed);
            Box(gate, "PostRight", new Vector3((opening + bar) * 0.5f, postHeight * 0.5f, 0f), new Vector3(bar, postHeight, bar), Quaternion.identity, palette.gateRed);
            Box(gate, "Top", new Vector3(0f, opening + bar * 0.5f, 0f), new Vector3(opening + bar * 2f, bar, bar), Quaternion.identity, palette.gateWhite);
            Box(gate, "FootLeft", new Vector3(-(opening + bar) * 0.5f, 0.05f, 0f), new Vector3(0.3f, 0.1f, 0.8f), Quaternion.identity, palette.gateWhite);
            Box(gate, "FootRight", new Vector3((opening + bar) * 0.5f, 0.05f, 0f), new Vector3(0.3f, 0.1f, 0.8f), Quaternion.identity, palette.gateWhite);

            var triggerObject = new GameObject("PassTrigger");
            triggerObject.transform.SetParent(gate, false);
            triggerObject.transform.localPosition = new Vector3(0f, opening * 0.5f, 0f);
            var box = triggerObject.AddComponent<BoxCollider>();
            box.isTrigger = true;
            box.size = new Vector3(opening - 0.1f, opening - 0.1f, 0.8f);
            var trigger = triggerObject.AddComponent<PassThroughTrigger>();
            trigger.TriggerId = "gate_" + index.ToString("00");
            trigger.Order = 100 + index;
        }

        // ------------------------------------------------------------------ vegetation and rocks

        private static void BuildForest(Transform parent)
        {
            var rng = new System.Random(2024);
            var placed = new List<Vector2>();

            // A dense patch north-east of the field, then trees scattered over the hills.
            Scatter(parent, rng, placed, 90, new Rect(150f, 140f, 260f, 260f), 7f);
            Scatter(parent, rng, placed, 60, new Rect(-560f, -520f, 300f, 200f), 7f);
            Scatter(parent, rng, placed, 200, new Rect(-700f, -700f, 1400f, 1400f), 10f);
        }

        private static void Scatter(Transform parent, System.Random rng, List<Vector2> placed, int count, Rect region, float spacing)
        {
            int added = 0;
            for (int attempt = 0; attempt < count * 30 && added < count; attempt++)
            {
                float x = Range(rng, region.xMin, region.xMax);
                float z = Range(rng, region.yMin, region.yMax);
                if (!IsTreeSpot(x, z, placed, spacing))
                {
                    continue;
                }

                float ground = TerrainShape.Height(x, z);
                if (ground > 75f)
                {
                    continue; // keep the mountain tops bare
                }

                var basePosition = new Vector3(x, ground - 0.2f, z);
                if (rng.NextDouble() < 0.65)
                {
                    PineTree(parent, basePosition, Range(rng, 8f, 17f), Range(rng, 0f, 360f), rng.NextDouble() < 0.5);
                }
                else
                {
                    BroadleafTree(parent, basePosition, Range(rng, 7f, 12f));
                }

                placed.Add(new Vector2(x, z));
                added++;
            }
        }

        private static bool IsTreeSpot(float x, float z, List<Vector2> placed, float spacing)
        {
            if (new Vector2(x, z).magnitude < 150f)
            {
                return false; // keep the central field open
            }

            if (TerrainShape.DistanceToRect(TerrainShape.TownArea, x, z) < 15f ||
                TerrainShape.DistanceToRect(TerrainShape.BandoArea, x, z) < 20f ||
                TerrainShape.DistanceToRect(TerrainShape.YardArea, x, z) < 15f ||
                TerrainShape.DistanceToRect(TerrainShape.TowerArea, x, z) < 15f)
            {
                return false;
            }

            float spacingSq = spacing * spacing;
            for (int i = 0; i < placed.Count; i++)
            {
                if ((placed[i] - new Vector2(x, z)).sqrMagnitude < spacingSq)
                {
                    return false;
                }
            }

            return true;
        }

        private static void PineTree(Transform parent, Vector3 basePosition, float height, float yaw, bool darkLeaves)
        {
            Transform tree = Group(parent, "Pine");
            tree.SetPositionAndRotation(basePosition, Quaternion.Euler(0f, yaw, 0f));
            float trunkHeight = height * 0.4f;
            Primitive(tree, PrimitiveType.Cylinder, "Trunk", new Vector3(0f, trunkHeight * 0.5f, 0f),
                new Vector3(0.45f, trunkHeight * 0.5f, 0.45f), Quaternion.identity, palette.trunk);

            Material leaves = darkLeaves ? palette.leavesDark : palette.leavesLight;
            float radius = height * 0.26f;
            for (int k = 0; k < 3; k++)
            {
                float coneBase = height * (0.25f + 0.2f * k);
                float coneHeight = height * (0.45f - 0.07f * k);
                float coneRadius = radius * (1f - 0.25f * k);
                Cone(tree, "Foliage" + k, new Vector3(0f, coneBase, 0f), new Vector3(coneRadius, coneHeight, coneRadius), leaves);
            }
        }

        private static void BroadleafTree(Transform parent, Vector3 basePosition, float height)
        {
            Transform tree = Group(parent, "Broadleaf");
            tree.position = basePosition;
            float trunkHeight = height * 0.45f;
            Primitive(tree, PrimitiveType.Cylinder, "Trunk", new Vector3(0f, trunkHeight * 0.5f, 0f),
                new Vector3(0.55f, trunkHeight * 0.5f, 0.55f), Quaternion.identity, palette.trunk);
            Primitive(tree, PrimitiveType.Sphere, "Canopy", new Vector3(0f, height * 0.68f, 0f),
                new Vector3(height * 0.62f, height * 0.5f, height * 0.62f), Quaternion.identity, palette.leavesLight);
        }

        private static void BuildRocks(Transform parent)
        {
            var rng = new System.Random(99);
            for (int i = 0; i < 30; i++)
            {
                float angle = Range(rng, 0f, Mathf.PI * 2f);
                float distance = Range(rng, 200f, 600f);
                float x = Mathf.Cos(angle) * distance;
                float z = Mathf.Sin(angle) * distance;
                if (TerrainShape.DistanceToRect(TerrainShape.TownArea, x, z) < 20f)
                {
                    continue;
                }

                float size = Range(rng, 2f, 7f);
                var scale = new Vector3(size * Range(rng, 0.8f, 1.4f), size * Range(rng, 0.5f, 0.9f), size * Range(rng, 0.8f, 1.4f));
                var position = new Vector3(x, TerrainShape.Height(x, z) - scale.y * 0.15f, z);
                Primitive(parent, PrimitiveType.Sphere, "Rock", position, scale,
                    Quaternion.Euler(Range(rng, -15f, 15f), Range(rng, 0f, 360f), Range(rng, -15f, 15f)), palette.rock);
            }
        }

        // ------------------------------------------------------------------ helpers

        private static Transform Group(Transform parent, string name)
        {
            var group = new GameObject(name).transform;
            group.SetParent(parent, false);
            return group;
        }

        /// <summary>A static cube with a BoxCollider. Position/rotation are local to <paramref name="parent"/>.</summary>
        private static GameObject Box(Transform parent, string name, Vector3 localPosition, Vector3 size, Quaternion localRotation, Material material)
        {
            return Primitive(parent, PrimitiveType.Cube, name, localPosition, size, localRotation, material);
        }

        /// <summary>A static Unity primitive with its default collider.</summary>
        private static GameObject Primitive(Transform parent, PrimitiveType type, string name, Vector3 localPosition,
            Vector3 localScale, Quaternion localRotation, Material material)
        {
            GameObject go = GameObject.CreatePrimitive(type);
            go.name = name;
            go.transform.SetParent(parent, false);
            go.transform.localPosition = localPosition;
            go.transform.localRotation = localRotation;
            go.transform.localScale = localScale;
            go.GetComponent<MeshRenderer>().sharedMaterial = material;
            GameObjectUtility.SetStaticEditorFlags(go, StaticFlags);
            return go;
        }

        private static void Cone(Transform parent, string name, Vector3 localPosition, Vector3 localScale, Material material)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            go.transform.localPosition = localPosition;
            go.transform.localScale = localScale;
            go.AddComponent<MeshFilter>().sharedMesh = coneMesh;
            go.AddComponent<MeshRenderer>().sharedMaterial = material;
            var collider = go.AddComponent<MeshCollider>();
            collider.sharedMesh = coneMesh;
            collider.convex = true;
            GameObjectUtility.SetStaticEditorFlags(go, StaticFlags);
        }

        private static float Range(System.Random rng, float min, float max)
        {
            return min + (float)rng.NextDouble() * (max - min);
        }
    }
}
