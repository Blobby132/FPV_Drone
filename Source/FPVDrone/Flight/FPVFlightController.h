// The simulated flight controller: pilot command + gyro/attitude -> four motor commands.
// Runs on the physics thread, once per physics step.

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"
#include "Flight/FPVPidAxis.h"
#include "Settings/FPVSettingsTypes.h"

struct FFPVControllerOutput
{
	/** Mixer motor commands 0..1 (the motor model adds idle and lag). */
	float MotorCommands[FPVQuad::NumMotors] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float MixerThrottle = 0.0f;
	FVector RateSetpointDegS = FVector::ZeroVector;
	FVector GyroDegS = FVector::ZeroVector;
	bool bMixerSaturated = false;
};

class FFPVFlightController
{
public:
	/** Clear all controller state (integrators, filters). Call on respawn. */
	void Reset();

	void Update(const FFPVPilotCommand& Command, const FFPVBodyState& State, const FFPVDroneTuning& Tuning,
		float Dt, FFPVControllerOutput& OutOutput);

private:
	/** Rate setpoints (deg/s, FC axes) for the current flight mode. */
	FVector ComputeRateSetpoints(const FFPVPilotCommand& Command, const FFPVBodyState& State, const FFPVDroneTuning& Tuning) const;

	FFPVPidAxis RollPid;
	FFPVPidAxis PitchPid;
	FFPVPidAxis YawPid;

	bool bLastMixerSaturated = false;
	EFPVFlightMode LastFlightMode = EFPVFlightMode::Angle;
	bool bHasRun = false;
};
