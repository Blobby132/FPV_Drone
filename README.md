# FPV Drone Simulator (Unity 6 LTS, URP)

A free-fly FPV quadcopter simulator built around a **PS5 DualSense** controller. The physics is a 4-motor
quad model with a Betaflight-style flight controller (rates, rate PID, airmode). Two flight modes: **Angle**
(self-levelling) and **Acro** (rate). There's an FPV camera with uptilt, a minimal OSD, a test map and a
settings menu you can drive entirely with the controller.

> **Status: untested.** All code was written without opening Unity. It has been compiled against Unity
> reference assemblies and the real Input System source outside Unity (see *How the code was checked*), but
> it has **not** been compiled or run inside the Unity editor yet. Expect a round of fixes on first open.

---

## First time opening in Unity: checklist

1. **Install Unity 6 LTS** with Unity Hub (6000.0.x LTS or 6000.3.x LTS). Add the *Windows Build Support*
   module if you want to make builds.
2. **Add the project:** Unity Hub → *Add* → *Add project from disk* → select this repository folder → pick
   your Unity 6 editor version.
3. **First import** takes a few minutes. Unity creates `Library/`, `ProjectSettings/` and a `.meta` file for
   every asset. Packages from `Packages/manifest.json` are installed: Input System 1.14.2, URP and uGUI.
   URP and uGUI ship with the editor, so if Package Manager reports a different version for them, accept the
   editor's version.
4. **Input System prompt:** Unity asks whether to enable the new input backends. Click **Yes**; the editor
   restarts. If you missed it: *Edit → Project Settings → Player → Other Settings → Active Input Handling* →
   *Input System Package (New)* (or *Both*), then restart.
5. **Check the Console for compile errors.** If there are any, stop here and share them so they can be fixed.
6. Run **FPV Sim → Build Test Scene** from the top menu bar. It:
   - creates and assigns a URP pipeline asset (`Assets/FPVSim/Settings/Rendering/`), switches to linear
     color space and sets the physics timestep to 0.002 s (500 Hz). This step is also available on its own
     as *FPV Sim → Setup Project*.
   - creates the default settings assets `Assets/FPVSim/Settings/DroneTuning.asset` and `PilotSettings.asset`
   - generates the drone prefab, materials, meshes and textures under `Assets/FPVSim/Generated/`
   - builds and saves `Assets/FPVSim/Generated/Scenes/FPVTestScene.unity` and adds it to Build Settings.
7. **Connect the DualSense** (USB is the most reliable; Bluetooth works with recent Input System versions)
   and press **Play**.
8. **Commit** the new `.meta` files, `ProjectSettings/`, `Packages/packages-lock.json` and
   `Assets/FPVSim/Settings/`. The `Generated/` folder can be committed or ignored; it is fully reproducible
   (see the commented-out line in `.gitignore`).

**If something looks wrong:**
- *Everything is pink:* URP isn't active. Run *FPV Sim → Setup Project* and check *Project Settings →
  Graphics → Default Render Pipeline* (and *Quality* → *Render Pipeline Asset*). If the automatic URP asset
  creation fails, create one with *Assets → Create → Rendering → URP Asset (with Universal Renderer)* and
  assign it there.
- *Controller does nothing:* check Active Input Handling (step 4) and *Window → Analysis → Input Debugger*
  to confirm the DualSense shows up. Steam (Big Picture / Steam Input) can grab the controller; close it.
- *The drone is jittery or unstable:* make sure physics runs at 500 Hz (the GameSession sets
  `Time.fixedDeltaTime` from *Physics Rate* in the tuning). The rate PID needs a high loop rate.

---

## Controller layout (PS5 DualSense)

Flying uses **only the two thumbsticks**, like an RC transmitter. Mode 2 is the default; Mode 1 is in
*Controls & Throttle*.

| Input | Mode 2 (default) | Mode 1 |
|---|---|---|
| Left stick up/down | Throttle | Pitch |
| Left stick left/right | Yaw | Yaw |
| Right stick up/down | Pitch | Throttle |
| Right stick left/right | Roll | Roll |

| Button | Action |
|---|---|
| Triangle | Toggle flight mode (Angle / Acro) |
| Circle | Reset / respawn the drone |
| Square | Toggle FPV / chase camera |
| D-pad up / down | FPV camera uptilt +/- (5° steps by default) |
| Options | Pause / settings menu |

Triggers and bumpers are not used for flying. Every button above can be rebound in *Button Bindings*.

**Throttle on a self-centering stick:** pick a mode in *Controls & Throttle → Throttle Mode*.
- **Hover-centered (default):** stick centered = hover throttle. Push up to climb, pull down to descend. The
  hover point is computed from the thrust-to-weight ratio (*Auto Hover Point*); you can also set it by hand.
- **Latched:** stick up/down raises or lowers a held throttle value that stays where it is when you let go.
  *Latched Ramp Speed* controls how fast it changes.

**In the menu:** D-pad or either stick moves, left/right changes values (hold to speed up), Cross confirms,
Circle goes back, Options resumes. Settings are saved automatically when the menu closes.

**Keyboard (debug only):** WASD = left stick, arrow keys = right stick, M = mode, R = reset, C = camera,
PgUp / PgDn = camera tilt, Esc = menu.

---

## Tuning the flight feel

All values live in two ScriptableObjects:
- `Assets/FPVSim/Settings/DroneTuning.asset`: airframe, motors, drag, rates, PIDs, throttle curve, crashes,
  physics rate, cosmetic battery.
- `Assets/FPVSim/Settings/PilotSettings.asset`: stick mode, deadzones, expo, inversion, throttle mode,
  smoothing, camera, rumble, OSD.

These assets are your **defaults**. At runtime the game works on copies, which you edit in the pause menu
or in the Inspector during Play mode: select *GameSession*, then double-click *Tuning* / *Pilot* under
*Runtime copies* on the SettingsManager. Changes apply immediately. To keep values you tuned in Play mode as
the new defaults, use the SettingsManager's context menu (⋮) → *Write runtime values to default assets*. The copies are saved as JSON
in `Application.persistentDataPath/FPVSim/` (`drone_tuning.json`, `pilot_settings.json`,
`input_bindings.json`); the menu's main page shows the exact folder. *Reset to Defaults* goes back to the
assets; deleting the JSON files does the same permanently.

### Rates (Acro feel), *Rates & Angle Mode* page
Betaflight "Betaflight rates": `rate = 200 · RC Rate · stick' / (1 − |stick| · Super Rate)` deg/s, where
`stick'` is the stick after expo. The page shows the resulting max rate per axis.
- **RC Rate** scales the whole curve. **Super Rate** adds rate near the end of the stick throw (it sets the
  max rate without making center stick twitchy). **Expo** softens the center.
- Defaults: 1.00 / 0.70 / 0.00 = 667 deg/s, a typical 5" freestyle setup. Thumbsticks are short, so if center
  feels twitchy, add stick expo first (*Controls → Right Expo*, default 0.20), then rate expo.

### Angle mode
- **Max Angle** (default 45°) is the tilt at full stick. **Self-Level Strength** controls how hard it returns
  to level. **Tilt Throttle Boost** (on by default) adds thrust while tilted so centered throttle roughly holds
  altitude; set it to 0 for realistic behaviour.

### PIDs, *PID Tuning* page
The numbers look like Betaflight's (P 45 / I 80 / D 30 / FF 120). Internally they're scaled by the
airframe's inertia and torque, so they keep working when you change thrust, mass or arm length.
- **P**: sharpness / how tightly it tracks the sticks. Too high gives fast oscillation after moves.
- **D**: damping. Raise it if the quad *bounces back* after a flip or roll stops.
- **I**: holds the attitude against slow drift. Rarely needs changing.
- **Feedforward**: snappier response to fast stick moves. Lower it if quick flicks overshoot.
- **Airmode** keeps full control at zero throttle (needed for flips and dives). Turn it off to make the quad
  go limp at zero throttle.

### Physics
- **Thrust-to-Weight** (default 5:1): punch and top speed. 4:1 is mellow, 8:1+ is a racer.
- **Mass**: heavier feels floatier and carries more momentum (inertia scales with the tuning values).
- **Drag**: multiplier on all linear drag. Lower means faster top speed and longer glides.
- **Motor Spin-Up / Spin-Down**: motor lag. Longer feels "heavier" and laggier, and needs more D.
- **Disarm On Crash / Crash Speed**: impacts faster than this cut the motors until you press Circle.
- **Physics Rate**: keep 500 Hz or higher. Lower rates make the rate loop unstable.

### Controls
- **Deadzone** (default 0.05 per stick, per axis): raise it if the drone drifts with sticks centered.
- **Expo** per stick: more expo means finer control around center.
- **Smoothing**: a low-pass on the sticks. On in Angle mode (30 ms), off in Acro (adds latency).
- **Invert** toggles exist for each stick axis.

### Camera
- **FPV Uptilt** (default 25°): fly faster with more uptilt. The D-pad changes it live.
- **FPV Field of View** (default 120°) is **horizontal**, like an FPV lens spec. It's converted to Unity's
  vertical FOV for your screen aspect.

---

## Project layout

```
Assets/FPVSim/
  Scripts/Runtime/            asmdef FPVSim.Runtime (Input System + uGUI)
    Core/                     GameSession (composition root), IGameMode + FreeFlyMode, GameplayEvents,
                              PassThroughTrigger, SpawnPoint
    Controls/                 FpvInputActions (bindings in code), PilotInputReader, PilotCommandSource,
                              StickShaping, ControlNames, BindingRebinder
    Flight/                   DroneController, FlightController, AngleController, RateController/PidAxis,
                              BetaflightRates, QuadMixer, MotorModel, QuadAirframe, ThrottleCurve,
                              BatterySimulator, DroneTuning, BodyAxes
    Cameras/                  CameraRig (FPV + chase)
    UserInterface/            OsdView, UiFactory, Menu/ (PauseMenu, MenuPage, row widgets)
    Settings/                 SettingsManager (runtime copies + JSON), PilotSettings
    Feedback/                 RumbleFeedback, IHapticsOutput, GamepadHaptics
    World/                    ProceduralMeshes, PassFeedback
  Scripts/Editor/             asmdef FPVSim.EditorTools: ProjectSetup, TestSceneBuilder, DroneBuilder,
                              World/ (terrain, environment, lighting builders)
  Settings/                   default DroneTuning / PilotSettings + URP assets (created by the builder)
  Generated/                  scene, prefab, materials, meshes, textures (rebuilt by the builder)
Packages/manifest.json
```

### How a physics step works
`DroneController.FixedUpdate` runs everything in a fixed order at 500 Hz:

1. **Pilot command** (`PilotCommandSource`): raw sticks → invert → deadzone → expo → Mode 2/1 mapping →
   smoothing → hover-centered or latched throttle.
2. **Flight controller** (`FlightController`): throttle curve (mid point = hover), setpoint (Acro: Betaflight
   rates; Angle: attitude error → rate), then a **rate PID** per axis (D on gyro, filtered feedforward). Its
   output is an angular acceleration, turned into mixer commands with the airframe's inertia and torque.
3. **Mixer** (`QuadMixer`): Betaflight Quad-X table with airmode desaturation.
4. **Motors** (`MotorModel`): per-motor first-order spool-up/down lag plus idle.
5. **Forces**: each motor's thrust is applied at its arm position (`AddForceAtPosition`), so roll and pitch
   torque come from real lever arms. Yaw comes from prop drag torque. Quadratic body drag, linear rotor drag
   and angular drag are applied explicitly; gravity comes from the Rigidbody.

Why 4 motors and not a simplified torque model: per-motor thrust, saturation and lag give the real
quad behaviours, such as airmode, losing authority at full throttle, yaw being weaker than roll, and motor
lag causing bounce-back, without faking them.

### Adding races and gates later
- `PassThroughTrigger` (already on every ring and gate) raises `GameplayEvents.DronePassedTrigger` with the
  pass direction. A gate just needs `TriggerId` / `Order`.
- A race is a new `IGameMode` (see `FreeFlyMode`): subscribe to the pass event, track the next expected
  `Order`, run a timer, and respawn at the last checkpoint in `OnRespawnRequested`. Assign it in
  `GameSession → References → Game Mode`.
- `IPilotCommandSource` lets something other than the gamepad fly (replays, ghosts, AI).
- `IHapticsOutput` is where DualSense adaptive-trigger effects can be added later.

---

## How the code was checked (no Unity yet)

- Compiled with the .NET SDK at **C# 9** (Unity 6's language version) against Unity 2021.3 reference
  assemblies, the **real Input System 1.14.2 source** compiled from the package, uGUI reference assemblies and
  small URP stubs checked against the URP 17 API docs. The few Unity 6-only renames (`Rigidbody.linearVelocity`,
  `linearDamping`, `angularDamping`, `PhysicsMaterial`) were checked against the Unity 6 docs.
  This found and fixed one real error (a class name clashing with `UnityEditor.SettingsService`).
- The rate loop (PID, motor lag, mixer saturation at 500 Hz) was simulated offline to pick the internal gain
  scaling. With the defaults, a fast 400 deg/s stick flick reaches 90% in ~70 ms with ~3% overshoot.
- Procedural mesh winding was verified numerically.

None of this replaces running it in Unity. Expect a short round of fixes on first open.

## Known limitations / ideas for later
- The battery is cosmetic and doesn't reduce thrust. There's no prop wash, ground effect or wind yet.
- The chase camera is a simple follow cam for debugging.
- The DualSense adaptive triggers, gyro and touchpad are not used.
- No gates, races or timers yet; the hooks described above are in place.
