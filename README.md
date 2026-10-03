# FPV Drone Simulator (Unreal Engine 5.8, C++)

A free-fly FPV quadcopter simulator written entirely in C++. There are **no binary assets**: the
drone, input actions, HUD, menu, environment and lighting are all created at runtime from code
and engine basic shapes, so the repository is plain text.

> **Status: untested.** Everything here was written without access to Unreal Engine. It has
> **not been compiled or run** yet. Expect some compile errors on first build; see
> [Needs verification](#needs-verification).

Milestone progress:

- [x] 1. Project skeleton, GameMode, drone pawn, controller input, Angle mode
- [x] 2. Acro mode, mode toggle, throttle modes
- [x] 3. Cameras, OSD, input debug overlay
- [x] 4. Runtime environment and lighting
- [ ] 5. Settings menu, JSON persistence, rumble

---

## First open

1. Install **Unreal Engine 5.8** from the Epic Games Launcher.
2. Install **Visual Studio** with the Unreal C++ toolchain, following Epic's current
   "Setting up Visual Studio for Unreal Engine" page for 5.8. That covers the *Game development
   with C++* workload, the Unreal Engine installer component, and the Windows SDK / MSVC versions
   that page lists.
3. Right-click `FPVDrone.uproject` → **Generate Visual Studio project files**.
4. Open `FPVDrone.sln`, select **Development Editor / Win64**, and build the `FPVDrone` project.
5. Open `FPVDrone.uproject`. The editor starts in `/Engine/Maps/Entry`, an empty engine map. Press
   **Play**. The game mode spawns the ground, test world, lighting and drone.

## Controller (PS5 DualSense)

Unreal on Windows reads **XInput** (Xbox-style) controllers natively. A DualSense normally has to be
presented as an XInput device before the standard `Gamepad_*` keys work. The usual ways are
**Steam Input** (with the editor/game added to Steam) or **DS4Windows**. If the drone doesn't
respond, open the input debug overlay (Create/Share button or `I`). It shows exactly which raw
axes and buttons Unreal receives.

Flying uses only the two thumbsticks, like an RC radio (Mode 2 by default):

| Stick | Mode 2 (default) | Mode 1 |
|---|---|---|
| Left stick up/down | Throttle | Pitch |
| Left stick left/right | Yaw | Yaw |
| Right stick up/down | Pitch | Throttle |
| Right stick left/right | Roll | Roll |

| Button | Action | Keyboard (debug) |
|---|---|---|
| Triangle | Toggle Angle / Acro | M |
| Circle | Reset / respawn drone | R |
| Square | Toggle FPV / chase camera | C |
| D-pad up / down | FPV camera uptilt +/- 5° | Page Up / Page Down |
| Create (Share) | Toggle input debug overlay | I |
| | Left stick / right stick | WASD / arrow keys |

Triggers and bumpers are not used for flying.

### On-screen display

- **Top left:** flight mode (ANGLE / ACRO), throttle mode, camera uptilt.
- **Top right:** battery pack voltage, per-cell voltage, % remaining, current and mAh used, flight timer.
  The battery is cosmetic: a 6S 1300 mAh LiPo that drains with motor output and sags under load.
- **Bottom:** speed in km/h and m/s (left), throttle % with a bar (center), and altitude in m (right).
  Altitude is measured from the drone's underside to the ground at the launch point.
- A center crosshair in FPV, plus "CHASE CAM" and a blinking "LOW BATTERY" warning when they apply.

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
  the D-pad; both values are also in the settings menu. The drone's own meshes are hidden in FPV.
- **Chase:** a spring-arm camera behind the drone that follows its heading with a level horizon.
  It's meant for debugging.

### Flight modes (Triangle toggles at any time)

- **Angle (default, easy).** The right stick sets a target tilt, up to *Max tilt* (45°). Center the
  stick and the drone levels itself. Yaw is rate-based.
- **Acro (realistic).** All three axes are rate-based, using Betaflight rates (RC rate, super rate,
  expo). Centered sticks command zero rotation, so the drone holds whatever attitude it is in,
  including upside down. This is how real FPV freestyle and racing quads fly.

Switching modes resets the PID integrators so the new mode starts cleanly, even mid-flight.

### Throttle modes (gamepad sticks spring back to center)

- **Hover-centered (default).** Throttle stick centered = hover. Pushing up adds thrust up to 100%;
  pulling down reduces it to motor idle. The hover point is computed from thrust-to-weight, motor
  idle and the throttle curve, so the drone holds altitude with the stick centered. You can switch it
  to a manual value in settings. Tilting the drone needs a bit more throttle to hold height, just like
  a real quad (there is no altitude hold).
- **Latched.** Up/down *ramps* a held throttle value that stays where it is when you let go, like the
  non-centering throttle stick on an RC radio. Ramp speed (default 75% per second at full
  deflection) is configurable. The held value starts at 0 and resets on respawn. Switching to latched mid-flight
  keeps the current throttle, so the drone doesn't drop.

## Test world (spawned at runtime)

`AFPVGameMode` spawns everything when you press Play, in any level:

- **Lighting** (only the parts the level is missing): sun, sky atmosphere, real-time sky light,
  exponential height fog. A level that already has a directional light keeps it, and so on.
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

Everything uses engine basic shapes with collision. The layout is deterministic (fixed random seed).
To fly in your own level, set `bSpawnTestEnvironment=False` (and optionally
`bSpawnLightingIfMissing=False`) in `Config/DefaultGame.ini`. The drone spawns at the level's
PlayerStart if there is one.

## Gameplay events (for future gates, races and timers)

`UFPVGameplayEvents` (a world subsystem) is the single place where gameplay announces things.
Nothing calls race logic directly:

| Event | Fired by | Used by (now) |
|---|---|---|
| `OnTriggerPassed(Drone, Trigger, bForward)` | `UFPVPassThroughTriggerComponent` (in every `AFPVGate`) | "GATE n" message |
| `OnDroneImpact(Drone, ImpactInfo)` | `AFPVDronePawn` hit callback | rumble (milestone 5) |
| `OnDroneReset(Drone)` | `AFPVDronePawn::ResetDrone` | (free for timers) |

A race mode can subclass `AFPVGameMode` (override `SpawnWorldContent` / `GetDroneSpawnTransform`),
collect gates by `AFPVGate::GetGateIndex()`, and subscribe to these events. The drone, the gates
and the HUD don't need to change.

## Code layout

```
FPVDrone.uproject
Config/                    DefaultEngine.ini (game mode, async physics), DefaultInput.ini, DefaultGame.ini
Source/FPVDrone/
  Core/        FPVGameMode              spawns world + drone, respawn; base for future race modes
  Drone/       FPVDronePawn             physics body, visuals, FPV camera, owns the physics bridge
  Physics/     FPVDronePhysicsBridge    thread-safe game thread <-> physics thread snapshot
               FPVDroneSimCallback      Chaos sim callback: runs the flight controller every physics step
  Flight/      FPVFlightController      Angle/Acro -> rate PID -> mixer
               FPVPidAxis, FPVQuadMixer, FPVMotorModel, FPVAirframeModel, FPVAngleController,
               FPVFlightMath (Betaflight rates, throttle curve), FPVUnits, FPVFlightTypes
  Input/       FPVPlayerController      Enhanced Input setup, sticks -> commands, buttons
               FPVInputConfig           runtime-created UInputActions / UInputMappingContexts
               FPVStickProcessor        dead zone, expo, inversion, smoothing, Mode 1/2, throttle modes
  Settings/    FPVSettingsTypes         every tunable value (USTRUCTs)
               FPVSettingsSubsystem     active settings + change notifications
  World/       FPVTestEnvironment       runtime basic-shape world (ground, hills, town, bando, trees, poles)
               FPVGate                  square / ring gate + pass-through trigger
               FPVSkyLighting           sun, sky atmosphere, sky light, fog (only what's missing)
  Gameplay/    FPVGameplayEvents        world event bus (trigger passed, impact, reset)
               FPVPassThroughTriggerComponent  detects a drone flying through a volume
  Camera/      FPVCameraRigComponent    FPV / chase camera switching, uptilt, FOV
  UI/          FPVHUD                   canvas HUD: gathers data, calls the renderers below
               FPVOsdRenderer           Betaflight-style OSD
               FPVInputDebugRenderer    raw input / telemetry overlay
               FPVHudCanvas             resolution-independent drawing helpers
  Drone/       FPVBatterySim            cosmetic LiPo model for the OSD
  Input/       FPVGamepadKeys           gamepad key list + PlayStation button names
```

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
  1 N·m = 10 000 kg·cm²/s². All conversions are in `Flight/FPVUnits.h`.

## Default tuning (typical 6S 5" freestyle quad)

| Value | Default |
|---|---|
| Mass | 0.65 kg |
| Thrust-to-weight | 5 : 1 |
| Arm length (center → motor) | 11 cm |
| Inertia roll/pitch, yaw | 0.0025, 0.0045 kg·m² |
| Motor spin-up / spin-down | 20 ms / 35 ms |
| Max tilt (Angle) | 45° |
| PID roll / pitch / yaw | 45/80/35, 47/84/37, 80/90/0 |

These defaults were checked offline with a small single-axis model of the same loop, not in Unreal:
it reaches a 600°/s roll command in about 50 ms with about 5% overshoot.

## Needs verification

These are APIs and behaviors I'm not certain are unchanged in UE 5.8. They're the first things to
check on first compile and play:

1. **Async physics settings:** `[/Script/Engine.PhysicsSettings] bTickPhysicsAsync` and
   `AsyncFixedTimeStepSize` key names and behavior.
2. **Chaos sim callback API:** `Chaos::TSimCallbackObject<Input, Output>` (default options =
   Presimulate), `FSimCallbackInput/Output`, `OnPreSimulate_Internal()`, `GetDeltaTime_Internal()`,
   `CreateAndRegisterSimCallbackObject_External<T>()`, `UnregisterAndFreeSimCallbackObject_External()`.
   Also check that `OnPreSimulate_Internal` runs every physics step.
3. **Physics-thread body accessors:** `FSingleParticlePhysicsProxy::GetPhysicsThreadAPI()`, plus
   `GetR/GetV/GetW` vs `R/V/W` (hedged with C++20 `requires` in `FPVDroneSimCallback.cpp`), and
   `AddForce/AddTorque`.
4. **`FBodyInstance::GetPhysicsActorHandle()`** returning the Chaos proxy pointer.
5. **`FBodyInstance::InertiaTensorScale` + `UpdateMassProperties()`** applying the configured inertia.
6. **Forces on the physics thread don't wake a sleeping body.** The pawn keeps the body awake from
   the game thread every tick.
7. **Enhanced Input objects created at runtime:** `UInputAction::ValueType` / `bTriggerWhenPaused`
   being public, `UInputMappingContext::MapKey` / `UnmapAll`, and
   `UEnhancedInputComponent::BindActionValue` / `GetBoundActionValue`.
8. **Default 0.25 stick dead zone removal** via `-AxisConfig` / `+AxisConfig` in `DefaultInput.ini`.
   The input debug overlay will show if a dead zone is still applied.
9. **DualSense on Windows** being visible as `Gamepad_*` keys only through Steam Input / DS4Windows
   (see above).
10. **`/Engine/BasicShapes/BasicShapeMaterial`** having a vector parameter called `Color`. If it
    doesn't, everything shows the default material color.
11. **`EngineAssociation: "5.8"`**, `BuildSettingsVersion.Latest`, `EngineIncludeOrderVersion.Latest`.
12. **`/Engine/Maps/Entry`** as the startup/default map.
13. **Lighting component setters/properties:** `UDirectionalLightComponent::SetAtmosphereSunLight`,
    `ULightComponentBase::Intensity`, `USkyLightComponent::bRealTimeCapture` (set in the
    constructor), and `UExponentialHeightFogComponent::SetFogDensity` / `SetFogHeightFalloff`.
14. **Overlap events for a fast physics body** against the gates' static trigger boxes. The triggers
    are 1.5 m deep so a fast drone overlaps them for at least one frame.
15. **Hit event `NormalImpulse`** magnitude from Chaos (used together with the pre-hit velocity
    to estimate impact speed).

## Git notes

Build output (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, IDE files) is ignored by
`.gitignore`. There are no binary assets yet, so **Git LFS is not set up**. When `.uasset` / `.umap`
files are added later, run `git lfs install` and track `*.uasset`, `*.umap` and other large
binaries *before* committing them.
