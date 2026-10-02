#include "Flight/FPVPidAxis.h"
#include "Flight/FPVFlightMath.h"

namespace FPVPidScale
{
	// Betaflight's internal term scales (pid.h). The PID sum is then divided by 1000
	// (PID_MIXER_SCALING) to become a fraction of the motor range.
	inline constexpr float P = 0.032029f;
	inline constexpr float I = 0.244381f;
	inline constexpr float D = 0.000529f;
	inline constexpr float Mixer = 0.001f;

	/** Betaflight ITERM_RELAX_SETPOINT_THRESHOLD (deg/s). */
	inline constexpr float RelaxThreshold = 40.0f;
}

void FFPVPidAxis::Reset()
{
	Integrator = 0.0f;
	PreviousGyro = 0.0f;
	bHasPreviousGyro = false;
	FilteredGyroDerivative = 0.0f;
	SetpointLowPass = 0.0f;
	bLastOutputClamped = false;
	LastTerms = FFPVPidTerms();
}

float FFPVPidAxis::Update(float SetpointDegS, float GyroDegS, float Dt, const FFPVPidGains& Gains,
	const FFPVPidProfile& Profile, float SumLimit, bool bMixerSaturated, float ITermGain)
{
	if (Dt <= 0.0f)
	{
		return LastTerms.Sum;
	}

	const float Error = SetpointDegS - GyroDegS;

	// P
	const float PTerm = Gains.P * FPVPidScale::P * Error * FPVPidScale::Mixer;

	// I-term relax: while the setpoint is changing quickly (high-pass of the setpoint is large),
	// don't integrate. Stops the I-term from overshooting at the end of flips and rolls.
	SetpointLowPass += FPVFlightMath::CutoffAlpha(Dt, Profile.ITermRelaxCutoffHz) * (SetpointDegS - SetpointLowPass);
	float RelaxFactor = 1.0f;
	if (Profile.bITermRelax)
	{
		const float SetpointHighPass = FMath::Abs(SetpointDegS - SetpointLowPass);
		RelaxFactor = FMath::Max(0.0f, 1.0f - SetpointHighPass / FPVPidScale::RelaxThreshold);
	}

	// I, with anti-windup: if the output was clamped (by the PID sum limit or the mixer) last
	// step, don't integrate further in the direction that made it saturate.
	const float IncrementI = Gains.I * FPVPidScale::I * Error * RelaxFactor * Dt * FPVPidScale::Mixer * FMath::Clamp(ITermGain, 0.0f, 1.0f);
	const bool bSaturated = bMixerSaturated || bLastOutputClamped;
	const bool bWindingUp = bSaturated && (IncrementI * LastTerms.Sum > 0.0f);
	if (!bWindingUp)
	{
		Integrator = FMath::Clamp(Integrator + IncrementI, -Profile.ITermLimit, Profile.ITermLimit);
	}

	// D on measurement (negative because a rising gyro should reduce the output), low-passed.
	float GyroDerivative = 0.0f;
	if (bHasPreviousGyro)
	{
		GyroDerivative = (GyroDegS - PreviousGyro) / Dt;
	}
	PreviousGyro = GyroDegS;
	bHasPreviousGyro = true;
	FilteredGyroDerivative += FPVFlightMath::CutoffAlpha(Dt, Profile.DTermCutoffHz) * (GyroDerivative - FilteredGyroDerivative);
	const float DTerm = -Gains.D * FPVPidScale::D * FilteredGyroDerivative * FPVPidScale::Mixer;

	const float RawSum = PTerm + Integrator + DTerm;
	const float Limit = FMath::Max(SumLimit, 0.0f);
	const float Sum = FMath::Clamp(RawSum, -Limit, Limit);
	bLastOutputClamped = FMath::Abs(RawSum) > Limit;

	LastTerms.P = PTerm;
	LastTerms.I = Integrator;
	LastTerms.D = DTerm;
	LastTerms.Sum = Sum;
	return Sum;
}
