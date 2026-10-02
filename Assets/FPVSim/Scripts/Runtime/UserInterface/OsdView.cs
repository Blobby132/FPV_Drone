using FPVSim.Cameras;
using FPVSim.Flight;
using FPVSim.Settings;
using UnityEngine;
using UnityEngine.UI;

namespace FPVSim.UserInterface
{
    /// <summary>
    /// Minimal Betaflight-style OSD: flight mode, speed, altitude, throttle, (cosmetic) battery, flight timer,
    /// a crosshair, warnings and short toast messages. Builds its own canvas at runtime. Values refresh at
    /// 20 Hz like a real OSD (and to keep string allocations low).
    /// </summary>
    [DisallowMultipleComponent]
    public sealed class OsdView : MonoBehaviour
    {
        private const float RefreshInterval = 0.05f;
        private const float Margin = 40f;
        private const float LowCellVoltage = 3.5f;

        private static readonly Color MainColor = new Color(1f, 1f, 1f, 0.95f);
        private static readonly Color WarningColor = new Color(1f, 0.55f, 0.1f, 1f);
        private static readonly Color AlarmColor = new Color(1f, 0.25f, 0.2f, 1f);
        private static readonly Color AngleColor = new Color(0.55f, 1f, 0.55f, 1f);

        private DroneController drone;
        private CameraRig cameraRig;
        private PilotSettings pilot;

        private Canvas canvas;
        private Text modeText;
        private Text viewText;
        private Text batteryText;
        private Text batteryDetailText;
        private Text timerText;
        private Text throttleText;
        private Text speedText;
        private Text altitudeText;
        private Text crosshairText;
        private Text warningText;
        private Text toastText;

        private float nextRefresh;
        private float toastUntil;
        private string resetButtonName = "Circle";

        public void Initialize(DroneController target, CameraRig rig, PilotSettings pilotSettings)
        {
            Unsubscribe();
            drone = target;
            cameraRig = rig;
            pilot = pilotSettings;

            if (canvas == null)
            {
                Build();
            }

            if (drone != null)
            {
                drone.FlightModeChanged += OnFlightModeChanged;
                drone.Crashed += OnCrashed;
            }

            if (cameraRig != null)
            {
                cameraRig.ViewChanged += OnViewChanged;
                cameraRig.UptiltChanged += OnUptiltChanged;
            }

            Refresh();
        }

        /// <summary>Name of the reset button shown in the crash message (follows rebinding).</summary>
        public void SetResetButtonName(string buttonName)
        {
            resetButtonName = string.IsNullOrEmpty(buttonName) ? "Circle" : buttonName;
        }

        /// <summary>Shows a short centered message (mode changes, camera tilt, "Settings saved", ...).</summary>
        public void ShowToast(string message, float duration = 1.2f)
        {
            if (toastText == null)
            {
                return;
            }

            toastText.text = message;
            toastText.enabled = true;
            toastUntil = Time.unscaledTime + duration;
        }

        private void OnDestroy()
        {
            Unsubscribe();
        }

        private void Unsubscribe()
        {
            if (drone != null)
            {
                drone.FlightModeChanged -= OnFlightModeChanged;
                drone.Crashed -= OnCrashed;
            }

            if (cameraRig != null)
            {
                cameraRig.ViewChanged -= OnViewChanged;
                cameraRig.UptiltChanged -= OnUptiltChanged;
            }
        }

        private void Update()
        {
            if (canvas == null)
            {
                return;
            }

            bool visible = pilot == null || pilot.showOsd;
            if (canvas.enabled != visible)
            {
                canvas.enabled = visible;
            }

            if (toastText.enabled && Time.unscaledTime > toastUntil)
            {
                toastText.enabled = false;
            }

            if (Time.unscaledTime >= nextRefresh)
            {
                nextRefresh = Time.unscaledTime + RefreshInterval;
                Refresh();
            }
        }

        private void Refresh()
        {
            if (drone == null || modeText == null)
            {
                return;
            }

            bool imperial = pilot != null && pilot.imperialUnits;

            bool acro = drone.Mode == FlightMode.Acro;
            modeText.text = acro ? "ACRO" : "ANGLE";
            modeText.color = acro ? MainColor : AngleColor;

            bool fpv = cameraRig == null || cameraRig.Mode == CameraViewMode.Fpv;
            viewText.text = fpv ? "CAM " + Mathf.RoundToInt(cameraRig != null ? cameraRig.Uptilt : 0f) + "°" : "CHASE CAM";
            crosshairText.enabled = fpv;

            BatterySimulator battery = drone.Battery;
            float cellVoltage = battery.CellVoltage;
            batteryText.text = battery.Voltage.ToString("0.0") + "V";
            batteryDetailText.text = cellVoltage.ToString("0.00") + "V/cell  " +
                                     Mathf.RoundToInt(battery.StateOfCharge * 100f) + "%  " +
                                     Mathf.RoundToInt(battery.CurrentAmps) + "A";
            Color batteryColor = cellVoltage < LowCellVoltage ? WarningColor : MainColor;
            batteryText.color = batteryColor;
            batteryDetailText.color = batteryColor;

            int seconds = Mathf.FloorToInt(drone.FlightTime);
            timerText.text = (seconds / 60).ToString("00") + ":" + (seconds % 60).ToString("00");

            throttleText.text = "THR " + Mathf.RoundToInt(drone.Throttle * 100f) + "%";

            float speed = drone.Speed;
            speedText.text = imperial
                ? Mathf.RoundToInt(speed * 2.23694f) + " mph"
                : Mathf.RoundToInt(speed * 3.6f) + " km/h";

            float altitude = drone.Altitude;
            altitudeText.text = imperial
                ? "ALT " + Mathf.RoundToInt(altitude * 3.28084f) + " ft"
                : "ALT " + altitude.ToString("0.0") + " m";

            UpdateWarning(cellVoltage);
        }

        private void UpdateWarning(float cellVoltage)
        {
            if (drone.IsCrashed)
            {
                warningText.text = "CRASHED\npress " + resetButtonName + " to reset";
                warningText.color = AlarmColor;
                warningText.enabled = true;
            }
            else if (cellVoltage < LowCellVoltage)
            {
                // Blink at 2 Hz.
                warningText.text = "LOW BATTERY";
                warningText.color = WarningColor;
                warningText.enabled = Mathf.Repeat(Time.unscaledTime, 1f) < 0.5f;
            }
            else
            {
                warningText.enabled = false;
            }
        }

        private void OnFlightModeChanged(FlightMode mode)
        {
            ShowToast(mode == FlightMode.Acro ? "ACRO MODE" : "ANGLE MODE");
            Refresh();
        }

        private void OnCrashed()
        {
            Refresh();
        }

        private void OnViewChanged(CameraViewMode mode)
        {
            ShowToast(mode == CameraViewMode.Fpv ? "FPV CAMERA" : "CHASE CAMERA");
            Refresh();
        }

        private void OnUptiltChanged(float degrees)
        {
            ShowToast("CAMERA TILT " + Mathf.RoundToInt(degrees) + "°", 0.8f);
            Refresh();
        }

        private void Build()
        {
            canvas = UiFactory.CreateCanvas("OSD Canvas", transform, 10);
            Transform root = canvas.transform;

            modeText = Corner(root, "Mode", new Vector2(0f, 1f), new Vector2(Margin, -30f), 36, TextAnchor.UpperLeft);
            viewText = Corner(root, "View", new Vector2(0f, 1f), new Vector2(Margin, -78f), 24, TextAnchor.UpperLeft);
            batteryText = Corner(root, "Battery", new Vector2(1f, 1f), new Vector2(-Margin, -30f), 36, TextAnchor.UpperRight);
            batteryDetailText = Corner(root, "BatteryDetail", new Vector2(1f, 1f), new Vector2(-Margin, -78f), 24, TextAnchor.UpperRight);
            timerText = Corner(root, "Timer", new Vector2(0.5f, 1f), new Vector2(0f, -30f), 30, TextAnchor.UpperCenter);
            throttleText = Corner(root, "Throttle", new Vector2(0f, 0f), new Vector2(Margin, 30f), 34, TextAnchor.LowerLeft);
            speedText = Corner(root, "Speed", new Vector2(1f, 0f), new Vector2(-Margin, 76f), 34, TextAnchor.LowerRight);
            altitudeText = Corner(root, "Altitude", new Vector2(1f, 0f), new Vector2(-Margin, 30f), 34, TextAnchor.LowerRight);

            crosshairText = UiFactory.CreateText("Crosshair", root, "-+-", 28, TextAnchor.MiddleCenter, MainColor);
            UiFactory.Place(crosshairText.rectTransform, new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), Vector2.zero, new Vector2(120f, 40f));

            warningText = UiFactory.CreateText("Warning", root, "", 40, TextAnchor.MiddleCenter, AlarmColor);
            UiFactory.Place(warningText.rectTransform, new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), new Vector2(0f, -140f), new Vector2(900f, 120f));
            warningText.enabled = false;

            toastText = UiFactory.CreateText("Toast", root, "", 32, TextAnchor.MiddleCenter, MainColor);
            UiFactory.Place(toastText.rectTransform, new Vector2(0.5f, 1f), new Vector2(0.5f, 1f), new Vector2(0f, -110f), new Vector2(900f, 60f));
            toastText.enabled = false;
        }

        private static Text Corner(Transform root, string name, Vector2 anchor, Vector2 offset, int size, TextAnchor alignment)
        {
            Text text = UiFactory.CreateText(name, root, "", size, alignment, MainColor);
            UiFactory.Place(text.rectTransform, anchor, anchor, offset, new Vector2(700f, size + 12f));
            return text;
        }
    }
}
