#include "Flight/FPVFlightController.h"
#include "Flight/FPVAngleController.h"
#include "Flight/FPVFlightMath.h"
#include "Flight/FPVQuadMixer.h"

namespace FPVFlightControllerConstants
{
	/** Time constant (s) with which the I-term bleeds off while the throttle is below ITermMinThrottle. */
	inline constexpr float ITermBleedTime = 0.3f;
}

void FFPVFlightController::Reset()
{
	RollPid.Reset();
	PitchPid.Reset();
	YawPid.Reset();
	bLastMixerSaturated = false;
	bHasRun = false;
}

FVector FFPVFlightController::ComputeRateSetpoints(const FFPVPilotCommand& Command, const FFPVBodyState& State,
	const FFPVDroneTuning& Tuning) const
{
	FVector Setpoint = FVector::ZeroVector;

	// Yaw is rate-based in every mode.
	Setpoint.Z = FPVFlightMath::BetaflightRate(Command.Yaw, Tuning.Rates.Yaw);

	switch (Command.FlightMode)
	{
	case EFPVFlightMode::Acro:
	{
		// Rate mode: sticks command rotation rates directly. With centered sticks the setpoint is
		// zero rate, so the drone holds whatever attitude it is in (no self-leveling).
		Setpoint.X = FPVFlightMath::BetaflightRate(Command.Roll, Tuning.Rates.Roll);
		Setpoint.Y = FPVFlightMath::BetaflightRate(Command.Pitch, Tuning.Rates.Pitch);
		break;
	}

	case EFPVFlightMode::Angle:
	default:
	{
		// Stick deflection -> target tilt; the outer loop turns the tilt error into rates.
		const float MaxTilt = Tuning.AngleMode.MaxTiltDeg;
		const FVector2D LevelRates = FPVAngleController::ComputeRateSetpoints(
			State.Rotation, Command.Roll * MaxTilt, Command.Pitch * MaxTilt, Tuning.AngleMode);
		Setpoint.X = LevelRates.X;
		Setpoint.Y = LevelRates.Y;
		break;
	}
	}

	return Setpoint;
}

void FFPVFlightController::Update(const FFPVPilotCommand& Command, const FFPVBodyState& State,
	const FFPVDroneTuning& Tuning, float Dt, FFPVControllerOutput& OutOutput)
{
	if (Dt <= 0.0f)
	{
		return;
	}

	// Switching modes changes what the setpoint means; start the PIDs fresh to avoid a kick.
	if (!bHasRun || Command.FlightMode != LastFlightMode)
	{
		RollPid.Reset();
		PitchPid.Reset();
		YawPid.Reset();
		LastFlightMode = Command.FlightMode;
		bHasRun = true;
	}

	// "Gyro": body-frame rotation rates in flight-controller axes.
	const FVector AngularVelocityBody = State.Rotation.UnrotateVector(State.AngularVelocity);
	const FVector Gyro = FPVAxes::BodyAngularVelocityToGyroDegS(AngularVelocityBody);

	const FVector Setpoint = ComputeRateSetpoints(Command, State, Tuning);

	const FFPVThrottleSettings& ThrottleSettings = Tuning.Throttle;
	const float Throttle = FPVFlightMath::ApplyThrottleCurve(Command.Throttle, ThrottleSettings.CurveMid, ThrottleSettings.CurveExpo);

	// Bleed the I-term off at very low throttle so it can't wind up while sitting on the ground.
	const FFPVPidProfile& Pid = Tuning.Pid;
	float ITermGain = 1.0f;
	if (Throttle < Pid.ITermMinThrottle)
	{
		ITermGain = 0.0f;
		const float Keep = 1.0f - FPVFlightMath::TimeConstantAlpha(Dt, FPVFlightControllerConstants::ITermBleedTime);
		RollPid.DecayIntegrator(Keep);
		PitchPid.DecayIntegrator(Keep);
		YawPid.DecayIntegrator(Keep);
	}

	const float RollOut = RollPid.Update(static_cast<float>(Setpoint.X), static_cast<float>(Gyro.X), Dt,
		Pid.Roll, Pid, Pid.PidSumLimit, bLastMixerSaturated, ITermGain);
	const float PitchOut = PitchPid.Update(static_cast<float>(Setpoint.Y), static_cast<float>(Gyro.Y), Dt,
		Pid.Pitch, Pid, Pid.PidSumLimit, bLastMixerSaturated, ITermGain);
	const float YawOut = YawPid.Update(static_cast<float>(Setpoint.Z), static_cast<float>(Gyro.Z), Dt,
		Pid.Yaw, Pid, Pid.PidSumLimitYaw, bLastMixerSaturated, ITermGain);

	const FFPVMixerResult Mix = FPVQuadMixer::Mix(Throttle, RollOut, PitchOut, YawOut, Pid.bAirMode);
	bLastMixerSaturated = Mix.bSaturated;

	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		OutOutput.MotorCommands[Index] = Mix.Motors[Index];
	}
	OutOutput.MixerThrottle = Mix.Throttle;
	OutOutput.RateSetpointDegS = Setpoint;
	OutOutput.GyroDegS = Gyro;
	OutOutput.bMixerSaturated = Mix.bSaturated;
}
