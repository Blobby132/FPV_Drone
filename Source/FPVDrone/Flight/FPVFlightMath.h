// Small, pure math helpers used by the input layer and the flight controller.
// Everything here is stateless and thread-safe.

#pragma once

#include "CoreMinimal.h"
#include "Settings/FPVSettingsTypes.h"

namespace FPVFlightMath
{
	/** Per-axis dead zone; the remaining travel is rescaled so the output still reaches +-1. */
	float ApplyDeadzone(float Value, float Deadzone);

	/** Cubic stick response curve. Expo 0 = linear, 1 = pure cubic. Input/output in [-1, 1]. */
	float ApplyStickExpo(float Value, float Expo);

	/** Betaflight rates ("Betaflight" rate type): stick [-1, 1] -> rotation rate in deg/s. */
	float BetaflightRate(float Stick, const FFPVAxisRates& Rates);

	/** Rate at full stick deflection (deg/s). */
	float BetaflightMaxRate(const FFPVAxisRates& Rates);

	/** Betaflight thr_mid / thr_expo throttle curve: [0, 1] -> [0, 1], monotonic. */
	float ApplyThrottleCurve(float Throttle, float Mid, float Expo);

	/** Inverse of ApplyThrottleCurve (numerical). */
	float InvertThrottleCurve(float CurvedThrottle, float Mid, float Expo);

	/**
	 * RC throttle (0..1, before the curve) at which the four motors together produce exactly the
	 * drone's weight, with no rotation commanded. Used as the center of the hover-centered throttle mode.
	 * Independent of mass and gravity because thrust-to-weight is defined relative to weight.
	 */
	float ComputeHoverThrottle(const FFPVDroneTuning& Tuning);

	/** Hover throttle used by the input layer: automatic (above) or the manual value from settings. */
	float GetEffectiveHoverThrottle(const FFPVDroneTuning& Tuning);

	/** Smoothing factor for a first-order low-pass with time constant Tau (s) over Dt (s). Tau <= 0 -> 1 (no smoothing). */
	float TimeConstantAlpha(float Dt, float Tau);

	/** Smoothing factor for a first-order low-pass with a cutoff frequency (Hz) over Dt (s). */
	float CutoffAlpha(float Dt, float CutoffHz);
}
