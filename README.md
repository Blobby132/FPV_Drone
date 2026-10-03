# FPV Drone Simulator (Unreal Engine 5.8, C++)

A free-fly FPV quadcopter simulator written entirely in C++. There are **no binary assets**: the
drone, input actions, HUD, menu, environment and lighting are all created at runtime from code
and engine basic shapes, so the repository is plain text.

> **Status: untested.** Everything here was written without access to Unreal Engine. It has
> **not been compiled or run** yet. Expect some compile errors on first build; see
> [Needs verification](#needs-verification).

Milestones (all written, none tested):

- [x] 1. Project skeleton, GameMode, drone pawn, controller input, Angle mode
- [x] 2. Acro mode, mode toggle, throttle modes
- [x] 3. Cameras, OSD, input debug overlay
- [x] 4. Runtime environment and lighting
- [x] 5. Settings menu, JSON persistence, button rebinding, rumble

---

## First open

1. Install **Unreal Engine 5.8** from the Epic Games Launcher.
2. Install **Visual Studio** with the Unreal C++ toolchain, following Epic's current
   "Setting up Visual Studio for Unreal Engine" page for 5.8. That covers the *Game development
   with C++* workload, the Unreal Engine installer component, and the Windows SDK / MSVC versions
   that page lists.
3. Right-click `FPVDrone.uproject` → **Generate Visual Studio project files**.
4. Open `FPVDrone.sln`, select **Development Editor / Win64**, and build the `FPVDrone` project.
   You can also just open the `.uproject` and let the editor offer to build the module.
5. Open `FPVDrone.uproject`. The editor starts in `/Engine/Maps/Entry`, an empty engine map.
   Press **Play**.

### Flying in any level

`AFPVGameMode` is the project's **global default game mode** (`Config/DefaultEngine.ini`), so it runs
in every level unless a level overrides it in World Settings. When you press Play it:

1. spawns lighting, but only the parts the level is missing (sun, sky atmosphere, sky light, fog);
2. spawns the runtime test world (unless `bSpawnTestEnvironment=False` in `Config/DefaultGame.ini`);
3. spawns the drone and possesses it, at the level's PlayerStart if one exists, otherwise on the
   launch pad at the origin.

So an empty level (**File → New Level → Empty Level**), the startup map, or your own level all work:
open it and press **Play**. To fly in your own level without the test world, set
`bSpawnTestEnvironment=False` (and optionally `bSpawnLightingIfMissing=False`) in
`Config/DefaultGame.ini`.

## Controller (PS5 DualSense)

> **Important:** Unreal on Windows reads **XInput** (Xbox-style) controllers natively. A DualSense
> normally has to be presented as an XInput device before the standard `Gamepad_*` keys work. The
> usual ways are **Steam Input** (add the editor/game to Steam, enable PlayStation configuration
> support) or **DS4Windows** (USB or Bluetooth). If the drone doesn't respond, open the **input
> debug overlay** (Create button, or `I` on the keyboard). It shows exactly which raw axes and
> buttons Unreal receives.

All flying is done with the two thumbsticks, like the gimbals on an RC radio (Mode 2 by default,
Mode 1 in the menu):

| Stick | Mode 2 (default) | Mode 1 |
|---|---|---|
| Left stick up/down | Throttle | Pitch |
| Left stick left/right | Yaw | Yaw |
| Right stick up/down | Pitch | Throttle |
| Right stick left/right | Roll | Roll |

Utility buttons (rebindable in the menu, except the stick and menu navigation):

| Button | Action | Keyboard (debug only) |
|---|---|---|
| Triangle | Toggle flight mode (Angle / Acro) | M |
| Circle | Reset / respawn drone | R |
| Square | Toggle FPV / chase camera | C |
| D-pad up / down | FPV camera uptilt +/- 5° | Page Up / Page Down |
| Options | Pause / settings menu | P |
| Create (Share) | Toggle input debug overlay | I |
| | Left stick / right stick | WASD / arrow keys |

Triggers and bumpers are not used for flying; you can assign utility actions to them in the menu.
Crash rumble uses Unreal's standard force feedback, so it works with whatever the
DualSense-to-XInput layer passes through. Adaptive triggers and gyro aren't used yet; the feedback
code lives in `UFPVControllerFeedback` so they can be added there later.

## Flying

### Flight modes (Triangle toggles at any time)

- **Angle (default, easy).** The right stick sets a target tilt, up to *Max tilt* (45°). Center the
  stick and the drone levels itself. Yaw is rate-based.
- **Acro (realistic).** All three axes are rate-based, using Betaflight rates (RC rate, super rate,
  expo). Centered sticks command zero rotation, so the drone holds whatever attitude it is in,
  including upside down. This is how real FPV freestyle and racing quads fly.

Switching modes resets the PID state so the new mode starts cleanly, even mid-flight.

### Throttle modes (gamepad sticks spring back to center)

- **Hover-centered (default).** Throttle stick centered = hover. Pushing up adds thrust up to 100%;
  pulling down reduces it to motor idle. The hover point is computed from thrust-to-weight, motor
  idle and the throttle curve, so the drone holds altitude with the stick centered. You can switch it
  to a manual value. Tilting the drone needs a bit more throttle to hold height, just like a real
  quad (there is no altitude hold).
- **Latched.** Up/down *ramps* a held throttle value that stays where it is when you let go, like the
  non-centering throttle stick on an RC radio. Ramp speed (default 75% per second at full
  deflection) is configurable. The held value starts at 0 and resets on respawn. Switching to latched mid-flight
  keeps the current throttle, so the drone doesn't drop.

## Pause / settings menu (Options)

The game pauses and the menu opens. It is fully navigable with the controller:

- **D-pad or left stick up/down:** move between rows.
- **Left/right:** change a value. Hold to repeat; holding longer changes it faster.
- **Cross:** select, toggle, or rebind. **Circle:** back (closes the menu from the main page).
- **Options:** resume.

Every change applies **immediately** to the drone, camera and input. Pages:

| Page | Contents |
|---|---|
| Rates (acro) | RC rate, super rate and expo for roll/pitch/yaw, plus the resulting max rates |
| PID | P/I/D per axis, airmode, I-term relax, I-term limit, D-term filter, PID sum limits |
| Angle mode | start-up flight mode, max tilt, level strength, max level rate, yaw rate |
| Throttle | throttle mode, automatic/manual hover point, latched ramp speed, throttle mid/expo |
| Physics | thrust-to-weight, mass, arm length, inertia, motor spin-up/down, idle, thrust exponent, prop yaw torque, drag |
| Sticks | Mode 1/2, per-stick dead zone and expo, per-axis inversion, smoothing (Angle/Acro) and its time |
| Camera | FPV uptilt and FOV, D-pad tilt step, start-up camera, chase distance and FOV |
| Buttons | rebind every utility action (press Cross, then the new button; a duplicate swaps) |
| Rumble & battery | rumble on/off, strength, threshold; battery cells, capacity, current, resistance |

The main page also has **Resume**, **Flight mode**, **Reset drone**, **Save settings**, **Reload saved settings**,
**Restore defaults** (press Cross twice to confirm), and **Quit**.

### Settings file

Settings are saved as JSON to **`<Project>/Saved/FPVDrone/Settings.json`**:

- automatically whenever the menu closes (if something changed) and when play ends;
- with **Save settings** in the menu;
- on first launch, a file with the defaults is written.

You can edit the file by hand while the game isn't running. Values are clamped to valid ranges when
loaded, and missing fields keep their defaults. Delete the file to go back to defaults. The `Saved/`
folder is git-ignored, so your personal tuning isn't committed.

## Tuning guide (flight feel)

Change one thing at a time, in this order:

1. **Controller first** (*Sticks*). Gamepad sticks are much less precise than RC gimbals.
   - Center drifts or twitches: raise the **dead zone** (0.05 → 0.08).
   - Hard to make small corrections: raise **stick expo** (0.2–0.4). Keep *smoothing* on in Angle
     mode; in Acro it adds latency.
2. **Rates** (*Rates*, Acro). These are how fast the drone rotates for a given stick position.
   - Whole stick too fast or slow: change the **RC rate**.
   - Only full deflection too fast (flips too quick): lower the **super rate**. Max rate =
     200 × RC rate / (1 − super rate). The page shows the resulting max rates.
   - Center too twitchy but full stick fine: raise **expo**.
   - A typical freestyle starting point is 600–800 °/s max on roll/pitch, 400–600 °/s on yaw.
3. **Power and weight** (*Physics*).
   - Sluggish climbs and weak punch-outs: raise **thrust-to-weight** (4 = cruiser, 5–6 = freestyle,
     8+ = racer).
   - Rotations feel heavy: lower the **inertia**, or raise **arm length** (more leverage).
   - Top speed too high or low: change **quadratic drag**.
   - Drone feels "floaty" when you chop throttle: raise **vertical drag x** a little.
4. **PID** (only if the drone itself misbehaves).
   - Fast wobble or oscillation: lower **P** or raise **D**.
   - Bounces back after a flip or roll: raise **D**, keep **I-term relax** on, maybe lower **I**.
   - Slowly drifts off attitude in Acro, or doesn't hold an angle in wind: raise **I**.
   - Yaw slow to respond: raise **yaw P**, or **prop yaw torque** (Physics).
   - Jittery motors: lower the **D-term filter** cutoff.
5. **Throttle** (*Throttle*).
   - Hover-centered too sensitive near hover: set **throttle mid** near the displayed hover point
     and add **throttle expo** (0.3–0.5). That flattens the curve around hover, and the stick
     center still hovers.
6. **Angle mode** (*Angle mode*). Lower **max tilt** (30°) for calm flying, and raise **level
   strength** for snappier self-leveling.
7. **Camera.** About 20° uptilt for slow cruising, 30–45° for fast flying. Wider FOV feels faster.

The input debug overlay is useful while tuning. It shows the rate **setpoint vs. gyro** per axis
(they should track closely), the motor outputs, and whether the mixer is **saturated** (asking for
more than the motors can give).

## On-screen display

- **Top left:** flight mode (ANGLE / ACRO), throttle mode, camera uptilt.
- **Top right:** battery pack voltage, per-cell voltage, % remaining, current and mAh used, flight timer.
  The battery is cosmetic: a 6S 1300 mAh LiPo that drains with motor output and sags under load.
- **Bottom:** speed in km/h and m/s (left), throttle % with a bar (center), and altitude in m (right).
  Altitude is measured from the drone's underside to the ground at the launch point.
- **Center:** a crosshair in FPV, plus short messages ("GATE 3", "IMPACT 12 m/s", camera tilt) and
  warnings ("CHASE CAM", a blinking "LOW BATTERY").

### Input debug overlay (Create button or `I`)

Use this to check what Unreal actually receives from the controller:

- **Stick boxes:** white dot = raw Enhanced Input value, green dot = after dead zone, curve,
  inversion and smoothing. Pushing a stick *up* must move the dot *up*.
- **Raw axes:** engine key values for both sticks and both triggers. With the sticks released,
  values should sit near 0. If they jump from 0 straight to about 0.25, the engine's default
  dead zone is still active (see *Needs verification*).
- **Buttons:** every gamepad button, lit while held, plus the last button pressed. Use this to confirm
  the DualSense face buttons, D-pad and Options/Create map to the expected keys.
- **Pilot command and flight controller:** throttle/roll/pitch/yaw sent to the drone; rate setpoint
  vs gyro per axis; the four motor outputs; mixer saturation; and the measured physics rate in Hz.
  The physics rate should read about 240 Hz.

### Cameras

- **FPV:** fixed to the frame at 25° uptilt with a 120° horizontal FOV. Change the uptilt live with
  the D-pad; both values are also in the menu. The drone's own meshes are hidden in FPV.
- **Chase:** a spring-arm camera behind the drone that follows its heading with a level horizon.
  It's meant for debugging (it can flip when the drone is upside down).

## Test world (spawned at runtime)

- **Ground:** a 2 km × 2 km slab with the engine grid material, which gives a good sense of speed.
  The launch pad is at the origin, with an arrow showing the spawn heading (+X).
- **Gates:** six gates (square and ring, numbered) in a loose loop around the pad. The first one
  is straight ahead at 25 m. Each has a pass-through trigger, and flying through shows "GATE n".
- **Town:** a 5 × 5 grid of buildings 140–300 m ahead, taller in the middle, with rooftop boxes.
- **Bando:** two towers joined by a bridge, forming a 10 m window plus a low walkway to dive under (left, about 80 m).
- **Window wall:** a 40 m wall with three 4 × 4 m windows at different heights (behind the pad).
- **Forest:** about 90 trees behind the pad (round and conifer), plus about 60 scattered elsewhere.
- **Slalom:** nine red and white 15 m poles to the right of the pad.
- **Hills:** large spheres mostly buried in the ground, so only a low dome shows. Uniform scale keeps
  sphere collision exact; Chaos doesn't support non-uniformly scaled sphere collision.
- **Lighting:** sun, sky atmosphere, real-time sky light and height fog, if the level has none.

Everything uses engine basic shapes with collision, and the layout is deterministic (fixed random
seed). Crashes are physical: the drone is a rigid body that bounces and tumbles, and Circle resets it.

## Extending: gates, races, timers

`UFPVGameplayEvents` (a world subsystem) is the single place where gameplay announces things.
Nothing calls race logic directly:

| Event | Fired by | Used by (now) |
|---|---|---|
| `OnTriggerPassed(Drone, Trigger, bForward)` | `UFPVPassThroughTriggerComponent` (in every `AFPVGate`) | "GATE n" message |
| `OnDroneImpact(Drone, ImpactInfo)` | `AFPVDronePawn` hit callback | rumble, "IMPACT" message |
| `OnDroneReset(Drone)` | `AFPVDronePawn::ResetDrone` | (free for timers) |

A race or time-trial mode can subclass `AFPVGameMode` (override `SpawnWorldContent` /
`GetDroneSpawnTransform` / `RespawnDrone`), collect gates by `AFPVGate::GetGateIndex()`, and subscribe
to these events. The drone, the gates and the HUD don't need to change.
`UFPVPassThroughTriggerComponent` can be attached to anything (finish lines, windows) to get the
same event.

## How the flight model works

- **Fixed 240 Hz physics.** `Config/DefaultEngine.ini` enables Chaos *async physics*
  (`bTickPhysicsAsync`) with a fixed `AsyncFixedTimeStepSize` of 1/240 s. The flight
  controller runs inside a Chaos **sim callback** (`FFPVDroneSimCallback::OnPreSimulate_Internal`),
  once per physics step on the physics thread. It reads the body's attitude and angular rate,
  runs the PID loop and motor model, and applies force and torque for exactly that step.
  - *Why not Tick?* Tick runs at the variable frame rate, and a force added there stays constant
    for the whole frame. A stiff rate controller then becomes unstable at low frame rates.
  - *Why not substepping with `AddCustomPhysics`?* With Chaos in UE5, that callback is not
    reliably called once per substep.
  - *Fallback:* if async physics causes trouble, set `bTickPhysicsAsync=False` and enable
    substepping (commented out in `DefaultEngine.ini`). The controller always uses the step's own
    `dt`, so it keeps working, just not at a guaranteed fixed rate.
- **Thread safety.** The game thread writes the latest stick command and tuning into
  `FFPVDronePhysicsBridge` under a mutex. The physics thread copies it out at the start of every step
  and publishes telemetry back the same way.
- **Four motors at their arm positions.** Each motor's thrust acts along the body up axis at its
  arm position: force plus `r × F` torque, plus the prop's reaction yaw torque. Motors spin up and down
  with a first-order lag. Gravity comes from Chaos (world gravity). Drag is linear plus
  quadratic, with extra drag along the up axis, plus angular drag.
- **Flight controller.** It is Betaflight-like. Angle mode turns the stick into a target tilt, and
  an outer loop turns the tilt error into a rate setpoint. Yaw is always rate-based. The per-axis rate PID uses
  D on the gyro, I-term relax and anti-windup. The quad-X mixer has airmode.
- **Units.** Flight math is in SI (kg, m, s, N, N·m). Unreal uses cm, so 1 N = 100 kg·cm/s² and
  1 N·m = 10 000 kg·cm²/s². All conversions are in `Flight/FPVUnits.h`. The OSD shows m, m/s and km/h.

### Default tuning (typical 6S 5" freestyle quad)

| Value | Default |
|---|---|
| Mass | 0.65 kg |
| Thrust-to-weight | 5 : 1 |
| Arm length (center → motor) | 11 cm |
| Inertia roll/pitch, yaw | 0.0025, 0.0045 kg·m² |
| Motor spin-up / spin-down | 20 ms / 35 ms |
| Rates roll/pitch, yaw | RC 1.00 / super 0.72 / expo 0.15 (≈714 °/s), RC 1.00 / super 0.65 / expo 0.05 (≈571 °/s) |
| Max tilt (Angle) | 45° |
| PID roll / pitch / yaw | 45/80/35, 47/84/37, 80/90/0 |
| Stick dead zone | 0.05 |
| FPV camera | 25° uptilt, 120° FOV |

These defaults were checked offline with a small model of the same control loop and motor model,
not in Unreal. It reaches a 600°/s roll command in about 50 ms with about 5% overshoot. It also
confirmed the axis signs: stick right rolls right, stick forward pitches the nose down, yaw right
turns the nose right, and Acro holds attitude.

## Code layout

```
FPVDrone.uproject
Config/   DefaultEngine.ini (game mode, async physics, rendering), DefaultInput.ini, DefaultGame.ini
Source/FPVDrone/
  Core/      FPVGameMode                     spawns lighting, world and drone; respawn; base for race modes
  Drone/     FPVDronePawn                    physics body, visuals, cameras, impacts, owns the physics bridge
             FPVBatterySim                   cosmetic LiPo model for the OSD
  Physics/   FPVDronePhysicsBridge           thread-safe game thread <-> physics thread snapshot
             FPVDroneSimCallback             Chaos sim callback: flight controller every physics step
  Flight/    FPVFlightController             Angle/Acro -> rate PID -> mixer
             FPVPidAxis, FPVQuadMixer, FPVMotorModel, FPVAirframeModel, FPVAngleController,
             FPVFlightMath (Betaflight rates, throttle curve), FPVUnits, FPVFlightTypes
  Input/     FPVPlayerController             Enhanced Input, sticks -> commands, buttons, menu, rumble
             FPVInputConfig                  runtime-created UInputActions / UInputMappingContexts
             FPVStickProcessor               dead zone, expo, inversion, smoothing, Mode 1/2, throttle modes
             FPVGamepadKeys                  gamepad key list + PlayStation button names
  Camera/    FPVCameraRigComponent           FPV / chase switching, uptilt, FOV
  UI/        FPVHUD                          canvas HUD: gathers data, calls the renderers
             FPVOsdRenderer, FPVInputDebugRenderer, FPVMenuRenderer, FPVHudCanvas
             FPVSettingsMenu                 menu pages, navigation, live editing, rebinding
  Settings/  FPVSettingsTypes                every tunable value (USTRUCTs, EditAnywhere)
             FPVSettingsSubsystem            active settings, change notifications, JSON save/load
  World/     FPVTestEnvironment              runtime basic-shape world
             FPVGate                         square / ring gate + pass-through trigger
             FPVSkyLighting                  sun, sky atmosphere, sky light, fog (only what's missing)
  Gameplay/  FPVGameplayEvents               world event bus (trigger passed, impact, reset)
             FPVPassThroughTriggerComponent  detects a drone flying through a volume
  Feedback/  FPVControllerFeedback           rumble (future: adaptive triggers, lightbar)
```

Every file-local helper lives in a uniquely named namespace, so unity builds can't collide.
No parameter or local shadows a member (UE treats shadowing as an error by default).

## Needs verification

These are APIs and behaviors I'm not certain are unchanged in UE 5.8. They're the first things to
check on first compile and play:

**Build**

1. `EngineAssociation: "5.8"`, `BuildSettingsVersion.Latest`, `EngineIncludeOrderVersion.Latest`.
2. **C++20** being the default language standard (used for `requires` in `FPVDroneSimCallback.cpp`).
3. `PublicIncludePaths.Add(ModuleDirectory)` (sub-folder includes like `"Flight/FPVUnits.h"`).

**Physics**

4. **Async physics settings:** `[/Script/Engine.PhysicsSettings] bTickPhysicsAsync` and
   `AsyncFixedTimeStepSize` key names and behavior. The debug overlay should show about 240 Hz.
5. **Chaos sim callback API:** `Chaos::TSimCallbackObject<Input, Output>` (default options =
   Presimulate), `FSimCallbackInput/Output`, `OnPreSimulate_Internal()`, `GetDeltaTime_Internal()`,
   `GetFNameForStatId()`, `CreateAndRegisterSimCallbackObject_External<T>()`,
   `UnregisterAndFreeSimCallbackObject_External()`. Also check that `OnPreSimulate_Internal` runs
   every physics step.
6. **Physics-thread body accessors:** `FSingleParticlePhysicsProxy::GetPhysicsThreadAPI()`, plus
   `GetR/GetV/GetW` vs `R/V/W` (hedged with C++20 `requires` in `FPVDroneSimCallback.cpp`), and
   `AddForce/AddTorque` on `Chaos::FRigidBodyHandle_Internal`.
7. **`FBodyInstance::GetPhysicsActorHandle()`** returning the Chaos proxy pointer.
8. **`FBodyInstance::InertiaTensorScale` + `UpdateMassProperties()`** applying the configured inertia.
9. **Forces on the physics thread don't wake a sleeping body.** The pawn keeps the body awake from
   the game thread every tick.
10. **Hit event `NormalImpulse`** magnitude from Chaos (used together with the pre-hit velocity to
    estimate impact speed).
11. **Overlap events for a fast physics body** against the gates' static trigger boxes. The triggers
    are 1.5 m deep so a fast drone overlaps them for at least one frame.

**Input**

12. **Enhanced Input objects created at runtime:** `UInputAction::ValueType` / `bTriggerWhenPaused`
    being public, `UInputMappingContext::MapKey` / `UnmapAll`, `UInputModifierNegate`, and
    `UEnhancedInputComponent::BindActionValue` / `GetBoundActionValue`, plus
    `RequestRebuildControlMappings()` after rebinding.
13. **Default 0.25 stick dead zone removal** via `-AxisConfig` / `+AxisConfig` in `DefaultInput.ini`.
    The input debug overlay will show if a dead zone is still applied.
14. **DualSense on Windows** being visible as `Gamepad_*` keys only through Steam Input / DS4Windows.
15. **Pause menu input:** `bShouldPerformFullTickWhenPaused` (PlayerTick while paused), menu actions
    with `bTriggerWhenPaused` firing while paused, and `WasInputKeyJustPressed` working while paused
    (rebinding). Swapping mapping contexts while a button is held is guarded by a short block window.

**Rendering, content and other APIs**

16. **`/Engine/BasicShapes/BasicShapeMaterial`** having a vector parameter called `Color`. If it
    doesn't, everything shows the default material color.
17. **`/Engine/Maps/Entry`** as the startup/default map.
18. **Lighting component setters/properties:** `UDirectionalLightComponent::SetAtmosphereSunLight`,
    `ULightComponentBase::Intensity`, `USkyLightComponent::bRealTimeCapture` (set in the
    constructor), and `UExponentialHeightFogComponent::SetFogDensity` / `SetFogHeightFalloff`.
19. **Canvas HUD:** `AHUD::DrawText/DrawRect/DrawLine/GetTextSize` and `GEngine->Get{Small,Medium,Large}Font()`.
20. **`FJsonObjectConverter::UStructToJsonObjectString` / `JsonObjectStringToUStruct`** signatures.
21. **`APlayerController::PlayDynamicForceFeedback`**, the native (non-latent) 6-argument overload.

## Git notes

Build output (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, IDE files) is ignored by
`.gitignore`. There are no binary assets yet, so **Git LFS is not set up**. When `.uasset` / `.umap`
files are added later, run `git lfs install` and track `*.uasset`, `*.umap` and other large
binaries *before* committing them.
