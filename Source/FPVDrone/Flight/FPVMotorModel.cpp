#include "Flight/FPVMotorModel.h"
#include "Flight/FPVFlightMath.h"

void FFPVMotorModel::Reset()
{
	for (float& Value : Output)
	{
		Value = 0.0f;
	}
}

void FFPVMotorModel::Step(const float (&Commands)[FPVQuad::NumMotors], float Dt, const FFPVAirframeSettings& Airframe)
{
	const float Idle = FMath::Clamp(Airframe.MotorIdle, 0.0f, 0.5f);
	const float SpinUpAlpha = FPVFlightMath::TimeConstantAlpha(Dt, Airframe.MotorSpinUpTime);
	const float SpinDownAlpha = FPVFlightMath::TimeConstantAlpha(Dt, Airframe.MotorSpinDownTime);

	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		const float Target = Idle + (1.0f - Idle) * FMath::Clamp(Commands[Index], 0.0f, 1.0f);
		const float Alpha = Target > Output[Index] ? SpinUpAlpha : SpinDownAlpha;
		Output[Index] = FMath::Clamp(Output[Index] + (Target - Output[Index]) * Alpha, 0.0f, 1.0f);
	}
}

float FFPVMotorModel::GetOutput(int32 MotorIndex) const
{
	return (MotorIndex >= 0 && MotorIndex < FPVQuad::NumMotors) ? Output[MotorIndex] : 0.0f;
}

float FFPVMotorModel::GetThrustN(int32 MotorIndex, float MaxThrustPerMotorN, float ThrustExponent) const
{
	const float Normalized = GetOutput(MotorIndex);
	const float Exponent = FMath::Max(ThrustExponent, 0.1f);
	return MaxThrustPerMotorN * FMath::Pow(Normalized, Exponent);
}
