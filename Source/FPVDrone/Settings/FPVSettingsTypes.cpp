#include "Settings/FPVSettingsTypes.h"

FName FFPVButtonBindings::GetKeyName(EFPVButtonAction Action) const
{
	switch (Action)
	{
	case EFPVButtonAction::ToggleFlightMode:	return ToggleFlightMode;
	case EFPVButtonAction::ResetDrone:			return ResetDrone;
	case EFPVButtonAction::ToggleCamera:		return ToggleCamera;
	case EFPVButtonAction::CameraTiltUp:		return CameraTiltUp;
	case EFPVButtonAction::CameraTiltDown:		return CameraTiltDown;
	case EFPVButtonAction::OpenMenu:			return OpenMenu;
	case EFPVButtonAction::ToggleInputDebug:	return ToggleInputDebug;
	default:									return NAME_None;
	}
}

void FFPVButtonBindings::SetKeyName(EFPVButtonAction Action, FName KeyName)
{
	switch (Action)
	{
	case EFPVButtonAction::ToggleFlightMode:	ToggleFlightMode = KeyName; break;
	case EFPVButtonAction::ResetDrone:			ResetDrone = KeyName; break;
	case EFPVButtonAction::ToggleCamera:		ToggleCamera = KeyName; break;
	case EFPVButtonAction::CameraTiltUp:		CameraTiltUp = KeyName; break;
	case EFPVButtonAction::CameraTiltDown:		CameraTiltDown = KeyName; break;
	case EFPVButtonAction::OpenMenu:			OpenMenu = KeyName; break;
	case EFPVButtonAction::ToggleInputDebug:	ToggleInputDebug = KeyName; break;
	default:									break;
	}
}

namespace
{
	void ClampValue(float& Value, float Min, float Max)
	{
		Value = FMath::Clamp(Value, Min, Max);
	}

	void SanitizeRates(FFPVAxisRates& Rates)
	{
		ClampValue(Rates.RcRate, 0.05f, 2.55f);
		ClampValue(Rates.SuperRate, 0.0f, 0.95f);
		ClampValue(Rates.Expo, 0.0f, 1.0f);
	}

	void SanitizeGains(FFPVPidGains& Gains)
	{
		ClampValue(Gains.P, 0.0f, 250.0f);
		ClampValue(Gains.I, 0.0f, 250.0f);
		ClampValue(Gains.D, 0.0f, 250.0f);
	}

	void SanitizeStick(FFPVStickSettings& Stick)
	{
		ClampValue(Stick.Deadzone, 0.0f, 0.5f);
		ClampValue(Stick.Expo, 0.0f, 1.0f);
	}
}

void FFPVUserSettings::Sanitize()
{
	FFPVDroneTuning& T = Drone;

	SanitizeRates(T.Rates.Roll);
	SanitizeRates(T.Rates.Pitch);
	SanitizeRates(T.Rates.Yaw);

	SanitizeGains(T.Pid.Roll);
	SanitizeGains(T.Pid.Pitch);
	SanitizeGains(T.Pid.Yaw);
	ClampValue(T.Pid.ITermLimit, 0.0f, 1.0f);
	ClampValue(T.Pid.ITermRelaxCutoffHz, 1.0f, 100.0f);
	ClampValue(T.Pid.ITermMinThrottle, 0.0f, 0.5f);
	ClampValue(T.Pid.DTermCutoffHz, 5.0f, 115.0f);
	ClampValue(T.Pid.PidSumLimit, 0.1f, 1.0f);
	ClampValue(T.Pid.PidSumLimitYaw, 0.1f, 1.0f);

	ClampValue(T.AngleMode.MaxTiltDeg, 5.0f, 80.0f);
	ClampValue(T.AngleMode.LevelStrength, 1.0f, 20.0f);
	ClampValue(T.AngleMode.MaxLevelRateDegPerSec, 50.0f, 1500.0f);

	FFPVAirframeSettings& A = T.Airframe;
	ClampValue(A.MassKg, 0.1f, 5.0f);
	ClampValue(A.ThrustToWeight, 1.2f, 15.0f);
	ClampValue(A.ArmLengthCm, 3.0f, 40.0f);
	ClampValue(A.RollPitchInertia, 0.0002f, 0.05f);
	ClampValue(A.YawInertia, 0.0002f, 0.08f);
	ClampValue(A.MotorSpinUpTime, 0.002f, 0.2f);
	ClampValue(A.MotorSpinDownTime, 0.002f, 0.3f);
	ClampValue(A.MotorIdle, 0.0f, 0.2f);
	ClampValue(A.ThrustExponent, 1.0f, 2.5f);
	ClampValue(A.YawTorquePerThrust, 0.0f, 0.1f);
	ClampValue(A.LinearDrag, 0.0f, 2.0f);
	ClampValue(A.QuadraticDrag, 0.0f, 0.2f);
	ClampValue(A.VerticalDragMultiplier, 0.5f, 5.0f);
	ClampValue(A.AngularDragRollPitch, 0.0f, 0.05f);
	ClampValue(A.AngularDragYaw, 0.0f, 0.05f);

	ClampValue(T.Throttle.ManualHoverThrottle, 0.05f, 0.95f);
	ClampValue(T.Throttle.LatchedRampPerSecond, 0.05f, 5.0f);
	ClampValue(T.Throttle.CurveMid, 0.0f, 1.0f);
	ClampValue(T.Throttle.CurveExpo, 0.0f, 1.0f);

	SanitizeStick(Input.LeftStick);
	SanitizeStick(Input.RightStick);
	ClampValue(Input.SmoothingTime, 0.005f, 0.3f);

	ClampValue(Camera.FpvUptiltDeg, -10.0f, 80.0f);
	ClampValue(Camera.FpvFovDeg, 60.0f, 170.0f);
	ClampValue(Camera.TiltStepDeg, 1.0f, 15.0f);
	ClampValue(Camera.ChaseDistanceCm, 100.0f, 1500.0f);
	ClampValue(Camera.ChaseFovDeg, 60.0f, 130.0f);

	ClampValue(Feedback.RumbleStrength, 0.0f, 1.0f);
	ClampValue(Feedback.ImpactThresholdMps, 0.5f, 20.0f);

	Battery.CellCount = FMath::Clamp(Battery.CellCount, 1, 8);
	ClampValue(Battery.CapacityMah, 200.0f, 10000.0f);
	ClampValue(Battery.MaxCurrentAmps, 1.0f, 300.0f);
	ClampValue(Battery.InternalResistanceOhm, 0.0f, 0.5f);

	// Restore any binding that was left empty (e.g. a hand-edited file).
	const FFPVButtonBindings DefaultBindings;
	for (int32 Index = 0; Index < static_cast<int32>(EFPVButtonAction::Count); ++Index)
	{
		const EFPVButtonAction Action = static_cast<EFPVButtonAction>(Index);
		if (Bindings.GetKeyName(Action).IsNone())
		{
			Bindings.SetKeyName(Action, DefaultBindings.GetKeyName(Action));
		}
	}
}

FString FPVSettingsText::GetButtonActionName(EFPVButtonAction Action)
{
	switch (Action)
	{
	case EFPVButtonAction::ToggleFlightMode:	return TEXT("Toggle flight mode");
	case EFPVButtonAction::ResetDrone:			return TEXT("Reset drone");
	case EFPVButtonAction::ToggleCamera:		return TEXT("Toggle FPV / chase camera");
	case EFPVButtonAction::CameraTiltUp:		return TEXT("Camera tilt up");
	case EFPVButtonAction::CameraTiltDown:		return TEXT("Camera tilt down");
	case EFPVButtonAction::OpenMenu:			return TEXT("Open menu");
	case EFPVButtonAction::ToggleInputDebug:	return TEXT("Toggle input debug overlay");
	default:									return TEXT("?");
	}
}
