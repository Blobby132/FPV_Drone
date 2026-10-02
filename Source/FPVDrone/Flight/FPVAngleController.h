// Angle (self-leveling) mode outer loop: target tilt -> roll/pitch rate setpoints.

#pragma once

#include "CoreMinimal.h"
#include "Settings/FPVSettingsTypes.h"

namespace FPVAngleController
{
	/**
	 * Computes roll/pitch rate setpoints (deg/s, flight-controller axes: X = roll right, Y = nose up)
	 * that rotate the drone towards the target tilt. The target is defined relative to the drone's
	 * current heading, so yaw is unaffected and the drone self-levels when both targets are zero.
	 *
	 * @param BodyRotation     Body -> world rotation.
	 * @param TargetRollDeg    + = right side down.
	 * @param TargetPitchDeg   + = nose up.
	 */
	FVector2D ComputeRateSetpoints(const FQuat& BodyRotation, float TargetRollDeg, float TargetPitchDeg,
		const FFPVAngleModeSettings& Settings);
}
