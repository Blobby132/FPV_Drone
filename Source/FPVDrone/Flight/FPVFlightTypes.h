// Plain data types shared by the input layer, the flight controller and the physics bridge.
// No UObjects here: these are copied across the game thread / physics thread boundary.

#pragma once

#include "CoreMinimal.h"
#include "Settings/FPVSettingsTypes.h"

namespace FPVQuad
{
	inline constexpr int32 NumMotors = 4;
}

/**
 * What the pilot is asking for this frame, after all stick processing.
 * Axis conventions (the same as a radio, but expressed as the drone's motion):
 *   Roll  +1 = roll right (right side down)
 *   Pitch +1 = nose up    (right stick pulled back in Mode 2)
 *   Yaw   +1 = nose right
 */
struct FFPVPilotCommand
{
	/** RC throttle 0..1, before the throttle curve (like the throttle gimbal position). */
	float Throttle = 0.0f;
	float Roll = 0.0f;
	float Pitch = 0.0f;
	float Yaw = 0.0f;
	EFPVFlightMode FlightMode = EFPVFlightMode::Angle;
};

/** Rigid body state as seen by the flight controller (SI units, world frame). */
struct FFPVBodyState
{
	/** Body -> world rotation. */
	FQuat Rotation = FQuat::Identity;
	/** World-space linear velocity (m/s). */
	FVector LinearVelocity = FVector::ZeroVector;
	/** World-space angular velocity (rad/s). */
	FVector AngularVelocity = FVector::ZeroVector;
};

/**
 * Flight controller axis rates in deg/s, as a gyro on the drone would report them:
 * X = roll (+ = roll right), Y = pitch (+ = nose up), Z = yaw (+ = nose right).
 *
 * Unreal's rotation math is the standard right-hand formula applied in a left-handed world,
 * so in terms of the body-frame angular velocity vector w_b (rad/s):
 *   roll right = rotation about -X  ->  RollRate  = -w_b.X
 *   nose up    = rotation about -Y  ->  PitchRate = -w_b.Y
 *   nose right = rotation about +Z  ->  YawRate   = +w_b.Z
 */
namespace FPVAxes
{
	FORCEINLINE FVector BodyAngularVelocityToGyroDegS(const FVector& BodyAngularVelocityRad)
	{
		return FVector(
			-FMath::RadiansToDegrees(BodyAngularVelocityRad.X),
			-FMath::RadiansToDegrees(BodyAngularVelocityRad.Y),
			FMath::RadiansToDegrees(BodyAngularVelocityRad.Z));
	}
}

/** Data published by the physics thread every step, read by the game thread (OSD, debug, props). */
struct FFPVFlightTelemetry
{
	/** Motor outputs 0..1 (after idle mapping and spin-up lag). */
	float MotorOutputs[FPVQuad::NumMotors] = { 0.0f, 0.0f, 0.0f, 0.0f };
	/** Collective throttle after the throttle curve and airmode adjustment (0..1). */
	float MixerThrottle = 0.0f;
	/** Rate setpoint and measured rate (deg/s, see FPVAxes). */
	FVector RateSetpointDegS = FVector::ZeroVector;
	FVector GyroDegS = FVector::ZeroVector;
	/** Sum of all four motors' thrust (N). */
	float TotalThrustN = 0.0f;
	bool bMixerSaturated = false;
	EFPVFlightMode FlightMode = EFPVFlightMode::Angle;
	/** Number of physics steps the flight controller has run (lets the debug overlay show the real physics rate). */
	uint64 PhysicsStepCount = 0;
	float LastPhysicsDt = 0.0f;
};
