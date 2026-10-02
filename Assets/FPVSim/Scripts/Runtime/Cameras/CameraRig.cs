using System;
using FPVSim.Flight;
using FPVSim.Settings;
using UnityEngine;

namespace FPVSim.Cameras
{
    /// <summary>
    /// Drives the single scene camera: FPV (rigidly mounted with uptilt, like a real FPV camera) or a smoothed
    /// chase view for debugging. Runs in LateUpdate so it follows the drone's interpolated transform.
    /// </summary>
    [RequireComponent(typeof(Camera))]
    [DisallowMultipleComponent]
    public sealed class CameraRig : MonoBehaviour
    {
        private const float FpvNearClip = 0.02f;
        private const float ChaseNearClip = 0.1f;
        private const float MinUptilt = -10f;
        private const float MaxUptilt = 70f;

        private Camera cameraComponent;
        private DroneController drone;
        private PilotSettings settings;
        private CameraViewMode mode = CameraViewMode.Fpv;
        private Vector3 chaseVelocity;
        private Vector3 chaseForward = Vector3.forward;
        private bool chaseInitialized;

        /// <summary>Raised when switching between FPV and chase view.</summary>
        public event Action<CameraViewMode> ViewChanged;

        /// <summary>Raised when the FPV uptilt changes (argument: new angle in degrees).</summary>
        public event Action<float> UptiltChanged;

        public CameraViewMode Mode => mode;
        public Camera ViewCamera => cameraComponent;
        public float Uptilt => settings != null ? settings.cameraUptilt : 0f;

        private void Awake()
        {
            cameraComponent = GetComponent<Camera>();
        }

        public void Initialize(DroneController target, PilotSettings pilotSettings)
        {
            if (cameraComponent == null)
            {
                cameraComponent = GetComponent<Camera>();
            }

            drone = target;
            settings = pilotSettings;
            mode = settings != null ? settings.startView : CameraViewMode.Fpv;
            chaseInitialized = false;
        }

        public void SetView(CameraViewMode newMode)
        {
            if (newMode == mode)
            {
                return;
            }

            mode = newMode;
            chaseInitialized = false;
            ViewChanged?.Invoke(mode);
        }

        public void ToggleView()
        {
            SetView(mode == CameraViewMode.Fpv ? CameraViewMode.Chase : CameraViewMode.Fpv);
        }

        /// <summary>Changes the FPV uptilt by one D-pad step (+1 = tilt up).</summary>
        public void StepUptilt(int direction)
        {
            if (settings == null)
            {
                return;
            }

            float step = settings.cameraTiltStep * Mathf.Sign(direction);
            settings.cameraUptilt = Mathf.Clamp(settings.cameraUptilt + step, MinUptilt, MaxUptilt);
            UptiltChanged?.Invoke(settings.cameraUptilt);
        }

        /// <summary>Snap the chase camera behind the drone (e.g. after a respawn).</summary>
        public void SnapChase()
        {
            chaseInitialized = false;
        }

        private void LateUpdate()
        {
            if (drone == null || settings == null || cameraComponent == null)
            {
                return;
            }

            if (mode == CameraViewMode.Fpv)
            {
                UpdateFpv();
            }
            else
            {
                UpdateChase(Time.deltaTime);
            }
        }

        private void UpdateFpv()
        {
            Transform mount = drone.CameraMount;
            // Negative rotation about local X tilts the view up.
            Quaternion rotation = mount.rotation * Quaternion.Euler(-settings.cameraUptilt, 0f, 0f);
            transform.SetPositionAndRotation(mount.position, rotation);
            cameraComponent.nearClipPlane = FpvNearClip;
            cameraComponent.fieldOfView = HorizontalToVerticalFov(settings.cameraFov, cameraComponent.aspect);
        }

        private void UpdateChase(float deltaTime)
        {
            Transform target = drone.transform;
            Vector3 targetPosition = target.position;

            // Follow the direction of travel when moving, otherwise the drone's heading.
            Vector3 velocity = drone.Body != null ? drone.Body.linearVelocity : Vector3.zero;
            Vector3 flatVelocity = new Vector3(velocity.x, 0f, velocity.z);
            Vector3 desiredForward = flatVelocity.sqrMagnitude > 9f
                ? flatVelocity.normalized
                : (AngleController.HeadingOf(target.rotation) * Vector3.forward);

            Vector3 desiredPosition;
            if (!chaseInitialized)
            {
                chaseForward = desiredForward;
                desiredPosition = targetPosition - chaseForward * settings.chaseDistance + Vector3.up * settings.chaseHeight;
                transform.position = desiredPosition;
                chaseVelocity = Vector3.zero;
                chaseInitialized = true;
            }
            else if (deltaTime > 0f)
            {
                chaseForward = Vector3.Slerp(chaseForward, desiredForward, 1f - Mathf.Exp(-3f * deltaTime)).normalized;
                desiredPosition = targetPosition - chaseForward * settings.chaseDistance + Vector3.up * settings.chaseHeight;
                transform.position = Vector3.SmoothDamp(transform.position, desiredPosition, ref chaseVelocity, 0.12f,
                    Mathf.Infinity, deltaTime);
            }

            Vector3 look = targetPosition + Vector3.up * 0.15f - transform.position;
            if (look.sqrMagnitude > 1e-4f)
            {
                transform.rotation = Quaternion.LookRotation(look, Vector3.up);
            }

            cameraComponent.nearClipPlane = ChaseNearClip;
            cameraComponent.fieldOfView = HorizontalToVerticalFov(settings.chaseFov, cameraComponent.aspect);
        }

        /// <summary>
        /// FPV lenses are specified by horizontal FOV, but Unity's Camera.fieldOfView is vertical. For 16:9,
        /// 120 deg horizontal is about 88 deg vertical.
        /// </summary>
        public static float HorizontalToVerticalFov(float horizontalDegrees, float aspect)
        {
            float h = Mathf.Clamp(horizontalDegrees, 1f, 170f) * Mathf.Deg2Rad;
            float v = 2f * Mathf.Atan(Mathf.Tan(h * 0.5f) / Mathf.Max(aspect, 0.01f));
            return Mathf.Clamp(v * Mathf.Rad2Deg, 1f, 179f);
        }
    }
}
