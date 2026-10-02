#include "Flight/FPVQuadMixer.h"

FFPVMixerResult FPVQuadMixer::Mix(float Throttle, float Roll, float Pitch, float Yaw, bool bAirMode)
{
	using namespace FPVQuadLayout;

	FFPVMixerResult Result;

	float MotorMix[FPVQuad::NumMotors];
	float MixMin = TNumericLimits<float>::Max();
	float MixMax = TNumericLimits<float>::Lowest();
	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		MotorMix[Index] = Roll * RollMix[Index] + Pitch * PitchMix[Index] + Yaw * YawMix[Index];
		MixMin = FMath::Min(MixMin, MotorMix[Index]);
		MixMax = FMath::Max(MixMax, MotorMix[Index]);
	}

	float AppliedThrottle = FMath::Clamp(Throttle, 0.0f, 1.0f);
	const float MixRange = MixMax - MixMin;

	if (MixRange > 1.0f)
	{
		// Corrections alone exceed the motor range: scale them down together.
		for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
		{
			MotorMix[Index] /= MixRange;
		}
		MixMin /= MixRange;
		MixMax /= MixRange;
		Result.bSaturated = true;
		if (bAirMode)
		{
			// Maximum correction: center the throttle.
			AppliedThrottle = 0.5f;
		}
	}
	else if (bAirMode || AppliedThrottle > 0.5f)
	{
		// Shift the throttle just enough that every motor stays within [0, 1].
		AppliedThrottle = FMath::Clamp(AppliedThrottle, -MixMin, 1.0f - MixMax);
	}

	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		const float Unclamped = AppliedThrottle + MotorMix[Index];
		Result.Motors[Index] = FMath::Clamp(Unclamped, 0.0f, 1.0f);
		if (Unclamped < -UE_KINDA_SMALL_NUMBER || Unclamped > 1.0f + UE_KINDA_SMALL_NUMBER)
		{
			Result.bSaturated = true;
		}
	}

	Result.Throttle = AppliedThrottle;
	return Result;
}
