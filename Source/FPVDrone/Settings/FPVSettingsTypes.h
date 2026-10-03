// All user-tunable values of the simulator. Every struct here is serialized to the JSON settings
// file (see UFPVSettingsSubsystem) and edited live from the pause menu (see UFPVSettingsMenu).
//
// Unit conventions used throughout the project:
//   * Flight math (thrust, drag, inertia, torque) is done in SI: kg, m, s, N, N*m, rad/s.
//   * Unreal positions/velocities are cm and cm/s. Conversions live in Flight/FPVUnits.h.
//   * Angular rates shown to the pilot (rates, setpoints) are deg/s, like Betaflight.

#pragma once

#include "CoreMinimal.h"
#include "FPVSettingsTypes.generated.h"

// ---------------------------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------------------------

/** Flight controller mode. */
UENUM(BlueprintType)
enum class EFPVFlightMode : uint8
{
	/** Self-leveling: right stick commands a tilt angle. */
	Angle	UMETA(DisplayName = "Angle"),
	/** Rate mode: sticks command rotation rates, no self-leveling. */
	Acro	UMETA(DisplayName = "Acro")
};

/** How a spring-loaded gamepad stick is turned into a throttle value. */
UENUM(BlueprintType)
enum class EFPVThrottleMode : uint8
{
	/** Stick centered = hover throttle. Up adds thrust, down removes it. */
	HoverCentered	UMETA(DisplayName = "Hover-centered"),
	/** Stick up/down ramps a held throttle that stays put when the stick is released. */
	Latched			UMETA(DisplayName = "Latched")
};

/** RC transmitter stick layout. */
UENUM(BlueprintType)
enum class EFPVStickMode : uint8
{
	/** Left stick: throttle + yaw. Right stick: pitch + roll. */
	Mode2	UMETA(DisplayName = "Mode 2"),
	/** Left stick: pitch + yaw. Right stick: throttle + roll. */
	Mode1	UMETA(DisplayName = "Mode 1")
};

/** Active camera. */
UENUM(BlueprintType)
enum class EFPVCameraView : uint8
{
	FPV		UMETA(DisplayName = "FPV"),
	Chase	UMETA(DisplayName = "Chase")
};

/** Utility actions that can be rebound to gamepad buttons. */
UENUM(BlueprintType)
enum class EFPVButtonAction : uint8
{
	ToggleFlightMode,
	ResetDrone,
	ToggleCamera,
	CameraTiltUp,
	CameraTiltDown,
	OpenMenu,
	ToggleInputDebug,
	Count	UMETA(Hidden)
};

// ---------------------------------------------------------------------------------------------
// Rates (Betaflight "Betaflight" rate type)
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVAxisRates
{
	GENERATED_BODY()

	/** Betaflight RC Rate. Center sensitivity is about 200 * RcRate deg/s per full stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates", meta = (ClampMin = "0.05", ClampMax = "2.55"))
	float RcRate = 1.0f;

	/** Betaflight Super Rate. Boosts the rate towards full stick. Max rate = 200 * RcRate / (1 - SuperRate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float SuperRate = 0.7f;

	/** Betaflight RC Expo. Softens the stick center without changing the max rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Expo = 0.0f;

	FFPVAxisRates() = default;
	FFPVAxisRates(float InRcRate, float InSuperRate, float InExpo)
		: RcRate(InRcRate), SuperRate(InSuperRate), Expo(InExpo)
	{
	}
};

/** Acro rates per axis. Defaults feel like a typical 5" freestyle quad (~710 deg/s roll/pitch). */
USTRUCT(BlueprintType)
struct FFPVRateProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates")
	FFPVAxisRates Roll = FFPVAxisRates(1.0f, 0.72f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates")
	FFPVAxisRates Pitch = FFPVAxisRates(1.0f, 0.72f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rates")
	FFPVAxisRates Yaw = FFPVAxisRates(1.0f, 0.65f, 0.05f);
};

// ---------------------------------------------------------------------------------------------
// PID (rate controller)
// ---------------------------------------------------------------------------------------------

/**
 * Per-axis gains. They use Betaflight's internal term scaling so the numbers have familiar
 * magnitudes, but the simulated airframe is not a real quad, so do not expect a 1:1 match with
 * a Betaflight tune.
 */
USTRUCT(BlueprintType)
struct FFPVPidGains
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.0", ClampMax = "250.0"))
	float P = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.0", ClampMax = "250.0"))
	float I = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.0", ClampMax = "250.0"))
	float D = 35.0f;

	FFPVPidGains() = default;
	FFPVPidGains(float InP, float InI, float InD)
		: P(InP), I(InI), D(InD)
	{
	}
};

USTRUCT(BlueprintType)
struct FFPVPidProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FFPVPidGains Roll = FFPVPidGains(45.0f, 80.0f, 35.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FFPVPidGains Pitch = FFPVPidGains(47.0f, 84.0f, 37.0f);

	/** Yaw has much less physical authority (prop drag torque), hence the higher P and no D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	FFPVPidGains Yaw = FFPVPidGains(80.0f, 90.0f, 0.0f);

	/** Max |I-term| as a fraction of the full motor range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ITermLimit = 0.3f;

	/** I-term relax: suppresses I build-up during fast stick moves (prevents bounce-back after flips). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bITermRelax = true;

	/** I-term relax setpoint filter cutoff (Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float ITermRelaxCutoffHz = 15.0f;

	/** Below this throttle (fraction of motor range, after the throttle curve) the I-term bleeds off,
	 *  so it can't wind up while the drone sits on the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ITermMinThrottle = 0.08f;

	/** D-term low-pass cutoff (Hz). Keep it below half the physics rate (240 Hz -> below 120 Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "5.0", ClampMax = "115.0"))
	float DTermCutoffHz = 80.0f;

	/** Max |PID sum| on roll/pitch as a fraction of motor range (Betaflight pidsum_limit / 1000). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PidSumLimit = 0.5f;

	/** Max |PID sum| on yaw as a fraction of motor range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PidSumLimitYaw = 0.4f;

	/** Airmode: keep full PID authority at zero throttle by shifting the throttle to make room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID")
	bool bAirMode = true;
};

// ---------------------------------------------------------------------------------------------
// Angle (self-level) mode
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVAngleModeSettings
{
	GENERATED_BODY()

	/** Tilt at full right-stick deflection (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle Mode", meta = (ClampMin = "5.0", ClampMax = "80.0"))
	float MaxTiltDeg = 45.0f;

	/** How hard the drone pulls towards the target tilt: deg/s of rotation per degree of error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle Mode", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float LevelStrength = 8.0f;

	/** Cap on the rotation rate the leveling loop may command (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle Mode", meta = (ClampMin = "50.0", ClampMax = "1500.0"))
	float MaxLevelRateDegPerSec = 500.0f;
};

// ---------------------------------------------------------------------------------------------
// Airframe / motors / aerodynamics
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVAirframeSettings
{
	GENERATED_BODY()

	/** All-up weight in kg (6S 5" freestyle quad with battery: ~0.65 kg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airframe", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MassKg = 0.65f;

	/** Max total thrust divided by weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airframe", meta = (ClampMin = "1.2", ClampMax = "15.0"))
	float ThrustToWeight = 5.0f;

	/** Frame center to motor (cm). 5" freestyle frame: ~11 cm (220 mm motor-to-motor diagonal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airframe", meta = (ClampMin = "3.0", ClampMax = "40.0"))
	float ArmLengthCm = 11.0f;

	/** Roll/pitch moment of inertia (kg*m^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airframe", meta = (ClampMin = "0.0002", ClampMax = "0.05"))
	float RollPitchInertia = 0.0025f;

	/** Yaw moment of inertia (kg*m^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Airframe", meta = (ClampMin = "0.0002", ClampMax = "0.08"))
	float YawInertia = 0.0045f;

	/** Motor + prop spin-up time constant (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motors", meta = (ClampMin = "0.002", ClampMax = "0.2"))
	float MotorSpinUpTime = 0.02f;

	/** Motor + prop spin-down time constant (s). Props slow down slower than they speed up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motors", meta = (ClampMin = "0.002", ClampMax = "0.3"))
	float MotorSpinDownTime = 0.035f;

	/** Motor output at zero throttle (fraction, like Betaflight's dshot_idle_value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motors", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float MotorIdle = 0.05f;

	/** Thrust = MaxThrust * output^Exponent. 1 = linear (like Betaflight thrust linearization), 2 = raw prop physics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motors", meta = (ClampMin = "1.0", ClampMax = "2.5"))
	float ThrustExponent = 1.0f;

	/** Prop reaction (yaw) torque per newton of thrust (m). Real 5" props ~0.01-0.02; a bit more feels better on a gamepad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motors", meta = (ClampMin = "0.0", ClampMax = "0.1"))
	float YawTorquePerThrust = 0.03f;

	/** Linear air drag (N per m/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float LinearDrag = 0.05f;

	/** Quadratic air drag (N per (m/s)^2). Mostly sets the top speed (~150 km/h at default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float QuadraticDrag = 0.012f;

	/** Extra drag along the drone's up axis (flat frame and prop discs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float VerticalDragMultiplier = 1.3f;

	/** Angular drag around roll/pitch (N*m per rad/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0", ClampMax = "0.05"))
	float AngularDragRollPitch = 0.0005f;

	/** Angular drag around yaw (N*m per rad/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0", ClampMax = "0.05"))
	float AngularDragYaw = 0.0008f;
};

// ---------------------------------------------------------------------------------------------
// Throttle
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVThrottleSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle")
	EFPVThrottleMode Mode = EFPVThrottleMode::HoverCentered;

	/** Hover-centered mode: compute the hover point from thrust-to-weight, motor idle and the throttle curve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle")
	bool bAutoHoverThrottle = true;

	/** Hover-centered mode: throttle (0-1) at stick center when bAutoHoverThrottle is off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float ManualHoverThrottle = 0.3f;

	/** Latched mode: throttle change per second at full stick deflection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float LatchedRampPerSecond = 0.75f;

	/** Throttle curve midpoint (Betaflight thr_mid, 0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurveMid = 0.5f;

	/** Throttle curve expo around the midpoint (Betaflight thr_expo, 0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurveExpo = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Complete drone tuning (everything the flight controller / physics model needs)
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVDroneTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	FFPVRateProfile Rates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	FFPVPidProfile Pid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	FFPVAngleModeSettings AngleMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	FFPVAirframeSettings Airframe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
	FFPVThrottleSettings Throttle;
};

// ---------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------

/** Settings for one physical thumbstick. */
USTRUCT(BlueprintType)
struct FFPVStickSettings
{
	GENERATED_BODY()

	/** Per-axis dead zone (fraction of travel). The remaining travel is rescaled to the full range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Deadzone = 0.05f;

	/** Stick response curve: 0 = linear, 1 = very soft center (cubic). Applied before rates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Expo = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bInvertX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bInvertY = false;
};

USTRUCT(BlueprintType)
struct FFPVInputSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	EFPVStickMode StickMode = EFPVStickMode::Mode2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FFPVStickSettings LeftStick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FFPVStickSettings RightStick;

	/** Low-pass the sticks in Angle mode (hides gamepad stick jitter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bSmoothInAngleMode = true;

	/** Low-pass the sticks in Acro mode (off by default: adds latency). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bSmoothInAcroMode = false;

	/** Smoothing time constant (s) when smoothing is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.005", ClampMax = "0.3"))
	float SmoothingTime = 0.04f;
};

// ---------------------------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVCameraSettings
{
	GENERATED_BODY()

	/** FPV camera uptilt (degrees, positive = looking up). Faster flying needs more uptilt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-10.0", ClampMax = "80.0"))
	float FpvUptiltDeg = 25.0f;

	/** FPV horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "60.0", ClampMax = "170.0"))
	float FpvFovDeg = 120.0f;

	/** Uptilt change per D-pad press (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "15.0"))
	float TiltStepDeg = 5.0f;

	/** Chase camera distance behind the drone (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "100.0", ClampMax = "1500.0"))
	float ChaseDistanceCm = 300.0f;

	/** Chase camera horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "60.0", ClampMax = "130.0"))
	float ChaseFovDeg = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	EFPVCameraView DefaultView = EFPVCameraView::FPV;
};

// ---------------------------------------------------------------------------------------------
// Feedback (rumble)
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVFeedbackSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	bool bRumbleEnabled = true;

	/** Overall rumble strength (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RumbleStrength = 0.8f;

	/** Impacts with a smaller speed change than this (m/s) don't rumble. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float ImpactThresholdMps = 2.5f;
};

// ---------------------------------------------------------------------------------------------
// Battery (cosmetic: shown on the OSD, does not limit thrust yet)
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVBatterySettings
{
	GENERATED_BODY()

	/** LiPo cells in series (6S is typical for a 5" freestyle quad). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery", meta = (ClampMin = "1", ClampMax = "8"))
	int32 CellCount = 6;

	/** Pack capacity (mAh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery", meta = (ClampMin = "200.0", ClampMax = "10000.0"))
	float CapacityMah = 1300.0f;

	/** Pack current draw at 100% motor output (A). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery", meta = (ClampMin = "1.0", ClampMax = "300.0"))
	float MaxCurrentAmps = 120.0f;

	/** Pack internal resistance (ohm). Causes the voltage to sag under load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float InternalResistanceOhm = 0.025f;
};

// ---------------------------------------------------------------------------------------------
// Button bindings (gamepad key names, e.g. "Gamepad_FaceButton_Top")
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVButtonBindings
{
	GENERATED_BODY()

	/** Triangle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName ToggleFlightMode = TEXT("Gamepad_FaceButton_Top");

	/** Circle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName ResetDrone = TEXT("Gamepad_FaceButton_Right");

	/** Square */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName ToggleCamera = TEXT("Gamepad_FaceButton_Left");

	/** D-pad up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName CameraTiltUp = TEXT("Gamepad_DPad_Up");

	/** D-pad down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName CameraTiltDown = TEXT("Gamepad_DPad_Down");

	/** Options */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName OpenMenu = TEXT("Gamepad_Special_Right");

	/** Create / Share */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bindings")
	FName ToggleInputDebug = TEXT("Gamepad_Special_Left");

	FName GetKeyName(EFPVButtonAction Action) const;
	void SetKeyName(EFPVButtonAction Action, FName KeyName);
};

// ---------------------------------------------------------------------------------------------
// Top level: everything that is saved to disk
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFPVUserSettings
{
	GENERATED_BODY()

	/** Settings file format version (for future migrations). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings")
	int32 Version = 1;

	/** Flight mode the drone starts in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EFPVFlightMode DefaultFlightMode = EFPVFlightMode::Angle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVDroneTuning Drone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVInputSettings Input;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVCameraSettings Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVFeedbackSettings Feedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVBatterySettings Battery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FFPVButtonBindings Bindings;

	/** Clamp every value into its valid range (used after loading a hand-edited file). */
	void Sanitize();
};

namespace FPVSettingsText
{
	/** Human readable name for a rebindable action ("Toggle flight mode"). */
	FString GetButtonActionName(EFPVButtonAction Action);
}
