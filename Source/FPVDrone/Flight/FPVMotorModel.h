// Four motors with first-order spin-up / spin-down lag.

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"
#include "Settings/FPVSettingsTypes.h"

class FFPVMotorModel
{
public:
	void Reset();

	/**
	 * Advance the motors towards the mixer commands.
	 * @param Commands  Mixer outputs 0..1. Mapped to Idle + (1 - Idle) * Command, so zero throttle still idles.
	 */
	void Step(const float (&Commands)[FPVQuad::NumMotors], float Dt, const FFPVAirframeSettings& Airframe);

	/** Current normalized motor output 0..1 (after idle mapping and lag). */
	float GetOutput(int32 MotorIndex) const;

	/** Thrust of one motor (N) given the per-motor maximum. */
	float GetThrustN(int32 MotorIndex, float MaxThrustPerMotorN, float ThrustExponent) const;

private:
	float Output[FPVQuad::NumMotors] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
