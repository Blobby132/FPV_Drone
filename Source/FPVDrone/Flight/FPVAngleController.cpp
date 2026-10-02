#include "Flight/FPVAngleController.h"

FVector2D FPVAngleController::ComputeRateSetpoints(const FQuat& BodyRotation, float TargetRollDeg, float TargetPitchDeg,
	const FFPVAngleModeSettings& Settings)
{
	const FVector WorldUp(0.0, 0.0, 1.0);
	const FVector BodyUp = BodyRotation.GetUpVector();
	const FVector BodyForward = BodyRotation.GetForwardVector();

	// Heading frame: the drone's forward direction flattened onto the ground plane.
	FVector HeadingForward(BodyForward.X, BodyForward.Y, 0.0);
	if (!HeadingForward.Normalize())
	{
		// Nose pointing straight up or down: the belly/back points along the heading instead.
		const double Sign = BodyForward.Z > 0.0 ? -1.0 : 1.0;
		HeadingForward = FVector(BodyUp.X * Sign, BodyUp.Y * Sign, 0.0);
		if (!HeadingForward.Normalize())
		{
			HeadingForward = FVector(1.0, 0.0, 0.0);
		}
	}
	// Up x Forward = Right in Unreal's axes (Z x X = Y).
	const FVector HeadingRight = FVector::CrossProduct(WorldUp, HeadingForward);

	// Desired "up" direction: rolled right tilts the up vector to the right, nose up tilts it backwards.
	const float MaxTilt = FMath::Clamp(Settings.MaxTiltDeg, 1.0f, 80.0f);
	const double RollRad = FMath::DegreesToRadians(FMath::Clamp(TargetRollDeg, -MaxTilt, MaxTilt));
	const double PitchRad = FMath::DegreesToRadians(FMath::Clamp(TargetPitchDeg, -MaxTilt, MaxTilt));
	FVector DesiredUp = WorldUp + HeadingRight * FMath::Tan(RollRad) - HeadingForward * FMath::Tan(PitchRad);
	DesiredUp.Normalize();

	// Shortest rotation that takes the current up vector onto the desired one (axis * angle, world).
	const FVector Cross = FVector::CrossProduct(BodyUp, DesiredUp);
	const double SinAngle = Cross.Size();
	const double CosAngle = FVector::DotProduct(BodyUp, DesiredUp);
	const double Angle = FMath::Atan2(SinAngle, CosAngle);

	FVector ErrorWorld = FVector::ZeroVector;
	if (SinAngle > 1e-6)
	{
		ErrorWorld = (Cross / SinAngle) * Angle;
	}
	else if (CosAngle < 0.0)
	{
		// Exactly upside down: any horizontal axis works; roll over.
		ErrorWorld = BodyForward * UE_DOUBLE_PI;
	}

	// Express the error in body axes. Rotations about -X are roll right and about -Y are nose up
	// (see FPVAxes in FPVFlightTypes.h), hence the sign flips.
	const FVector ErrorBody = BodyRotation.UnrotateVector(ErrorWorld);
	const float RollErrorDeg = static_cast<float>(-FMath::RadiansToDegrees(ErrorBody.X));
	const float PitchErrorDeg = static_cast<float>(-FMath::RadiansToDegrees(ErrorBody.Y));

	const float MaxRate = FMath::Max(Settings.MaxLevelRateDegPerSec, 1.0f);
	return FVector2D(
		FMath::Clamp(RollErrorDeg * Settings.LevelStrength, -MaxRate, MaxRate),
		FMath::Clamp(PitchErrorDeg * Settings.LevelStrength, -MaxRate, MaxRate));
}
