// One axis of the rate PID controller (roll, pitch or yaw).

#pragma once

#include "CoreMinimal.h"
#include "Settings/FPVSettingsTypes.h"

/** The individual terms of the last update (fractions of motor range), for debugging. */
struct FFPVPidTerms
{
	float P = 0.0f;
	float I = 0.0f;
	float D = 0.0f;
	float Sum = 0.0f;
};

/**
 * Rate PID, structured like Betaflight's:
 *  - P on rate error, I on rate error (with I-term relax and anti-windup), D on measurement
 *    (gyro) with a low-pass filter, so stick moves don't kick the D-term.
 *  - Term scaling matches Betaflight's (PTERM/ITERM/DTERM_SCALE, then / 1000 for the mixer),
 *    so the gains have Betaflight-like magnitudes.
 * Output is a mixer command: a fraction of the full motor range, clamped to +-SumLimit.
 */
class FFPVPidAxis
{
public:
	void Reset();

	/** Scale the integrator towards zero (Keep = 1 holds it, 0 clears it). */
	void DecayIntegrator(float Keep) { Integrator *= FMath::Clamp(Keep, 0.0f, 1.0f); }

	/**
	 * @param SetpointDegS      Desired rotation rate (deg/s).
	 * @param GyroDegS          Measured rotation rate (deg/s).
	 * @param Dt                Step duration (s).
	 * @param Gains             P/I/D for this axis.
	 * @param Profile           Shared PID settings (limits, filters, relax).
	 * @param SumLimit          Clamp for the output (fraction of motor range).
	 * @param bMixerSaturated   The mixer could not deliver last step's command (anti-windup).
	 * @param ITermGain         0..1 scale on integration (0 = hold, used for low-throttle bleed-off).
	 */
	float Update(float SetpointDegS, float GyroDegS, float Dt, const FFPVPidGains& Gains,
		const FFPVPidProfile& Profile, float SumLimit, bool bMixerSaturated, float ITermGain);

	const FFPVPidTerms& GetLastTerms() const { return LastTerms; }

private:
	float Integrator = 0.0f;
	float PreviousGyro = 0.0f;
	bool bHasPreviousGyro = false;
	float FilteredGyroDerivative = 0.0f;
	float SetpointLowPass = 0.0f;
	bool bLastOutputClamped = false;
	FFPVPidTerms LastTerms;
};
