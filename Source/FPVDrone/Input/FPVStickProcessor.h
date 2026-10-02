// Turns raw thumbstick values into an FPV pilot command, the way an RC radio + flight controller
// would see it: dead zone -> inversion -> response curve -> smoothing -> Mode 1/2 channel
// assignment -> spring-loaded-throttle handling (hover-centered or latched).

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"
#include "Settings/FPVSettingsTypes.h"

/** Raw (or shaped) values of the two physical thumbsticks, -1..1, Y = +1 when pushed up. */
struct FFPVStickValues
{
	float LeftX = 0.0f;
	float LeftY = 0.0f;
	float RightX = 0.0f;
	float RightY = 0.0f;
};

class FFPVStickProcessor
{
public:
	/** Forget smoothing history and set the latched throttle back to zero (call on respawn). */
	void Reset();

	/**
	 * @param Raw         Raw stick values from Enhanced Input.
	 * @param Input       Dead zones, curves, inversion, smoothing, stick mode.
	 * @param Tuning      Throttle mode, hover point and ramp speed.
	 * @param FlightMode  Selects whether smoothing is active.
	 * @param Dt          Frame time (s).
	 */
	FFPVPilotCommand Process(const FFPVStickValues& Raw, const FFPVInputSettings& Input,
		const FFPVDroneTuning& Tuning, EFPVFlightMode FlightMode, float Dt);

	/** Stick values after dead zone, inversion, curve and smoothing (for the input debug overlay). */
	const FFPVStickValues& GetShapedSticks() const { return Shaped; }

	float GetLatchedThrottle() const { return LatchedThrottle; }

private:
	static float ShapeAxis(float Value, float Deadzone, float Expo, bool bInvert);

	FFPVStickValues Shaped;
	FFPVStickValues Smoothed;
	float LatchedThrottle = 0.0f;
	float LastThrottle = 0.0f;
	EFPVThrottleMode LastThrottleMode = EFPVThrottleMode::HoverCentered;
	bool bHasLastThrottleMode = false;
};
