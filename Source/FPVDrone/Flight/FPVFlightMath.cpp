#include "Flight/FPVFlightMath.h"

float FPVFlightMath::ApplyDeadzone(float Value, float Deadzone)
{
	const float Dz = FMath::Clamp(Deadzone, 0.0f, 0.95f);
	const float Magnitude = FMath::Abs(Value);
	if (Magnitude <= Dz)
	{
		return 0.0f;
	}
	const float Scaled = FMath::Clamp((Magnitude - Dz) / (1.0f - Dz), 0.0f, 1.0f);
	return Value < 0.0f ? -Scaled : Scaled;
}

float FPVFlightMath::ApplyStickExpo(float Value, float Expo)
{
	const float V = FMath::Clamp(Value, -1.0f, 1.0f);
	const float E = FMath::Clamp(Expo, 0.0f, 1.0f);
	return V * (1.0f - E) + V * V * V * E;
}

float FPVFlightMath::BetaflightRate(float Stick, const FFPVAxisRates& Rates)
{
	// Port of Betaflight's applyBetaflightRates() (rates type BETAFLIGHT), with the
	// stored percentages (e.g. rc_rate = 100) expressed as fractions (1.0).
	float Command = FMath::Clamp(Stick, -1.0f, 1.0f);
	const float CommandAbs = FMath::Abs(Command);

	const float Expo = FMath::Clamp(Rates.Expo, 0.0f, 1.0f);
	if (Expo > 0.0f)
	{
		Command = Command * CommandAbs * CommandAbs * CommandAbs * Expo + Command * (1.0f - Expo);
	}

	float RcRate = FMath::Max(Rates.RcRate, 0.0f);
	if (RcRate > 2.0f)
	{
		// Betaflight's RC_RATE_INCREMENTAL: rc rates above 2.0 grow much faster.
		RcRate += 14.54f * (RcRate - 2.0f);
	}

	float AngleRate = 200.0f * RcRate * Command;

	const float SuperRate = FMath::Clamp(Rates.SuperRate, 0.0f, 0.99f);
	if (SuperRate > 0.0f)
	{
		const float SuperFactor = 1.0f / FMath::Clamp(1.0f - CommandAbs * SuperRate, 0.01f, 1.0f);
		AngleRate *= SuperFactor;
	}

	// Betaflight's default rate_limit.
	return FMath::Clamp(AngleRate, -1998.0f, 1998.0f);
}

float FPVFlightMath::BetaflightMaxRate(const FFPVAxisRates& Rates)
{
	return BetaflightRate(1.0f, Rates);
}

float FPVFlightMath::ApplyThrottleCurve(float Throttle, float Mid, float Expo)
{
	// Port of Betaflight's throttle lookup: expo is applied symmetrically around thr_mid,
	// and the curve always passes through (0,0), (Mid,Mid) and (1,1).
	const float T = FMath::Clamp(Throttle, 0.0f, 1.0f);
	const float M = FMath::Clamp(Mid, 0.0f, 1.0f);
	const float E = FMath::Clamp(Expo, 0.0f, 1.0f);

	const float Delta = T - M;
	const float Range = Delta > 0.0f ? (1.0f - M) : M;
	if (Range <= UE_KINDA_SMALL_NUMBER)
	{
		return T;
	}
	const float Normalized = Delta / Range;
	return FMath::Clamp(M + Delta * (1.0f - E + E * Normalized * Normalized), 0.0f, 1.0f);
}

float FPVFlightMath::InvertThrottleCurve(float CurvedThrottle, float Mid, float Expo)
{
	// The curve is monotonic, so a short bisection is exact enough (2^-30).
	const float Target = FMath::Clamp(CurvedThrottle, 0.0f, 1.0f);
	float Low = 0.0f;
	float High = 1.0f;
	for (int32 Iteration = 0; Iteration < 30; ++Iteration)
	{
		const float MidPoint = 0.5f * (Low + High);
		if (ApplyThrottleCurve(MidPoint, Mid, Expo) < Target)
		{
			Low = MidPoint;
		}
		else
		{
			High = MidPoint;
		}
	}
	return 0.5f * (Low + High);
}

float FPVFlightMath::ComputeHoverThrottle(const FFPVDroneTuning& Tuning)
{
	const FFPVAirframeSettings& Airframe = Tuning.Airframe;

	// Each motor must produce 1/TWR of its max thrust: Output^Exponent = 1/TWR.
	const float ThrustToWeight = FMath::Max(Airframe.ThrustToWeight, 1.01f);
	const float Exponent = FMath::Max(Airframe.ThrustExponent, 0.1f);
	const float MotorOutput = FMath::Pow(1.0f / ThrustToWeight, 1.0f / Exponent);

	// Motor output = Idle + (1 - Idle) * MixerThrottle  ->  solve for the mixer throttle.
	const float Idle = FMath::Clamp(Airframe.MotorIdle, 0.0f, 0.5f);
	const float MixerThrottle = FMath::Clamp((MotorOutput - Idle) / (1.0f - Idle), 0.0f, 1.0f);

	// Undo the throttle curve to get the RC (stick) throttle.
	return InvertThrottleCurve(MixerThrottle, Tuning.Throttle.CurveMid, Tuning.Throttle.CurveExpo);
}

float FPVFlightMath::GetEffectiveHoverThrottle(const FFPVDroneTuning& Tuning)
{
	return Tuning.Throttle.bAutoHoverThrottle
		? ComputeHoverThrottle(Tuning)
		: FMath::Clamp(Tuning.Throttle.ManualHoverThrottle, 0.05f, 0.95f);
}

float FPVFlightMath::TimeConstantAlpha(float Dt, float Tau)
{
	if (Tau <= UE_KINDA_SMALL_NUMBER || Dt <= 0.0f)
	{
		return 1.0f;
	}
	return 1.0f - FMath::Exp(-Dt / Tau);
}

float FPVFlightMath::CutoffAlpha(float Dt, float CutoffHz)
{
	if (CutoffHz <= UE_KINDA_SMALL_NUMBER || Dt <= 0.0f)
	{
		return 1.0f;
	}
	const float Rc = 1.0f / (2.0f * UE_PI * CutoffHz);
	return Dt / (Dt + Rc);
}
