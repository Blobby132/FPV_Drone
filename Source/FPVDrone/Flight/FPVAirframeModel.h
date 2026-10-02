// Turns motor thrusts and the body's motion into a net force and torque (SI units).

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"
#include "Settings/FPVSettingsTypes.h"

class FFPVMotorModel;

/** Net force/torque to apply to the rigid body this step (SI, world frame). Gravity is NOT included (Chaos applies it). */
struct FFPVAirframeForces
{
	FVector ForceWorldN = FVector::ZeroVector;
	FVector TorqueWorldNm = FVector::ZeroVector;
	float TotalThrustN = 0.0f;
};

namespace FPVAirframeModel
{
	/** Max thrust of a single motor (N): (ThrustToWeight * Mass * g) / 4. */
	float GetMaxThrustPerMotorN(const FFPVAirframeSettings& Airframe, float GravityMps2);

	/**
	 * Sums the four motors (thrust along the body up axis, applied at each arm position, plus
	 * each prop's reaction torque), linear/quadratic air drag (stronger along the up axis) and
	 * angular drag. Assumes the center of mass is at the body origin (symmetric frame).
	 */
	FFPVAirframeForces ComputeForces(const FFPVBodyState& State, const FFPVMotorModel& Motors,
		const FFPVAirframeSettings& Airframe, float GravityMps2);
}
