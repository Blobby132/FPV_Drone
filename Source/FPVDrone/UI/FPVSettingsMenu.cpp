#include "UI/FPVSettingsMenu.h"

#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Drone/FPVDronePawn.h"
#include "Flight/FPVAirframeModel.h"
#include "Flight/FPVFlightMath.h"
#include "Input/FPVGamepadKeys.h"
#include "Input/FPVPlayerController.h"
#include "Settings/FPVSettingsSubsystem.h"

namespace FPVMenuConstants
{
	/** Delay before a held direction starts repeating, and the repeat intervals (s). */
	inline constexpr float RepeatDelay = 0.35f;
	inline constexpr float VerticalRepeatInterval = 0.11f;
	inline constexpr float HorizontalRepeatInterval = 0.07f;
	/** Stick deflection that counts as a direction press. */
	inline constexpr float StickThreshold = 0.6f;
	/** Seconds to press a button when rebinding. */
	inline constexpr float CaptureTimeout = 6.0f;
}

namespace FPVMenuFormat
{
	FString Number(float Value, int32 Decimals)
	{
		switch (Decimals)
		{
		case 0:		return FString::Printf(TEXT("%.0f"), Value);
		case 1:		return FString::Printf(TEXT("%.1f"), Value);
		case 2:		return FString::Printf(TEXT("%.2f"), Value);
		case 3:		return FString::Printf(TEXT("%.3f"), Value);
		default:	return FString::Printf(TEXT("%.4f"), Value);
		}
	}
}

// =============================================================================================
// Setup
// =============================================================================================

void UFPVSettingsMenu::Initialize(AFPVPlayerController* InOwner)
{
	Owner = InOwner;
	if (InOwner != nullptr)
	{
		Working = InOwner->GetSettings();
	}
	BuildPages();
}

int32 UFPVSettingsMenu::AddPage(const FString& Title)
{
	FFPVMenuPage& Page = Pages.AddDefaulted_GetRef();
	Page.Title = Title;
	return Pages.Num() - 1;
}

FFPVMenuItem& UFPVSettingsMenu::AddItem(int32 Page, EFPVMenuItemType Type, const FString& Label, const FString& Description)
{
	FFPVMenuItem& Item = Pages[Page].Items.AddDefaulted_GetRef();
	Item.Type = Type;
	Item.Label = Label;
	Item.Description = Description;
	return Item;
}

void UFPVSettingsMenu::AddFloat(int32 Page, const FString& Label, float* Value, float Min, float Max, float Step, int32 Decimals,
	const FString& Unit, const FString& Description, float DisplayScale)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Float, Label, Description);
	Item.FloatValue = Value;
	Item.Min = Min;
	Item.Max = Max;
	Item.Step = Step;
	Item.Decimals = Decimals;
	Item.Unit = Unit;
	Item.DisplayScale = DisplayScale;
}

void UFPVSettingsMenu::AddInt(int32 Page, const FString& Label, int32* Value, int32 Min, int32 Max, int32 Step, const FString& Unit, const FString& Description)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Int, Label, Description);
	Item.IntValue = Value;
	Item.Min = static_cast<float>(Min);
	Item.Max = static_cast<float>(Max);
	Item.Step = static_cast<float>(FMath::Max(Step, 1));
	Item.Decimals = 0;
	Item.Unit = Unit;
}

void UFPVSettingsMenu::AddBool(int32 Page, const FString& Label, bool* Value, const FString& Description)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Bool, Label, Description);
	Item.BoolValue = Value;
}

void UFPVSettingsMenu::AddChoice(int32 Page, const FString& Label, uint8* Value, const TArray<FString>& Labels, const FString& Description)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Choice, Label, Description);
	Item.ChoiceValue = Value;
	Item.ChoiceLabels = Labels;
}

void UFPVSettingsMenu::AddPageLink(int32 Page, const FString& Label, int32 TargetPage, const FString& Description)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Page, Label, Description);
	Item.TargetPage = TargetPage;
}

void UFPVSettingsMenu::AddAction(int32 Page, const FString& Label, TFunction<void()> OnActivate, const FString& Description, TFunction<FString()> DynamicText)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Action, Label, Description);
	Item.OnActivate = MoveTemp(OnActivate);
	Item.DynamicText = MoveTemp(DynamicText);
}

void UFPVSettingsMenu::AddInfo(int32 Page, TFunction<FString()> DynamicText)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Info, FString(), FString());
	Item.DynamicText = MoveTemp(DynamicText);
}

void UFPVSettingsMenu::AddBinding(int32 Page, EFPVButtonAction Action)
{
	FFPVMenuItem& Item = AddItem(Page, EFPVMenuItemType::Binding, FPVSettingsText::GetButtonActionName(Action),
		TEXT("Press Cross, then the gamepad button to use. A button already in use swaps with this one."));
	Item.BindingAction = Action;
}

void UFPVSettingsMenu::AddRatesItems(int32 Page, const FString& AxisName, FFPVAxisRates& Rates)
{
	AddFloat(Page, AxisName + TEXT(" RC rate"), &Rates.RcRate, 0.05f, 2.55f, 0.01f, 2, FString(),
		TEXT("Overall sensitivity. Center sensitivity is about 200 x RC rate deg/s per full stick."));
	AddFloat(Page, AxisName + TEXT(" super rate"), &Rates.SuperRate, 0.0f, 0.95f, 0.01f, 2, FString(),
		TEXT("Extra rate towards full stick. Max rate = 200 x RC rate / (1 - super rate)."));
	AddFloat(Page, AxisName + TEXT(" expo"), &Rates.Expo, 0.0f, 1.0f, 0.01f, 2, FString(),
		TEXT("Softens the stick center without changing the max rate."));
}

void UFPVSettingsMenu::AddPidItems(int32 Page, const FString& AxisName, FFPVPidGains& Gains)
{
	AddFloat(Page, AxisName + TEXT(" P"), &Gains.P, 0.0f, 250.0f, 1.0f, 0, FString(),
		TEXT("Proportional: how hard the drone pushes towards the commanded rate. Too high = fast oscillation."));
	AddFloat(Page, AxisName + TEXT(" I"), &Gains.I, 0.0f, 250.0f, 1.0f, 0, FString(),
		TEXT("Integral: removes steady drift and holds the attitude against disturbances. Too high = slow wobble."));
	AddFloat(Page, AxisName + TEXT(" D"), &Gains.D, 0.0f, 250.0f, 1.0f, 0, FString(),
		TEXT("Derivative: damps overshoot (bounce-back). Too high = jittery motors."));
}

void UFPVSettingsMenu::BuildPages()
{
	Pages.Reset();

	FFPVDroneTuning& Drone = Working.Drone;
	FFPVAirframeSettings& Airframe = Drone.Airframe;
	FFPVPidProfile& Pid = Drone.Pid;
	FFPVThrottleSettings& Throttle = Drone.Throttle;
	FFPVInputSettings& Input = Working.Input;
	FFPVCameraSettings& Camera = Working.Camera;

	MainPage = AddPage(TEXT("SETTINGS"));
	const int32 RatesPage = AddPage(TEXT("SETTINGS  >  RATES (ACRO)"));
	const int32 PidPage = AddPage(TEXT("SETTINGS  >  PID"));
	const int32 AnglePage = AddPage(TEXT("SETTINGS  >  ANGLE MODE"));
	const int32 ThrottlePage = AddPage(TEXT("SETTINGS  >  THROTTLE"));
	const int32 PhysicsPage = AddPage(TEXT("SETTINGS  >  PHYSICS"));
	const int32 InputPage = AddPage(TEXT("SETTINGS  >  STICKS"));
	const int32 CameraPage = AddPage(TEXT("SETTINGS  >  CAMERA"));
	const int32 ControlsPage = AddPage(TEXT("SETTINGS  >  BUTTONS"));
	const int32 FeedbackPage = AddPage(TEXT("SETTINGS  >  RUMBLE & BATTERY"));

	// ---- Main --------------------------------------------------------------------------------
	AddAction(MainPage, TEXT("Resume"), [this]()
	{
		if (AFPVPlayerController* Controller = Owner.Get())
		{
			Controller->CloseMenu();
		}
	}, TEXT("Close the menu and keep flying."));
	AddAction(MainPage, TEXT("Flight mode"), [this]()
	{
		const AFPVPlayerController* Controller = Owner.Get();
		if (AFPVDronePawn* Pawn = Controller ? Controller->GetDrone() : nullptr)
		{
			Pawn->ToggleFlightMode();
		}
	}, TEXT("Switch the current flight mode (same as Triangle)."), [this]()
	{
		const AFPVPlayerController* Controller = Owner.Get();
		const AFPVDronePawn* Pawn = Controller ? Controller->GetDrone() : nullptr;
		return (Pawn != nullptr && Pawn->GetFlightMode() == EFPVFlightMode::Acro) ? FString(TEXT("ACRO")) : FString(TEXT("ANGLE"));
	});
	AddAction(MainPage, TEXT("Reset drone"), [this]()
	{
		if (AFPVPlayerController* Controller = Owner.Get())
		{
			Controller->RequestResetDrone();
			Controller->CloseMenu();
		}
	}, TEXT("Put the drone back on the launch pad and resume."));
	AddPageLink(MainPage, TEXT("Rates (acro)"), RatesPage, TEXT("Betaflight rates: RC rate, super rate and expo per axis."));
	AddPageLink(MainPage, TEXT("PID"), PidPage, TEXT("Rate controller gains and filters."));
	AddPageLink(MainPage, TEXT("Angle mode"), AnglePage, TEXT("Self-leveling: max tilt and leveling strength."));
	AddPageLink(MainPage, TEXT("Throttle"), ThrottlePage, TEXT("Hover-centered or latched throttle, hover point, throttle curve."));
	AddPageLink(MainPage, TEXT("Physics"), PhysicsPage, TEXT("Mass, thrust-to-weight, motors, drag, inertia."));
	AddPageLink(MainPage, TEXT("Sticks"), InputPage, TEXT("Mode 1/2, dead zones, response curves, inversion, smoothing."));
	AddPageLink(MainPage, TEXT("Camera"), CameraPage, TEXT("FPV uptilt and field of view, chase camera."));
	AddPageLink(MainPage, TEXT("Buttons"), ControlsPage, TEXT("Rebind the utility buttons."));
	AddPageLink(MainPage, TEXT("Rumble & battery"), FeedbackPage, TEXT("Crash rumble and the cosmetic battery."));
	AddAction(MainPage, TEXT("Save settings"), [this]()
	{
		UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(Owner.Get());
		if (Settings != nullptr && Settings->SaveToDisk())
		{
			ShowStatus(TEXT("Saved to ") + UFPVSettingsSubsystem::GetSettingsFilePath(), 4.0f);
		}
		else
		{
			ShowStatus(TEXT("Could not save settings (see log)."));
		}
	}, TEXT("Write the settings to Saved/FPVDrone/Settings.json (also happens automatically when the menu closes)."));
	AddAction(MainPage, TEXT("Reload saved settings"), [this]()
	{
		UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(Owner.Get());
		if (Settings != nullptr && Settings->LoadFromDisk())
		{
			Working = Settings->GetSettings();
			ShowStatus(TEXT("Settings reloaded from disk."));
		}
		else
		{
			ShowStatus(TEXT("No valid settings file to load."));
		}
	}, TEXT("Discard unsaved changes and load the settings file."));
	AddAction(MainPage, TEXT("Restore defaults"), [this]()
	{
		if (ConfirmDefaultsTimeLeft <= 0.0f)
		{
			ConfirmDefaultsTimeLeft = 3.0f;
			ShowStatus(TEXT("Press Cross again to restore ALL settings to defaults."), 3.0f);
			return;
		}
		ConfirmDefaultsTimeLeft = 0.0f;
		Working = FFPVUserSettings();
		ApplyWorking();
		ShowStatus(TEXT("Defaults restored (not saved yet)."));
	}, TEXT("Reset every setting to its default value. Asks for confirmation."));
	AddAction(MainPage, TEXT("Quit"), [this]()
	{
		if (AFPVPlayerController* Controller = Owner.Get())
		{
			if (UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(Controller))
			{
				Settings->SaveToDisk();
			}
			UKismetSystemLibrary::QuitGame(Controller, Controller, EQuitPreference::Quit, false);
		}
	}, TEXT("Save and quit (ends Play-In-Editor when running in the editor)."));

	// ---- Rates ------------------------------------------------------------------------------
	AddInfo(RatesPage, [this]()
	{
		const FFPVRateProfile& Rates = Working.Drone.Rates;
		return FString::Printf(TEXT("Max rates:  roll %.0f   pitch %.0f   yaw %.0f  deg/s"),
			FPVFlightMath::BetaflightMaxRate(Rates.Roll), FPVFlightMath::BetaflightMaxRate(Rates.Pitch), FPVFlightMath::BetaflightMaxRate(Rates.Yaw));
	});
	AddRatesItems(RatesPage, TEXT("Roll"), Drone.Rates.Roll);
	AddRatesItems(RatesPage, TEXT("Pitch"), Drone.Rates.Pitch);
	AddRatesItems(RatesPage, TEXT("Yaw"), Drone.Rates.Yaw);

	// ---- PID --------------------------------------------------------------------------------
	AddPidItems(PidPage, TEXT("Roll"), Pid.Roll);
	AddPidItems(PidPage, TEXT("Pitch"), Pid.Pitch);
	AddPidItems(PidPage, TEXT("Yaw"), Pid.Yaw);
	AddBool(PidPage, TEXT("Airmode"), &Pid.bAirMode, TEXT("Keep full control authority at zero throttle (needed for flips and dives)."));
	AddBool(PidPage, TEXT("I-term relax"), &Pid.bITermRelax, TEXT("Stop I-term build-up during fast stick moves (less bounce-back)."));
	AddFloat(PidPage, TEXT("I-term relax cutoff"), &Pid.ITermRelaxCutoffHz, 1.0f, 100.0f, 1.0f, 0, TEXT("Hz"), TEXT("Lower = relax acts on slower stick moves too."));
	AddFloat(PidPage, TEXT("I-term limit"), &Pid.ITermLimit, 0.0f, 1.0f, 0.05f, 0, TEXT("%"), TEXT("Max I-term as a share of the motor range."), 100.0f);
	AddFloat(PidPage, TEXT("I-term min throttle"), &Pid.ITermMinThrottle, 0.0f, 0.5f, 0.01f, 0, TEXT("%"), TEXT("Below this throttle the I-term bleeds off (prevents wind-up on the ground)."), 100.0f);
	AddFloat(PidPage, TEXT("D-term filter"), &Pid.DTermCutoffHz, 5.0f, 115.0f, 1.0f, 0, TEXT("Hz"), TEXT("Low-pass on the D-term. Must stay below half the physics rate (120 Hz)."));
	AddFloat(PidPage, TEXT("PID sum limit"), &Pid.PidSumLimit, 0.1f, 1.0f, 0.05f, 0, TEXT("%"), TEXT("Max roll/pitch correction as a share of the motor range."), 100.0f);
	AddFloat(PidPage, TEXT("PID sum limit (yaw)"), &Pid.PidSumLimitYaw, 0.1f, 1.0f, 0.05f, 0, TEXT("%"), TEXT("Max yaw correction as a share of the motor range."), 100.0f);

	// ---- Angle mode -------------------------------------------------------------------------
	AddChoice(AnglePage, TEXT("Start in"), reinterpret_cast<uint8*>(&Working.DefaultFlightMode), { TEXT("Angle"), TEXT("Acro") },
		TEXT("Flight mode after spawning."));
	AddFloat(AnglePage, TEXT("Max tilt"), &Drone.AngleMode.MaxTiltDeg, 5.0f, 80.0f, 1.0f, 0, TEXT("deg"), TEXT("Tilt at full right-stick deflection."));
	AddFloat(AnglePage, TEXT("Level strength"), &Drone.AngleMode.LevelStrength, 1.0f, 20.0f, 0.5f, 1, TEXT("1/s"), TEXT("How quickly the drone reaches the target tilt and levels out."));
	AddFloat(AnglePage, TEXT("Max level rate"), &Drone.AngleMode.MaxLevelRateDegPerSec, 50.0f, 1500.0f, 10.0f, 0, TEXT("deg/s"), TEXT("Rotation rate cap for the leveling loop."));
	AddFloat(AnglePage, TEXT("Yaw RC rate"), &Drone.Rates.Yaw.RcRate, 0.05f, 2.55f, 0.01f, 2, FString(), TEXT("Yaw is rate-based in Angle mode too (same as the Rates page)."));

	// ---- Throttle ---------------------------------------------------------------------------
	AddChoice(ThrottlePage, TEXT("Throttle mode"), reinterpret_cast<uint8*>(&Throttle.Mode), { TEXT("Hover-centered"), TEXT("Latched") },
		TEXT("Hover-centered: stick center = hover. Latched: up/down ramps a throttle that stays put."));
	AddInfo(ThrottlePage, [this]()
	{
		return FString::Printf(TEXT("Hover point: %.1f%% throttle"), FPVFlightMath::GetEffectiveHoverThrottle(Working.Drone) * 100.0f);
	});
	AddBool(ThrottlePage, TEXT("Automatic hover point"), &Throttle.bAutoHoverThrottle, TEXT("Compute the hover point from thrust-to-weight, idle and the throttle curve."));
	AddFloat(ThrottlePage, TEXT("Manual hover point"), &Throttle.ManualHoverThrottle, 0.05f, 0.95f, 0.01f, 0, TEXT("%"), TEXT("Throttle at stick center when the automatic hover point is off."), 100.0f);
	AddFloat(ThrottlePage, TEXT("Latched ramp speed"), &Throttle.LatchedRampPerSecond, 0.05f, 5.0f, 0.05f, 0, TEXT("%/s"), TEXT("Latched mode: throttle change per second at full stick."), 100.0f);
	AddFloat(ThrottlePage, TEXT("Throttle mid"), &Throttle.CurveMid, 0.0f, 1.0f, 0.01f, 2, FString(), TEXT("Throttle curve midpoint (Betaflight thr_mid)."));
	AddFloat(ThrottlePage, TEXT("Throttle expo"), &Throttle.CurveExpo, 0.0f, 1.0f, 0.01f, 2, FString(), TEXT("Flattens the throttle curve around the midpoint (Betaflight thr_expo)."));

	// ---- Physics ----------------------------------------------------------------------------
	AddInfo(PhysicsPage, [this]()
	{
		const FFPVAirframeSettings& A = Working.Drone.Airframe;
		const float PerMotor = FPVAirframeModel::GetMaxThrustPerMotorN(A, 9.81f);
		return FString::Printf(TEXT("Max thrust: %.1f N per motor, %.1f N total (weight %.1f N)"), PerMotor, PerMotor * 4.0f, A.MassKg * 9.81f);
	});
	AddFloat(PhysicsPage, TEXT("Thrust-to-weight"), &Airframe.ThrustToWeight, 1.2f, 15.0f, 0.1f, 1, TEXT(": 1"), TEXT("Max total thrust / weight. 4-6 is a typical 5\" freestyle quad."));
	AddFloat(PhysicsPage, TEXT("Mass"), &Airframe.MassKg, 0.1f, 5.0f, 0.01f, 2, TEXT("kg"), TEXT("All-up weight including battery."));
	AddFloat(PhysicsPage, TEXT("Arm length"), &Airframe.ArmLengthCm, 3.0f, 40.0f, 0.5f, 1, TEXT("cm"), TEXT("Frame center to motor. Longer arms = more roll/pitch authority (physics only)."));
	AddFloat(PhysicsPage, TEXT("Roll/pitch inertia"), &Airframe.RollPitchInertia, 0.0002f, 0.05f, 0.0001f, 4, TEXT("kg m2"), TEXT("Resistance to rotation. Higher = slower, heavier feel."));
	AddFloat(PhysicsPage, TEXT("Yaw inertia"), &Airframe.YawInertia, 0.0002f, 0.08f, 0.0001f, 4, TEXT("kg m2"), TEXT("Resistance to yaw rotation."));
	AddFloat(PhysicsPage, TEXT("Motor spin-up"), &Airframe.MotorSpinUpTime, 0.002f, 0.2f, 0.002f, 0, TEXT("ms"), TEXT("Motor + prop response time when speeding up."), 1000.0f);
	AddFloat(PhysicsPage, TEXT("Motor spin-down"), &Airframe.MotorSpinDownTime, 0.002f, 0.3f, 0.002f, 0, TEXT("ms"), TEXT("Motor + prop response time when slowing down."), 1000.0f);
	AddFloat(PhysicsPage, TEXT("Motor idle"), &Airframe.MotorIdle, 0.0f, 0.2f, 0.005f, 1, TEXT("%"), TEXT("Motor output at zero throttle."), 100.0f);
	AddFloat(PhysicsPage, TEXT("Thrust exponent"), &Airframe.ThrustExponent, 1.0f, 2.5f, 0.05f, 2, FString(), TEXT("1 = linear thrust (like thrust linearization), 2 = raw prop physics."));
	AddFloat(PhysicsPage, TEXT("Prop yaw torque"), &Airframe.YawTorquePerThrust, 0.0f, 0.1f, 0.001f, 3, TEXT("m"), TEXT("Prop reaction torque per newton of thrust: yaw authority."));
	AddFloat(PhysicsPage, TEXT("Linear drag"), &Airframe.LinearDrag, 0.0f, 2.0f, 0.01f, 2, TEXT("N/(m/s)"), TEXT("Drag proportional to speed."));
	AddFloat(PhysicsPage, TEXT("Quadratic drag"), &Airframe.QuadraticDrag, 0.0f, 0.2f, 0.001f, 3, TEXT("N/(m/s)2"), TEXT("Drag proportional to speed squared: sets top speed."));
	AddFloat(PhysicsPage, TEXT("Vertical drag x"), &Airframe.VerticalDragMultiplier, 0.5f, 5.0f, 0.1f, 1, FString(), TEXT("Extra drag along the drone's up axis (falling flat, punch-outs)."));
	AddFloat(PhysicsPage, TEXT("Angular drag roll/pitch"), &Airframe.AngularDragRollPitch, 0.0f, 0.05f, 0.0001f, 4, TEXT("Nm s"), TEXT("Rotation damping around roll/pitch."));
	AddFloat(PhysicsPage, TEXT("Angular drag yaw"), &Airframe.AngularDragYaw, 0.0f, 0.05f, 0.0001f, 4, TEXT("Nm s"), TEXT("Rotation damping around yaw."));

	// ---- Sticks -----------------------------------------------------------------------------
	AddChoice(InputPage, TEXT("Stick mode"), reinterpret_cast<uint8*>(&Input.StickMode), { TEXT("Mode 2 (throttle left)"), TEXT("Mode 1 (throttle right)") },
		TEXT("Mode 2: left = throttle/yaw, right = pitch/roll. Mode 1: left = pitch/yaw, right = throttle/roll."));
	AddFloat(InputPage, TEXT("Left stick dead zone"), &Input.LeftStick.Deadzone, 0.0f, 0.5f, 0.01f, 0, TEXT("%"), TEXT("Ignore small movements around center."), 100.0f);
	AddFloat(InputPage, TEXT("Left stick expo"), &Input.LeftStick.Expo, 0.0f, 1.0f, 0.05f, 2, FString(), TEXT("Softer center for finer control (applied before rates)."));
	AddBool(InputPage, TEXT("Invert left X"), &Input.LeftStick.bInvertX, TEXT("Flip the left stick's horizontal axis."));
	AddBool(InputPage, TEXT("Invert left Y"), &Input.LeftStick.bInvertY, TEXT("Flip the left stick's vertical axis."));
	AddFloat(InputPage, TEXT("Right stick dead zone"), &Input.RightStick.Deadzone, 0.0f, 0.5f, 0.01f, 0, TEXT("%"), TEXT("Ignore small movements around center."), 100.0f);
	AddFloat(InputPage, TEXT("Right stick expo"), &Input.RightStick.Expo, 0.0f, 1.0f, 0.05f, 2, FString(), TEXT("Softer center for finer control (applied before rates)."));
	AddBool(InputPage, TEXT("Invert right X"), &Input.RightStick.bInvertX, TEXT("Flip the right stick's horizontal axis."));
	AddBool(InputPage, TEXT("Invert right Y"), &Input.RightStick.bInvertY, TEXT("Flip the right stick's vertical axis."));
	AddBool(InputPage, TEXT("Smoothing in Angle"), &Input.bSmoothInAngleMode, TEXT("Low-pass the sticks in Angle mode (hides stick jitter)."));
	AddBool(InputPage, TEXT("Smoothing in Acro"), &Input.bSmoothInAcroMode, TEXT("Low-pass the sticks in Acro mode (adds latency; off by default)."));
	AddFloat(InputPage, TEXT("Smoothing time"), &Input.SmoothingTime, 0.005f, 0.3f, 0.005f, 0, TEXT("ms"), TEXT("Smoothing time constant."), 1000.0f);

	// ---- Camera -----------------------------------------------------------------------------
	AddFloat(CameraPage, TEXT("FPV camera uptilt"), &Camera.FpvUptiltDeg, -10.0f, 80.0f, 1.0f, 0, TEXT("deg"), TEXT("More uptilt for faster flying. Also D-pad up/down in flight."));
	AddFloat(CameraPage, TEXT("FPV field of view"), &Camera.FpvFovDeg, 60.0f, 170.0f, 1.0f, 0, TEXT("deg"), TEXT("Horizontal field of view of the FPV camera."));
	AddFloat(CameraPage, TEXT("D-pad tilt step"), &Camera.TiltStepDeg, 1.0f, 15.0f, 1.0f, 0, TEXT("deg"), TEXT("Uptilt change per D-pad press."));
	AddChoice(CameraPage, TEXT("Start with"), reinterpret_cast<uint8*>(&Camera.DefaultView), { TEXT("FPV camera"), TEXT("Chase camera") }, TEXT("Camera after spawning."));
	AddFloat(CameraPage, TEXT("Chase distance"), &Camera.ChaseDistanceCm, 100.0f, 1500.0f, 25.0f, 1, TEXT("m"), TEXT("Chase camera distance behind the drone."), 0.01f);
	AddFloat(CameraPage, TEXT("Chase field of view"), &Camera.ChaseFovDeg, 60.0f, 130.0f, 1.0f, 0, TEXT("deg"), TEXT("Horizontal field of view of the chase camera."));

	// ---- Buttons ----------------------------------------------------------------------------
	for (int32 Index = 0; Index < static_cast<int32>(EFPVButtonAction::Count); ++Index)
	{
		AddBinding(ControlsPage, static_cast<EFPVButtonAction>(Index));
	}
	AddAction(ControlsPage, TEXT("Restore default buttons"), [this]()
	{
		Working.Bindings = FFPVButtonBindings();
		ApplyWorking();
		ShowStatus(TEXT("Default buttons restored."));
	}, TEXT("Triangle = mode, Circle = reset, Square = camera, D-pad = tilt, Options = menu, Create = input debug."));
	AddInfo(ControlsPage, []()
	{
		return FString(TEXT("Sticks always fly the drone. Cross / Circle / D-pad always navigate this menu."));
	});

	// ---- Rumble & battery -------------------------------------------------------------------
	AddBool(FeedbackPage, TEXT("Crash rumble"), &Working.Feedback.bRumbleEnabled, TEXT("Vibrate the controller on hard impacts."));
	AddFloat(FeedbackPage, TEXT("Rumble strength"), &Working.Feedback.RumbleStrength, 0.0f, 1.0f, 0.05f, 0, TEXT("%"), TEXT("Overall vibration strength."), 100.0f);
	AddFloat(FeedbackPage, TEXT("Rumble threshold"), &Working.Feedback.ImpactThresholdMps, 0.5f, 20.0f, 0.5f, 1, TEXT("m/s"), TEXT("Impacts softer than this don't rumble."));
	AddInt(FeedbackPage, TEXT("Battery cells"), &Working.Battery.CellCount, 1, 8, 1, TEXT("S"), TEXT("LiPo cells in series (cosmetic battery on the OSD)."));
	AddFloat(FeedbackPage, TEXT("Battery capacity"), &Working.Battery.CapacityMah, 200.0f, 10000.0f, 50.0f, 0, TEXT("mAh"), TEXT("Pack capacity."));
	AddFloat(FeedbackPage, TEXT("Max current"), &Working.Battery.MaxCurrentAmps, 1.0f, 300.0f, 5.0f, 0, TEXT("A"), TEXT("Current draw at full throttle."));
	AddFloat(FeedbackPage, TEXT("Internal resistance"), &Working.Battery.InternalResistanceOhm, 0.0f, 0.5f, 0.005f, 3, TEXT("ohm"), TEXT("Voltage sag under load."));
}

// =============================================================================================
// Open / close / navigation
// =============================================================================================

void UFPVSettingsMenu::Open()
{
	if (const AFPVPlayerController* Controller = Owner.Get())
	{
		Working = Controller->GetSettings();
	}
	PageStack.Reset();
	SelectionStack.Reset();
	PushPage(MainPage);
	VerticalRepeat = FRepeatState();
	HorizontalRepeat = FRepeatState();
	bCapturing = false;
	ConfirmDefaultsTimeLeft = 0.0f;
	StatusTimeLeft = 0.0f;
	bOpen = true;
}

void UFPVSettingsMenu::Close()
{
	bOpen = false;
	bCapturing = false;
}

FFPVMenuPage& UFPVSettingsMenu::CurrentPage()
{
	return Pages[PageStack.Num() > 0 ? PageStack.Last() : MainPage];
}

const FFPVMenuPage& UFPVSettingsMenu::CurrentPage() const
{
	return Pages[PageStack.Num() > 0 ? PageStack.Last() : MainPage];
}

int32 UFPVSettingsMenu::GetSelected() const
{
	return SelectionStack.Num() > 0 ? SelectionStack.Last() : 0;
}

void UFPVSettingsMenu::SetSelected(int32 Index)
{
	if (SelectionStack.Num() > 0)
	{
		SelectionStack.Last() = Index;
	}
}

void UFPVSettingsMenu::PushPage(int32 Page)
{
	if (!Pages.IsValidIndex(Page))
	{
		return;
	}
	PageStack.Add(Page);
	// Select the first selectable row.
	int32 First = 0;
	const TArray<FFPVMenuItem>& Items = Pages[Page].Items;
	while (First < Items.Num() && !Items[First].IsSelectable())
	{
		++First;
	}
	SelectionStack.Add(FMath::Min(First, FMath::Max(Items.Num() - 1, 0)));
}

void UFPVSettingsMenu::MoveSelection(int32 Direction)
{
	const TArray<FFPVMenuItem>& Items = CurrentPage().Items;
	if (Items.Num() == 0 || Direction == 0)
	{
		return;
	}
	// Step over non-selectable rows; wrap around at the ends.
	int32 Index = GetSelected();
	for (int32 Attempt = 0; Attempt < Items.Num(); ++Attempt)
	{
		Index = (Index + Direction + Items.Num()) % Items.Num();
		if (Items[Index].IsSelectable())
		{
			SetSelected(Index);
			return;
		}
	}
}

void UFPVSettingsMenu::AdjustSelected(int32 Direction, int32 Multiplier)
{
	TArray<FFPVMenuItem>& Items = CurrentPage().Items;
	const int32 Selected = GetSelected();
	if (!Items.IsValidIndex(Selected) || Direction == 0)
	{
		return;
	}

	FFPVMenuItem& Item = Items[Selected];
	switch (Item.Type)
	{
	case EFPVMenuItemType::Float:
	{
		if (Item.FloatValue == nullptr)
		{
			return;
		}
		const float Step = FMath::Max(Item.Step, UE_KINDA_SMALL_NUMBER);
		float Value = *Item.FloatValue + static_cast<float>(Direction * Multiplier) * Step;
		// Snap to the step grid (relative to Min) so repeated steps don't drift.
		Value = Item.Min + FMath::RoundToFloat((Value - Item.Min) / Step) * Step;
		*Item.FloatValue = FMath::Clamp(Value, Item.Min, Item.Max);
		break;
	}
	case EFPVMenuItemType::Int:
	{
		if (Item.IntValue == nullptr)
		{
			return;
		}
		const int32 Step = FMath::Max(FMath::RoundToInt(Item.Step), 1);
		*Item.IntValue = FMath::Clamp(*Item.IntValue + Direction * Multiplier * Step,
			FMath::RoundToInt(Item.Min), FMath::RoundToInt(Item.Max));
		break;
	}
	case EFPVMenuItemType::Bool:
		if (Item.BoolValue == nullptr)
		{
			return;
		}
		*Item.BoolValue = !*Item.BoolValue;
		break;
	case EFPVMenuItemType::Choice:
	{
		const int32 Count = Item.ChoiceLabels.Num();
		if (Item.ChoiceValue == nullptr || Count == 0)
		{
			return;
		}
		const int32 Current = FMath::Clamp(static_cast<int32>(*Item.ChoiceValue), 0, Count - 1);
		*Item.ChoiceValue = static_cast<uint8>((Current + Direction + Count) % Count);
		break;
	}
	default:
		return;
	}

	ApplyWorking();
}

void UFPVSettingsMenu::Confirm()
{
	if (!bOpen || bCapturing)
	{
		return;
	}

	TArray<FFPVMenuItem>& Items = CurrentPage().Items;
	const int32 Selected = GetSelected();
	if (!Items.IsValidIndex(Selected))
	{
		return;
	}

	FFPVMenuItem& Item = Items[Selected];
	switch (Item.Type)
	{
	case EFPVMenuItemType::Action:
		if (Item.OnActivate)
		{
			// Copy first: the action may close the menu or rebuild state.
			const TFunction<void()> Action = Item.OnActivate;
			Action();
		}
		break;
	case EFPVMenuItemType::Page:
		PushPage(Item.TargetPage);
		break;
	case EFPVMenuItemType::Bool:
	case EFPVMenuItemType::Choice:
		AdjustSelected(1, 1);
		break;
	case EFPVMenuItemType::Binding:
		StartCapture(Item.BindingAction);
		break;
	default:
		break;
	}
}

bool UFPVSettingsMenu::Back()
{
	if (!bOpen || bCapturing)
	{
		return false;
	}
	if (PageStack.Num() > 1)
	{
		PageStack.Pop();
		SelectionStack.Pop();
		return false;
	}
	return true;
}

int32 UFPVSettingsMenu::UpdateRepeat(FRepeatState& State, int32 Direction, float DeltaSeconds, float RepeatInterval, int32& OutMultiplier)
{
	OutMultiplier = 1;
	if (Direction != State.Direction)
	{
		State.Direction = Direction;
		State.HeldTime = 0.0f;
		State.NextRepeat = FPVMenuConstants::RepeatDelay;
		return Direction != 0 ? 1 : 0;
	}
	if (Direction == 0)
	{
		return 0;
	}

	State.HeldTime += DeltaSeconds;
	State.NextRepeat -= DeltaSeconds;
	if (State.NextRepeat > 0.0f)
	{
		return 0;
	}
	State.NextRepeat += RepeatInterval;
	if (State.NextRepeat < 0.0f)
	{
		// Long frame hitch: don't fire a burst of repeats.
		State.NextRepeat = RepeatInterval;
	}
	// Holding longer changes values faster.
	OutMultiplier = State.HeldTime > 2.5f ? 10 : (State.HeldTime > 1.2f ? 4 : 1);
	return 1;
}

void UFPVSettingsMenu::Tick(float RealDeltaSeconds, const FFPVMenuInput& Input)
{
	if (!bOpen)
	{
		return;
	}

	StatusTimeLeft = FMath::Max(StatusTimeLeft - RealDeltaSeconds, 0.0f);
	ConfirmDefaultsTimeLeft = FMath::Max(ConfirmDefaultsTimeLeft - RealDeltaSeconds, 0.0f);

	if (bCapturing)
	{
		TickCapture(RealDeltaSeconds);
		return;
	}

	using namespace FPVMenuConstants;
	// Rows grow downwards, so "up" means a lower index.
	int32 Vertical = 0;
	if (Input.bUp || Input.StickY > StickThreshold)
	{
		Vertical = -1;
	}
	else if (Input.bDown || Input.StickY < -StickThreshold)
	{
		Vertical = 1;
	}

	int32 Horizontal = 0;
	if (Input.bRight || Input.StickX > StickThreshold)
	{
		Horizontal = 1;
	}
	else if (Input.bLeft || Input.StickX < -StickThreshold)
	{
		Horizontal = -1;
	}

	int32 Multiplier = 1;
	if (UpdateRepeat(VerticalRepeat, Vertical, RealDeltaSeconds, VerticalRepeatInterval, Multiplier) > 0)
	{
		MoveSelection(Vertical);
	}
	if (UpdateRepeat(HorizontalRepeat, Horizontal, RealDeltaSeconds, HorizontalRepeatInterval, Multiplier) > 0)
	{
		AdjustSelected(Horizontal, Multiplier);
	}
}

// =============================================================================================
// Applying settings
// =============================================================================================

void UFPVSettingsMenu::ApplyWorking()
{
	AFPVPlayerController* Controller = Owner.Get();
	if (Controller == nullptr)
	{
		return;
	}
	Controller->UpdateSettings(Working);
	// Pick up any clamping the settings subsystem applied.
	Working = Controller->GetSettings();
}

void UFPVSettingsMenu::ShowStatus(const FString& Message, float Seconds)
{
	StatusMessage = Message;
	StatusTimeLeft = Seconds;
}

// =============================================================================================
// Button rebinding
// =============================================================================================

void UFPVSettingsMenu::StartCapture(EFPVButtonAction Action)
{
	bCapturing = true;
	CaptureAction = Action;
	CaptureTimeLeft = FPVMenuConstants::CaptureTimeout;
	// The Cross press that started the capture is still "just pressed" this frame; skip a frame.
	CaptureArmFrames = 1;
}

void UFPVSettingsMenu::TickCapture(float RealDeltaSeconds)
{
	AFPVPlayerController* Controller = Owner.Get();
	if (Controller == nullptr)
	{
		bCapturing = false;
		return;
	}

	if (CaptureArmFrames > 0)
	{
		--CaptureArmFrames;
		return;
	}

	for (const FKey& Key : FPVGamepadKeys::GetButtons())
	{
		if (Controller->WasInputKeyJustPressed(Key))
		{
			FinishCapture(Key);
			return;
		}
	}

	CaptureTimeLeft -= RealDeltaSeconds;
	if (CaptureTimeLeft <= 0.0f || Controller->WasInputKeyJustPressed(EKeys::BackSpace) || Controller->WasInputKeyJustPressed(EKeys::Escape))
	{
		bCapturing = false;
		ShowStatus(TEXT("Rebinding cancelled."));
	}
}

void UFPVSettingsMenu::FinishCapture(const FKey& Key)
{
	bCapturing = false;

	// The menu button must never collide with the keys that navigate the menu, or the menu
	// would close the moment you press Cross/Circle.
	const bool bMenuNavigationKey = Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right
		|| Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down
		|| Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right;
	if (CaptureAction == EFPVButtonAction::OpenMenu && bMenuNavigationKey)
	{
		ShowStatus(FString::Printf(TEXT("%s is used to navigate the menu; pick another button."), *FPVGamepadKeys::GetDisplayName(Key)), 3.5f);
		return;
	}

	const FName NewKeyName = Key.GetFName();
	const FName OldKeyName = Working.Bindings.GetKeyName(CaptureAction);

	// If another action already uses this button, give it this action's old button (swap).
	for (int32 Index = 0; Index < static_cast<int32>(EFPVButtonAction::Count); ++Index)
	{
		const EFPVButtonAction Other = static_cast<EFPVButtonAction>(Index);
		if (Other != CaptureAction && Working.Bindings.GetKeyName(Other) == NewKeyName)
		{
			Working.Bindings.SetKeyName(Other, OldKeyName);
		}
	}
	Working.Bindings.SetKeyName(CaptureAction, NewKeyName);
	ApplyWorking();

	ShowStatus(FString::Printf(TEXT("%s  ->  %s"), *FPVSettingsText::GetButtonActionName(CaptureAction), *FPVGamepadKeys::GetDisplayName(Key)));
}

// =============================================================================================
// View
// =============================================================================================

FString UFPVSettingsMenu::FormatValue(const FFPVMenuItem& Item) const
{
	switch (Item.Type)
	{
	case EFPVMenuItemType::Float:
		if (Item.FloatValue == nullptr)
		{
			return FString();
		}
		return FPVMenuFormat::Number(*Item.FloatValue * Item.DisplayScale, Item.Decimals) + (Item.Unit.IsEmpty() ? FString() : TEXT(" ") + Item.Unit);
	case EFPVMenuItemType::Int:
		if (Item.IntValue == nullptr)
		{
			return FString();
		}
		return FString::Printf(TEXT("%d"), *Item.IntValue) + (Item.Unit.IsEmpty() ? FString() : TEXT(" ") + Item.Unit);
	case EFPVMenuItemType::Bool:
		return (Item.BoolValue != nullptr && *Item.BoolValue) ? TEXT("ON") : TEXT("OFF");
	case EFPVMenuItemType::Choice:
		if (Item.ChoiceValue == nullptr || Item.ChoiceLabels.Num() == 0)
		{
			return FString();
		}
		return Item.ChoiceLabels[FMath::Clamp(static_cast<int32>(*Item.ChoiceValue), 0, Item.ChoiceLabels.Num() - 1)];
	case EFPVMenuItemType::Binding:
		if (bCapturing && CaptureAction == Item.BindingAction)
		{
			return TEXT("press a button...");
		}
		return FPVGamepadKeys::GetDisplayName(Working.Bindings.GetKeyName(Item.BindingAction));
	case EFPVMenuItemType::Page:
		return TEXT(">");
	case EFPVMenuItemType::Action:
		return Item.DynamicText ? Item.DynamicText() : FString();
	case EFPVMenuItemType::Info:
	default:
		return FString();
	}
}

FString UFPVSettingsMenu::BuildHints(const FFPVMenuItem* Item) const
{
	const FString MenuButton = FPVGamepadKeys::GetDisplayName(Working.Bindings.OpenMenu);
	FString Hints = TEXT("[D-pad / left stick] Navigate   ");
	if (Item != nullptr)
	{
		switch (Item->Type)
		{
		case EFPVMenuItemType::Float:
		case EFPVMenuItemType::Int:
			Hints += TEXT("[Left / Right] Change (hold = faster)   ");
			break;
		case EFPVMenuItemType::Bool:
		case EFPVMenuItemType::Choice:
			Hints += TEXT("[Cross or Left / Right] Change   ");
			break;
		case EFPVMenuItemType::Binding:
			Hints += TEXT("[Cross] Rebind   ");
			break;
		case EFPVMenuItemType::Page:
		case EFPVMenuItemType::Action:
			Hints += TEXT("[Cross] Select   ");
			break;
		default:
			break;
		}
	}
	Hints += FString::Printf(TEXT("[Circle] Back   [%s] Resume"), *MenuButton);
	return Hints;
}

void UFPVSettingsMenu::BuildView(FFPVMenuView& OutView) const
{
	const FFPVMenuPage& Page = CurrentPage();
	const int32 Selected = GetSelected();

	OutView.Title = Page.Title;
	OutView.SelectedRow = Selected;
	OutView.Rows.Reset(Page.Items.Num());
	for (int32 Index = 0; Index < Page.Items.Num(); ++Index)
	{
		const FFPVMenuItem& Item = Page.Items[Index];
		FFPVMenuViewRow& Row = OutView.Rows.AddDefaulted_GetRef();
		Row.bSelectable = Item.IsSelectable();
		Row.bSelected = (Index == Selected);
		if (Item.Type == EFPVMenuItemType::Info)
		{
			Row.Label = Item.DynamicText ? Item.DynamicText() : FString();
		}
		else
		{
			Row.Label = Item.Label;
			Row.Value = FormatValue(Item);
		}

		const float Range = Item.Max - Item.Min;
		if (Range > 0.0f)
		{
			if (Item.Type == EFPVMenuItemType::Float && Item.FloatValue != nullptr)
			{
				Row.SliderFraction = FMath::Clamp((*Item.FloatValue - Item.Min) / Range, 0.0f, 1.0f);
			}
			else if (Item.Type == EFPVMenuItemType::Int && Item.IntValue != nullptr)
			{
				Row.SliderFraction = FMath::Clamp((static_cast<float>(*Item.IntValue) - Item.Min) / Range, 0.0f, 1.0f);
			}
		}
	}

	const FFPVMenuItem* SelectedItem = Page.Items.IsValidIndex(Selected) ? &Page.Items[Selected] : nullptr;
	OutView.Description = SelectedItem ? SelectedItem->Description : FString();
	OutView.Hints = BuildHints(SelectedItem);
	OutView.Status = StatusTimeLeft > 0.0f ? StatusMessage : FString();
	OutView.bCapturing = bCapturing;
	if (bCapturing)
	{
		OutView.CapturePrompt = FString::Printf(TEXT("Press a gamepad button for \"%s\"   (%.0f s, Backspace cancels)"),
			*FPVSettingsText::GetButtonActionName(CaptureAction), FMath::Max(CaptureTimeLeft, 0.0f));
	}
}
