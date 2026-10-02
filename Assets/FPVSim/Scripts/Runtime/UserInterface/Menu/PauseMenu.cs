using System;
using System.Collections;
using FPVSim.Controls;
using FPVSim.Flight;
using FPVSim.Settings;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Controls;
using UnityEngine.InputSystem.UI;
using UnityEngine.UI;

namespace FPVSim.UserInterface
{
    /// <summary>
    /// Pause / settings menu, fully usable with the controller: D-pad or either stick to move, left/right to
    /// change values, Cross to confirm, Circle to go back, Options to resume. Built in code at runtime; uses the
    /// Input System UI module's default UI actions for navigation.
    /// </summary>
    [DisallowMultipleComponent]
    public sealed class PauseMenu : MonoBehaviour
    {
        /// <summary>Everything the menu needs from the game, passed in by the GameSession.</summary>
        public sealed class Context
        {
            public SettingsManager settings;
            public PilotInputReader input;

            /// <summary>Called when the user picks "Quit".</summary>
            public Action quit;

            /// <summary>Optional: shows a short message outside the menu (OSD toast).</summary>
            public Action<string> notify;
        }

        private static readonly FlightAxis[] Axes = { FlightAxis.Roll, FlightAxis.Pitch, FlightAxis.Yaw };

        private Context context;
        private GameObject canvasObject;
        private RectTransform viewport;
        private Text titleText;
        private Text hintText;
        private Text statusText;
        private float statusUntil;

        private MenuPage mainPage;
        private MenuPage currentPage;
        private MenuRow returnRow;
        private int lastActionFrame = -1;
        private bool rebinding;

        /// <summary>Raised after the menu closed (the GameSession resumes the game).</summary>
        public event Action Closed;

        public bool IsOpen => canvasObject != null && canvasObject.activeSelf;

        public void Initialize(Context menuContext)
        {
            context = menuContext;
            EnsureEventSystem();
            if (canvasObject == null)
            {
                Build();
            }

            if (context.settings != null)
            {
                context.settings.SettingsReplaced += OnSettingsReplaced;
            }
        }

        private void OnDestroy()
        {
            if (context != null && context.settings != null)
            {
                context.settings.SettingsReplaced -= OnSettingsReplaced;
            }
        }

        // ------------------------------------------------------------------ open / close / navigation

        public void Open()
        {
            if (IsOpen || Time.frameCount == lastActionFrame)
            {
                return;
            }

            lastActionFrame = Time.frameCount;
            EnsureEventSystem();
            canvasObject.SetActive(true);
            ShowPage(mainPage, null);
        }

        public void Close()
        {
            if (!IsOpen || rebinding || Time.frameCount == lastActionFrame)
            {
                return;
            }

            lastActionFrame = Time.frameCount;
            currentPage?.Hide();
            currentPage = null;
            canvasObject.SetActive(false);
            if (EventSystem.current != null)
            {
                EventSystem.current.SetSelectedGameObject(null);
            }

            Closed?.Invoke();
        }

        /// <summary>Circle: leave the sub page, or close the menu from the main page.</summary>
        public void Back()
        {
            if (!IsOpen || rebinding || Time.frameCount == lastActionFrame)
            {
                return;
            }

            if (currentPage != mainPage)
            {
                lastActionFrame = Time.frameCount;
                ShowPage(mainPage, returnRow);
            }
            else
            {
                Close();
            }
        }

        public void ShowHint(string hint)
        {
            if (hintText != null)
            {
                hintText.text = hint;
            }
        }

        /// <summary>Called by rows whenever they change a value.</summary>
        public void NotifySettingChanged()
        {
            context?.settings?.MarkDirty();
            currentPage?.RefreshAll();
        }

        private void ShowPage(MenuPage page, MenuRow select)
        {
            currentPage?.Hide();
            currentPage = page;
            titleText.text = page.Title.ToUpperInvariant();
            page.Show(select);
        }

        private void OpenSubPage(MenuPage page)
        {
            if (Time.frameCount == lastActionFrame)
            {
                return;
            }

            lastActionFrame = Time.frameCount;
            GameObject selected = EventSystem.current != null ? EventSystem.current.currentSelectedGameObject : null;
            returnRow = selected != null ? selected.GetComponent<MenuRow>() : null;
            ShowPage(page, null);
        }

        private void Update()
        {
            if (statusText != null && statusText.enabled && Time.unscaledTime > statusUntil)
            {
                statusText.enabled = false;
            }

            if (!IsOpen || rebinding || currentPage == null || EventSystem.current == null)
            {
                return;
            }

            // Keep something selected so the controller always works (e.g. after a mouse click on empty space).
            GameObject selected = EventSystem.current.currentSelectedGameObject;
            if (selected == null || !selected.activeInHierarchy)
            {
                MenuRow first = currentPage.FirstRow;
                if (first != null)
                {
                    EventSystem.current.SetSelectedGameObject(first.gameObject);
                }
            }
        }

        private void ShowStatus(string message)
        {
            statusText.text = message;
            statusText.enabled = true;
            statusUntil = Time.unscaledTime + 2f;
        }

        private void OnSettingsReplaced()
        {
            if (IsOpen)
            {
                currentPage?.RefreshAll();
            }
        }

        // ------------------------------------------------------------------ rebinding

        public void StartRebind(RebindRow row)
        {
            if (rebinding || row == null || row.Action == null || context?.input == null)
            {
                return;
            }

            StartCoroutine(RebindRoutine(row));
        }

        private IEnumerator RebindRoutine(RebindRow row)
        {
            rebinding = true;
            EventSystem eventSystem = EventSystem.current;
            if (eventSystem != null)
            {
                eventSystem.sendNavigationEvents = false; // the next button press belongs to the rebind
            }

            // Wait until the Cross press that started this is released, so it isn't captured.
            row.SetWaiting("release button...");
            float giveUp = Time.unscaledTime + 2f;
            while (AnyButtonHeld() && Time.unscaledTime < giveUp)
            {
                yield return null;
            }

            yield return null;
            row.SetWaiting("press a button...");
            ShowHint("Press the new button. Wait " + Mathf.RoundToInt(BindingRebinder.TimeoutSeconds) + " s to cancel.");

            bool done = false;
            bool success = false;
            BindingRebinder.Start(context.input.Actions, row.Action, ok =>
            {
                done = true;
                success = ok;
            });

            while (!done)
            {
                yield return null;
            }

            // The new button may be Cross or Circle: wait for its release before UI navigation resumes.
            giveUp = Time.unscaledTime + 2f;
            while (AnyButtonHeld() && Time.unscaledTime < giveUp)
            {
                yield return null;
            }

            yield return null;
            row.SetWaiting(null);
            if (eventSystem != null)
            {
                eventSystem.sendNavigationEvents = true;
            }

            rebinding = false;
            if (success)
            {
                context.settings?.MarkDirty();
                ShowStatus(row.Action.name + " -> " + ControlNames.ForAction(row.Action));
            }
            else
            {
                ShowStatus("Rebind cancelled");
            }

            currentPage?.RefreshAll();
            ShowHint(row.Hint);
        }

        private static bool AnyButtonHeld()
        {
            Gamepad pad = Gamepad.current;
            if (pad != null)
            {
                foreach (InputControl control in pad.allControls)
                {
                    if (control is ButtonControl button && !button.synthetic && button.isPressed)
                    {
                        return true;
                    }
                }
            }

            Keyboard keyboard = Keyboard.current;
            return keyboard != null && keyboard.anyKey.isPressed;
        }

        // ------------------------------------------------------------------ building

        private static void EnsureEventSystem()
        {
            if (EventSystem.current != null || FindFirstObjectByType<EventSystem>() != null)
            {
                return;
            }

            // The Input System UI module assigns its default UI actions (D-pad / sticks to navigate,
            // South button = Cross to submit, East button = Circle to cancel) when it is enabled.
            var eventSystemObject = new GameObject("EventSystem");
            eventSystemObject.AddComponent<EventSystem>();
            eventSystemObject.AddComponent<InputSystemUIInputModule>();
        }

        private void Build()
        {
            Canvas canvas = UiFactory.CreateCanvas("PauseMenu Canvas", transform, 100);
            canvasObject = canvas.gameObject;

            Image backdrop = UiFactory.CreateImage("Backdrop", canvas.transform, MenuStyle.Backdrop, true);
            UiFactory.Stretch(backdrop.rectTransform);

            Image panel = UiFactory.CreateImage("Panel", canvas.transform, MenuStyle.Panel, true);
            UiFactory.Place(panel.rectTransform, new Vector2(0.5f, 0.5f), new Vector2(0.5f, 0.5f), Vector2.zero,
                new Vector2(MenuStyle.PanelWidth, MenuStyle.PanelHeight));

            Image accent = UiFactory.CreateImage("AccentLine", panel.transform, MenuStyle.Accent);
            UiFactory.Place(accent.rectTransform, new Vector2(0.5f, 1f), new Vector2(0.5f, 1f), Vector2.zero,
                new Vector2(MenuStyle.PanelWidth, 4f));

            titleText = UiFactory.CreateText("Title", panel.transform, "PAUSED", MenuStyle.TitleSize, TextAnchor.MiddleLeft, MenuStyle.Text, false);
            UiFactory.Place(titleText.rectTransform, new Vector2(0f, 1f), new Vector2(0f, 1f), new Vector2(32f, -20f),
                new Vector2(MenuStyle.PanelWidth - 64f, 56f));

            Text brand = UiFactory.CreateText("Brand", panel.transform, "FPV SIM", MenuStyle.HintTextSize, TextAnchor.MiddleRight, MenuStyle.TextDim, false);
            UiFactory.Place(brand.rectTransform, new Vector2(1f, 1f), new Vector2(1f, 1f), new Vector2(-32f, -30f),
                new Vector2(300f, 40f));

            viewport = UiFactory.CreateRect("Viewport", panel.transform);
            viewport.anchorMin = new Vector2(0f, 0f);
            viewport.anchorMax = new Vector2(1f, 1f);
            viewport.offsetMin = new Vector2(24f, 110f);
            viewport.offsetMax = new Vector2(-24f, -90f);
            viewport.gameObject.AddComponent<RectMask2D>();

            hintText = UiFactory.CreateText("Hint", panel.transform, "", MenuStyle.HintTextSize, TextAnchor.MiddleLeft, MenuStyle.Accent, false);
            UiFactory.Place(hintText.rectTransform, new Vector2(0f, 0f), new Vector2(0f, 0f), new Vector2(32f, 60f),
                new Vector2(MenuStyle.PanelWidth - 64f, 36f));

            Text footer = UiFactory.CreateText("Footer", panel.transform,
                "Cross: select     Circle: back     Left / Right: adjust     Options: resume",
                MenuStyle.HintTextSize, TextAnchor.MiddleLeft, MenuStyle.TextDim, false);
            UiFactory.Place(footer.rectTransform, new Vector2(0f, 0f), new Vector2(0f, 0f), new Vector2(32f, 20f),
                new Vector2(MenuStyle.PanelWidth - 64f, 36f));

            statusText = UiFactory.CreateText("Status", panel.transform, "", MenuStyle.HintTextSize, TextAnchor.MiddleRight, MenuStyle.Accent, false);
            UiFactory.Place(statusText.rectTransform, new Vector2(1f, 0f), new Vector2(1f, 0f), new Vector2(-32f, 60f),
                new Vector2(420f, 36f));
            statusText.enabled = false;

            BuildPages();
            canvasObject.SetActive(false);
        }

        private void BuildPages()
        {
            DroneTuning t = context.settings.Tuning;
            PilotSettings p = context.settings.Pilot;

            MenuPage rates = BuildRatesPage(t, p);
            MenuPage pids = BuildPidPage(t);
            MenuPage physics = BuildPhysicsPage(t);
            MenuPage controls = BuildControlsPage(t, p);
            MenuPage camera = BuildCameraPage(p);
            MenuPage bindings = BuildBindingsPage();
            MenuPage help = BuildHelpPage(p);

            mainPage = new MenuPage(this, "Paused", viewport);
            mainPage.AddButton("Resume", Close);
            mainPage.AddButton("Rates & Angle Mode", () => OpenSubPage(rates), () => ">");
            mainPage.AddButton("PID Tuning", () => OpenSubPage(pids), () => ">");
            mainPage.AddButton("Physics", () => OpenSubPage(physics), () => ">");
            mainPage.AddButton("Controls & Throttle", () => OpenSubPage(controls), () => ">");
            mainPage.AddButton("Camera", () => OpenSubPage(camera), () => ">");
            mainPage.AddButton("Button Bindings", () => OpenSubPage(bindings), () => ">");
            mainPage.AddButton("Controller Help", () => OpenSubPage(help), () => ">");
            mainPage.AddSpacer();
            mainPage.AddButton("Save Settings", SaveSettings, null, "Cross: save to JSON now (also saved when the menu closes)");
            mainPage.AddButton("Revert to Saved", RevertSettings, null, "Cross: reload the last saved settings");
            mainPage.AddButton("Reset to Defaults", ResetSettings, null, "Cross: restore default tuning and bindings");
            mainPage.AddSpacer();
            mainPage.AddButton("Quit", () => context.quit?.Invoke(), null, "Cross: quit the simulator");
            mainPage.AddInfo("", () => "Settings file: " + context.settings.SettingsFolder);
            mainPage.FinishLayout();
        }

        private MenuPage BuildRatesPage(DroneTuning t, PilotSettings p)
        {
            var page = new MenuPage(this, "Rates & Angle Mode", viewport);
            page.AddInfo("", () =>
                "Max rate (deg/s)   Roll " + Mathf.RoundToInt(BetaflightRates.MaxRateDegPerSec(t.rollRates)) +
                "   Pitch " + Mathf.RoundToInt(BetaflightRates.MaxRateDegPerSec(t.pitchRates)) +
                "   Yaw " + Mathf.RoundToInt(BetaflightRates.MaxRateDegPerSec(t.yawRates)));
            foreach (FlightAxis axis in Axes)
            {
                FlightAxis a = axis;
                string name = a.ToString();
                page.AddSlider(name + " RC Rate", () => t.GetRates(a).rcRate, v => t.GetRates(a).rcRate = v, 0.05f, 3f, 0.01f, Fmt("0.00"));
                page.AddSlider(name + " Super Rate", () => t.GetRates(a).superRate, v => t.GetRates(a).superRate = v, 0f, 0.95f, 0.01f, Fmt("0.00"));
                page.AddSlider(name + " Expo", () => t.GetRates(a).expo, v => t.GetRates(a).expo = v, 0f, 1f, 0.01f, Fmt("0.00"));
            }

            page.AddSpacer();
            page.AddInfo("Angle mode", null, true);
            page.AddSlider("Max Angle", () => t.angleMaxDeg, v => t.angleMaxDeg = v, 10f, 80f, 1f, Fmt("0", "°"));
            page.AddSlider("Self-Level Strength", () => t.angleStrength, v => t.angleStrength = v, 1f, 20f, 0.5f, Fmt("0.0"));
            page.AddSlider("Tilt Throttle Boost", () => t.angleThrottleCompensation, v => t.angleThrottleCompensation = v, 0f, 1f, 0.05f, Percent);
            page.AddChoice("Start In", new[] { "Angle", "Acro" }, () => (int)p.startFlightMode, v => p.startFlightMode = (FlightMode)v);
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private MenuPage BuildPidPage(DroneTuning t)
        {
            var page = new MenuPage(this, "PID Tuning", viewport);
            page.AddInfo("Betaflight-like gains, scaled by the airframe's torque, so they keep working when you");
            page.AddInfo("change thrust or mass. Raise D if it bounces back after flips, P for a sharper feel.");
            foreach (FlightAxis axis in Axes)
            {
                FlightAxis a = axis;
                string name = a.ToString();
                page.AddSlider(name + " P", () => t.GetPid(a).p, v => t.GetPid(a).p = v, 0f, 200f, 1f, Fmt("0"));
                page.AddSlider(name + " I", () => t.GetPid(a).i, v => t.GetPid(a).i = v, 0f, 200f, 1f, Fmt("0"));
                page.AddSlider(name + " D", () => t.GetPid(a).d, v => t.GetPid(a).d = v, 0f, 150f, 1f, Fmt("0"));
                page.AddSlider(name + " Feedforward", () => t.GetPid(a).ff, v => t.GetPid(a).ff = v, 0f, 300f, 5f, Fmt("0"));
            }

            page.AddToggle("Airmode", () => t.airmode, v => t.airmode = v);
            page.AddSlider("D-Term Filter", () => t.dTermCutoffHz, v => t.dTermCutoffHz = v, 20f, 250f, 5f, Fmt("0", " Hz"));
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private MenuPage BuildPhysicsPage(DroneTuning t)
        {
            var page = new MenuPage(this, "Physics", viewport);
            page.AddSlider("Thrust-to-Weight", () => t.thrustToWeight, v => t.thrustToWeight = v, 1.5f, 15f, 0.1f, Fmt("0.0", " : 1"));
            page.AddSlider("Mass", () => t.massKg, v => t.massKg = v, 0.2f, 2f, 0.01f, Fmt("0.00", " kg"));
            page.AddSlider("Drag", () => t.dragMultiplier, v => t.dragMultiplier = v, 0f, 3f, 0.05f, Fmt("0.00", "x"));
            page.AddSlider("Angular Drag", () => t.angularDrag, v => t.angularDrag = v, 0f, 0.02f, 0.0005f, Fmt("0.0000"));
            page.AddSlider("Motor Spin-Up", () => t.motorSpinUpTime, v => t.motorSpinUpTime = v, 0.005f, 0.1f, 0.001f, Ms);
            page.AddSlider("Motor Spin-Down", () => t.motorSpinDownTime, v => t.motorSpinDownTime = v, 0.005f, 0.15f, 0.001f, Ms);
            page.AddSlider("Yaw Authority", () => t.yawTorqueCoefficient, v => t.yawTorqueCoefficient = v, 0.005f, 0.08f, 0.001f, Fmt("0.000"));
            page.AddToggle("Disarm On Crash", () => t.disarmOnCrash, v => t.disarmOnCrash = v);
            page.AddSlider("Crash Speed", () => t.crashSpeed, v => t.crashSpeed = v, 2f, 40f, 0.5f, Fmt("0.0", " m/s"));
            page.AddSlider("Physics Rate", () => t.physicsRateHz, v => t.physicsRateHz = Mathf.RoundToInt(v), 200f, 1000f, 50f, Fmt("0", " Hz"));
            page.AddInfo("", () => "Hover throttle: " + Mathf.RoundToInt(t.HoverThrottle * 100f) + "%   Max thrust: " +
                                   (t.thrustToWeight * t.massKg * 9.81f).ToString("0.0") + " N");
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private MenuPage BuildControlsPage(DroneTuning t, PilotSettings p)
        {
            var page = new MenuPage(this, "Controls & Throttle", viewport);
            page.AddChoice("Stick Mode", new[] { "Mode 2 (throttle left)", "Mode 1 (throttle right)" },
                () => (int)p.stickMode, v => p.stickMode = (StickMode)v);
            page.AddChoice("Throttle Mode", new[] { "Hover-centered", "Latched" },
                () => (int)p.throttleMode, v => p.throttleMode = (ThrottleMode)v);
            page.AddToggle("Auto Hover Point", () => t.autoHoverThrottle, v => t.autoHoverThrottle = v);
            page.AddSlider("Manual Hover Point", () => t.throttleMid, v => t.throttleMid = v, 0.05f, 0.8f, 0.01f, Percent);
            page.AddInfo("", () => "Hover point in use: " + Mathf.RoundToInt(t.EffectiveThrottleMid * 100f) +
                                   "% throttle at center stick" + (t.autoHoverThrottle ? " (auto)" : ""));
            page.AddSlider("Throttle Expo", () => t.throttleExpo, v => t.throttleExpo = v, 0f, 1f, 0.05f, Fmt("0.00"));
            page.AddSlider("Latched Ramp Speed", () => p.latchedThrottleRampSpeed, v => p.latchedThrottleRampSpeed = v, 0.1f, 3f, 0.05f, Fmt("0.00", " /s"));
            page.AddSpacer();
            page.AddInfo("Sticks", null, true);
            page.AddSlider("Left Deadzone", () => p.leftStick.deadzone, v => p.leftStick.deadzone = v, 0f, 0.3f, 0.01f, Fmt("0.00"));
            page.AddSlider("Right Deadzone", () => p.rightStick.deadzone, v => p.rightStick.deadzone = v, 0f, 0.3f, 0.01f, Fmt("0.00"));
            page.AddSlider("Left Expo", () => p.leftStick.expo, v => p.leftStick.expo = v, 0f, 1f, 0.01f, Fmt("0.00"));
            page.AddSlider("Right Expo", () => p.rightStick.expo, v => p.rightStick.expo = v, 0f, 1f, 0.01f, Fmt("0.00"));
            page.AddToggle("Invert Left X", () => p.leftStick.invertX, v => p.leftStick.invertX = v);
            page.AddToggle("Invert Left Y", () => p.leftStick.invertY, v => p.leftStick.invertY = v);
            page.AddToggle("Invert Right X", () => p.rightStick.invertX, v => p.rightStick.invertX = v);
            page.AddToggle("Invert Right Y", () => p.rightStick.invertY, v => p.rightStick.invertY = v);
            page.AddSlider("Smoothing (Angle)", () => p.smoothingAngle, v => p.smoothingAngle = v, 0f, 0.2f, 0.005f, MsOrOff);
            page.AddSlider("Smoothing (Acro)", () => p.smoothingAcro, v => p.smoothingAcro = v, 0f, 0.2f, 0.005f, MsOrOff);
            page.AddSpacer();
            page.AddInfo("Feedback and display", null, true);
            page.AddToggle("Rumble", () => p.rumbleEnabled, v => p.rumbleEnabled = v);
            page.AddSlider("Rumble Strength", () => p.rumbleStrength, v => p.rumbleStrength = v, 0f, 1f, 0.05f, Percent);
            page.AddToggle("Show OSD", () => p.showOsd, v => p.showOsd = v);
            page.AddToggle("Imperial Units", () => p.imperialUnits, v => p.imperialUnits = v);
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private MenuPage BuildCameraPage(PilotSettings p)
        {
            var page = new MenuPage(this, "Camera", viewport);
            page.AddSlider("FPV Uptilt", () => p.cameraUptilt, v => p.cameraUptilt = v, -10f, 70f, 1f, Fmt("0", "°"));
            page.AddSlider("FPV Field of View", () => p.cameraFov, v => p.cameraFov = v, 60f, 170f, 1f, Fmt("0", "° horizontal"));
            page.AddSlider("D-Pad Tilt Step", () => p.cameraTiltStep, v => p.cameraTiltStep = v, 1f, 15f, 1f, Fmt("0", "°"));
            page.AddChoice("Start View", new[] { "FPV", "Chase" }, () => (int)p.startView, v => p.startView = (CameraViewMode)v);
            page.AddSpacer();
            page.AddInfo("Chase camera (debug)", null, true);
            page.AddSlider("Chase Distance", () => p.chaseDistance, v => p.chaseDistance = v, 1f, 15f, 0.25f, Fmt("0.00", " m"));
            page.AddSlider("Chase Height", () => p.chaseHeight, v => p.chaseHeight = v, -2f, 6f, 0.25f, Fmt("0.00", " m"));
            page.AddSlider("Chase Field of View", () => p.chaseFov, v => p.chaseFov = v, 40f, 140f, 1f, Fmt("0", "°"));
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private MenuPage BuildBindingsPage()
        {
            var page = new MenuPage(this, "Button Bindings", viewport);
            page.AddInfo("Select an action and press Cross, then press the new button.");
            page.AddInfo("Sticks are reserved for flying. Wait " + Mathf.RoundToInt(BindingRebinder.TimeoutSeconds) +
                         " s to cancel. Duplicates are swapped.");
            InputActionAsset actions = context.input.Actions;
            AddRebind(page, actions, FpvInputActions.ToggleFlightMode, "Toggle Angle / Acro");
            AddRebind(page, actions, FpvInputActions.Respawn, "Reset Drone");
            AddRebind(page, actions, FpvInputActions.ToggleCamera, "FPV / Chase Camera");
            AddRebind(page, actions, FpvInputActions.CameraTiltUp, "Camera Tilt Up");
            AddRebind(page, actions, FpvInputActions.CameraTiltDown, "Camera Tilt Down");
            AddRebind(page, actions, FpvInputActions.Pause, "Pause Menu");
            page.AddSpacer();
            page.AddButton("Reset Bindings", () =>
            {
                BindingRebinder.ResetAll(actions);
                context.settings.MarkDirty();
                ShowStatus("Bindings reset to default");
                currentPage?.RefreshAll();
            }, null, "Cross: restore the default buttons");
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        private static void AddRebind(MenuPage page, InputActionAsset actions, string actionName, string label)
        {
            InputAction action = actions.FindAction(actionName, false);
            if (action != null)
            {
                page.AddRebind(label, action);
            }
        }

        private MenuPage BuildHelpPage(PilotSettings p)
        {
            var page = new MenuPage(this, "Controller Help", viewport);
            InputActionAsset actions = context.input.Actions;
            Func<string, string> button = actionName => ControlNames.ForAction(actions.FindAction(actionName, false));

            page.AddInfo("", () => "Flying (" + (p.stickMode == StickMode.Mode1 ? "Mode 1" : "Mode 2") + ", " +
                                   (p.throttleMode == ThrottleMode.Latched ? "latched" : "hover-centered") + " throttle)", true);
            page.AddInfo("", () => p.stickMode == StickMode.Mode1
                ? "  Left stick:  pitch (up / down), yaw (left / right)"
                : "  Left stick:  throttle (up / down), yaw (left / right)");
            page.AddInfo("", () => p.stickMode == StickMode.Mode1
                ? "  Right stick: throttle (up / down), roll (left / right)"
                : "  Right stick: pitch (up / down), roll (left / right)");
            page.AddInfo("", () => p.throttleMode == ThrottleMode.Latched
                ? "  Throttle stick ramps the throttle up / down; release to hold it."
                : "  Throttle stick centered = hover; push up to climb, pull down to descend.");
            page.AddSpacer(8f);
            page.AddInfo("Buttons", null, true);
            page.AddInfo("", () => "  " + button(FpvInputActions.ToggleFlightMode) + ": toggle Angle / Acro mode");
            page.AddInfo("", () => "  " + button(FpvInputActions.Respawn) + ": reset / respawn drone");
            page.AddInfo("", () => "  " + button(FpvInputActions.ToggleCamera) + ": FPV / chase camera");
            page.AddInfo("", () => "  " + button(FpvInputActions.CameraTiltUp) + " / " + button(FpvInputActions.CameraTiltDown) + ": camera tilt up / down");
            page.AddInfo("", () => "  " + button(FpvInputActions.Pause) + ": pause / settings menu");
            page.AddSpacer(8f);
            page.AddInfo("Menu", null, true);
            page.AddInfo("  D-pad or either stick: move    Left / Right: change value");
            page.AddInfo("  Cross: select    Circle: back    Options: resume");
            page.AddSpacer(8f);
            page.AddInfo("Keyboard (debug only)", null, true);
            page.AddInfo("  WASD = left stick, arrow keys = right stick");
            page.AddInfo("  M mode, R reset, C camera, PgUp / PgDn tilt, Esc menu");
            page.AddSpacer();
            page.AddButton("Back", Back);
            page.FinishLayout();
            return page;
        }

        // ------------------------------------------------------------------ actions

        private void SaveSettings()
        {
            bool ok = context.settings.Save();
            ShowStatus(ok ? "Settings saved" : "Save failed (see Console)");
            context.notify?.Invoke(ok ? "SETTINGS SAVED" : "SAVE FAILED");
        }

        private void RevertSettings()
        {
            context.settings.RevertToSaved();
            ShowStatus("Reverted to saved settings");
        }

        private void ResetSettings()
        {
            context.settings.ResetToDefaults();
            ShowStatus("Defaults restored (not saved yet)");
        }

        // ------------------------------------------------------------------ formatting

        private static Func<float, string> Fmt(string format, string suffix = "")
        {
            return v => v.ToString(format) + suffix;
        }

        private static string Percent(float v)
        {
            return Mathf.RoundToInt(v * 100f) + "%";
        }

        private static string Ms(float seconds)
        {
            return Mathf.RoundToInt(seconds * 1000f) + " ms";
        }

        private static string MsOrOff(float seconds)
        {
            return seconds <= 0.0001f ? "Off" : Mathf.RoundToInt(seconds * 1000f) + " ms";
        }
    }
}
