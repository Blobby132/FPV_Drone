using FPVSim.Flight;
using UnityEditor;
using UnityEngine;

namespace FPVSim.EditorTools
{
    /// <summary>
    /// Builds the 5" quad prefab from primitives: one box collider for physics, cosmetic parts without
    /// colliders, spinning tri-blade props, and a camera mount.
    /// </summary>
    internal static class DroneBuilder
    {
        private sealed class DroneMaterials
        {
            public Material carbon;
            public Material accent;
            public Material motor;
            public Material prop;
            public Material electronics;
            public Material battery;
            public Material camera;
        }

        public static GameObject BuildPrefab(DroneTuning tuning)
        {
            var materials = new DroneMaterials
            {
                carbon = MaterialLibrary.Lit("Drone_Carbon", new Color(0.08f, 0.08f, 0.09f), 0.55f),
                accent = MaterialLibrary.Lit("Drone_Accent", new Color(1f, 0.42f, 0.05f), 0.4f),
                motor = MaterialLibrary.Lit("Drone_Motor", new Color(0.62f, 0.64f, 0.68f), 0.7f, 0.85f),
                prop = MaterialLibrary.Lit("Drone_Prop", new Color(0.15f, 0.75f, 0.95f), 0.5f),
                electronics = MaterialLibrary.Lit("Drone_Electronics", new Color(0.05f, 0.35f, 0.15f), 0.4f),
                battery = MaterialLibrary.Lit("Drone_Battery", new Color(0.18f, 0.18f, 0.2f), 0.3f),
                camera = MaterialLibrary.Lit("Drone_Camera", new Color(0.25f, 0.25f, 0.28f), 0.6f, 0.3f),
            };

            var root = new GameObject("FPV_Quad");
            try
            {
                BuildHierarchy(root, tuning, materials);
                AssetUtility.EnsureFolder(EditorPaths.Prefabs);
                GameObject prefab = PrefabUtility.SaveAsPrefabAsset(root, EditorPaths.DronePrefab);
                return prefab;
            }
            finally
            {
                Object.DestroyImmediate(root);
            }
        }

        private static void BuildHierarchy(GameObject root, DroneTuning tuning, DroneMaterials m)
        {
            // Physics: one box around the frame and props. Center of mass is forced to the origin by
            // DroneController, so the collider shape doesn't affect handling.
            var body = root.AddComponent<Rigidbody>();
            body.mass = tuning.massKg;
            body.useGravity = true;
            body.interpolation = RigidbodyInterpolation.Interpolate;
            body.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;

            var collider = root.AddComponent<BoxCollider>();
            float span = tuning.armLength * 1.414f + 0.13f; // motor-to-motor plus prop radius
            collider.size = new Vector3(span * 0.85f, 0.065f, span * 0.85f);
            collider.center = new Vector3(0f, 0.018f, 0f);
            collider.sharedMaterial = CreateFramePhysicsMaterial();

            var controller = root.AddComponent<DroneController>();
            controller.DefaultTuning = tuning;

            Transform visuals = new GameObject("Visuals").transform;
            visuals.SetParent(root.transform, false);

            float a = tuning.armLength * 0.70710678f;
            Part(PrimitiveType.Cube, "BottomPlate", visuals, new Vector3(0f, -0.004f, 0f), new Vector3(0.05f, 0.004f, 0.15f), Quaternion.identity, m.carbon);
            Part(PrimitiveType.Cube, "TopPlate", visuals, new Vector3(0f, 0.03f, 0f), new Vector3(0.046f, 0.003f, 0.11f), Quaternion.identity, m.carbon);
            Part(PrimitiveType.Cube, "Stack", visuals, new Vector3(0f, 0.012f, -0.005f), new Vector3(0.032f, 0.018f, 0.032f), Quaternion.identity, m.electronics);
            Part(PrimitiveType.Cube, "Battery", visuals, new Vector3(0f, 0.049f, -0.006f), new Vector3(0.036f, 0.032f, 0.078f), Quaternion.identity, m.battery);
            Part(PrimitiveType.Cube, "BatteryStrap", visuals, new Vector3(0f, 0.049f, -0.006f), new Vector3(0.038f, 0.034f, 0.012f), Quaternion.identity, m.accent);
            Part(PrimitiveType.Cube, "CameraCage", visuals, new Vector3(0f, 0.014f, 0.06f), new Vector3(0.03f, 0.03f, 0.004f), Quaternion.identity, m.carbon);
            Part(PrimitiveType.Cube, "FpvCameraBody", visuals, new Vector3(0f, 0.016f, 0.066f), new Vector3(0.019f, 0.019f, 0.018f), Quaternion.Euler(-25f, 0f, 0f), m.camera);
            Part(PrimitiveType.Cylinder, "Antenna", visuals, new Vector3(0f, 0.04f, -0.07f), new Vector3(0.004f, 0.022f, 0.004f), Quaternion.Euler(-35f, 0f, 0f), m.accent);

            var propellers = new Transform[QuadAirframe.MotorCount];
            for (int i = 0; i < QuadAirframe.MotorCount; i++)
            {
                Vector2 sign = QuadAirframe.MotorSigns[i];
                var motorPosition = new Vector3(sign.x * a, 0f, sign.y * a);
                string label = MotorLabel(i);

                // Arm from the center to the motor.
                Quaternion armRotation = Quaternion.LookRotation(motorPosition.normalized, Vector3.up);
                Part(PrimitiveType.Cube, "Arm_" + label, visuals, motorPosition * 0.5f,
                    new Vector3(0.014f, 0.005f, tuning.armLength + 0.012f), armRotation, m.carbon);

                // Motor bell (Unity's cylinder is 2 units tall, so a Y scale of 0.008 gives 16 mm).
                Part(PrimitiveType.Cylinder, "Motor_" + label, visuals, motorPosition + new Vector3(0f, 0.008f, 0f),
                    new Vector3(0.028f, 0.008f, 0.028f), Quaternion.identity, m.motor);

                // Prop pivot rotated by DroneController; three blades around it.
                Transform pivot = new GameObject("Prop_" + label).transform;
                pivot.SetParent(visuals, false);
                pivot.localPosition = motorPosition + new Vector3(0f, 0.019f, 0f);
                for (int blade = 0; blade < 3; blade++)
                {
                    Quaternion bladeRotation = Quaternion.Euler(0f, blade * 120f, 0f);
                    Part(PrimitiveType.Cube, "Blade" + blade, pivot, bladeRotation * new Vector3(0f, 0f, 0.032f),
                        new Vector3(0.012f, 0.0015f, 0.0635f), bladeRotation * Quaternion.Euler(0f, 0f, 8f), m.prop);
                }

                propellers[i] = pivot;
            }

            Transform mount = new GameObject("CameraMount").transform;
            mount.SetParent(root.transform, false);
            mount.localPosition = new Vector3(0f, 0.018f, 0.072f);
            mount.localRotation = Quaternion.identity;

            controller.SetVisuals(mount, propellers);
        }

        private static string MotorLabel(int index)
        {
            switch (index)
            {
                case 0: return "RearRight";
                case 1: return "FrontRight";
                case 2: return "RearLeft";
                default: return "FrontLeft";
            }
        }

        private static GameObject Part(PrimitiveType type, string name, Transform parent, Vector3 localPosition,
            Vector3 localScale, Quaternion localRotation, Material material)
        {
            GameObject part = GameObject.CreatePrimitive(type);
            part.name = name;
            Object.DestroyImmediate(part.GetComponent<Collider>());
            part.transform.SetParent(parent, false);
            part.transform.localPosition = localPosition;
            part.transform.localRotation = localRotation;
            part.transform.localScale = localScale;
            var renderer = part.GetComponent<MeshRenderer>();
            renderer.sharedMaterial = material;
            renderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
            return part;
        }

        private static PhysicsMaterial CreateFramePhysicsMaterial()
        {
            var material = new PhysicsMaterial("DroneFrame")
            {
                dynamicFriction = 0.45f,
                staticFriction = 0.55f,
                bounciness = 0.15f,
                frictionCombine = PhysicsMaterialCombine.Average,
                bounceCombine = PhysicsMaterialCombine.Average,
            };
            return AssetUtility.SaveGenerated(material, EditorPaths.Materials + "/DroneFrame_Physics.asset");
        }
    }
}
