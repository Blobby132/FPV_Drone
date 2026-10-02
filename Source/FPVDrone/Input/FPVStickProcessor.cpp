#include "Input/FPVStickProcessor.h"
#include "Flight/FPVFlightMath.h"

void FFPVStickProcessor::Reset()
{
	Shaped = FFPVStickValues();
	Smoothed = FFPVStickValues();
	LatchedThrottle = 0.0f;
	LastThrottle = 0.0f;
	bHasLastThrottleMode = false;
}

float FFPVStickProcessor::ShapeAxis(float Value, float Deadzone, float Expo, bool bInvert)
{
	float Result = FMath::Clamp(Value, -1.0f, 1.0f);
	if (bInvert)
	{
		Result = -Result;
	}
	Result = FPVFlightMath::ApplyDeadzone(Result, Deadzone);
	return FPVFlightMath::ApplyStickExpo(Result, Expo);
}

FFPVPilotCommand FFPVStickProcessor::Process(const FFPVStickValues& Raw, const FFPVInputSettings& Input,
	const FFPVDroneTuning& Tuning, EFPVFlightMode FlightMode, float Dt)
{
	// 1) Per physical stick: inversion, dead zone, response curve.
	FFPVStickValues Current;
	Current.LeftX = ShapeAxis(Raw.LeftX, Input.LeftStick.Deadzone, Input.LeftStick.Expo, Input.LeftStick.bInvertX);
	Current.LeftY = ShapeAxis(Raw.LeftY, Input.LeftStick.Deadzone, Input.LeftStick.Expo, Input.LeftStick.bInvertY);
	Current.RightX = ShapeAxis(Raw.RightX, Input.RightStick.Deadzone, Input.RightStick.Expo, Input.RightStick.bInvertX);
	Current.RightY = ShapeAxis(Raw.RightY, Input.RightStick.Deadzone, Input.RightStick.Expo, Input.RightStick.bInvertY);

	// 2) Optional smoothing (first-order low-pass). Off in Acro by default because it adds latency.
	const bool bSmooth = (FlightMode == EFPVFlightMode::Angle) ? Input.bSmoothInAngleMode : Input.bSmoothInAcroMode;
	const float Alpha = bSmooth ? FPVFlightMath::TimeConstantAlpha(Dt, Input.SmoothingTime) : 1.0f;
	Smoothed.LeftX += (Current.LeftX - Smoothed.LeftX) * Alpha;
	Smoothed.LeftY += (Current.LeftY - Smoothed.LeftY) * Alpha;
	Smoothed.RightX += (Current.RightX - Smoothed.RightX) * Alpha;
	Smoothed.RightY += (Current.RightY - Smoothed.RightY) * Alpha;
	Shaped = Smoothed;

	// 3) Channel assignment (Mode 2: throttle on the left stick; Mode 1: throttle on the right).
	float ThrottleStick = 0.0f;
	float YawStick = Smoothed.LeftX;
	float PitchStick = 0.0f;
	float RollStick = Smoothed.RightX;
	if (Input.StickMode == EFPVStickMode::Mode1)
	{
		PitchStick = Smoothed.LeftY;
		ThrottleStick = Smoothed.RightY;
	}
	else
	{
		ThrottleStick = Smoothed.LeftY;
		PitchStick = Smoothed.RightY;
	}

	// 4) Throttle from a spring-loaded stick.
	const FFPVThrottleSettings& ThrottleSettings = Tuning.Throttle;
	if (!bHasLastThrottleMode || ThrottleSettings.Mode != LastThrottleMode)
	{
		// Entering latched mode: start from the current throttle so the drone doesn't drop or jump.
		LatchedThrottle = LastThrottle;
		LastThrottleMode = ThrottleSettings.Mode;
		bHasLastThrottleMode = true;
	}

	float Throttle = 0.0f;
	if (ThrottleSettings.Mode == EFPVThrottleMode::Latched)
	{
		LatchedThrottle = FMath::Clamp(LatchedThrottle + ThrottleStick * ThrottleSettings.LatchedRampPerSecond * Dt, 0.0f, 1.0f);
		Throttle = LatchedThrottle;
	}
	else
	{
		// Centered stick = hover; full up = full throttle; full down = zero (motor idle).
		const float Hover = FPVFlightMath::GetEffectiveHoverThrottle(Tuning);
		Throttle = ThrottleStick >= 0.0f
			? FMath::Lerp(Hover, 1.0f, ThrottleStick)
			: FMath::Lerp(Hover, 0.0f, -ThrottleStick);
	}
	LastThrottle = FMath::Clamp(Throttle, 0.0f, 1.0f);

	// 5) Pilot command. Pushing the pitch stick forward (up) pitches the nose down.
	FFPVPilotCommand Command;
	Command.Throttle = LastThrottle;
	Command.Roll = FMath::Clamp(RollStick, -1.0f, 1.0f);
	Command.Pitch = FMath::Clamp(-PitchStick, -1.0f, 1.0f);
	Command.Yaw = FMath::Clamp(YawStick, -1.0f, 1.0f);
	Command.FlightMode = FlightMode;
	return Command;
}
