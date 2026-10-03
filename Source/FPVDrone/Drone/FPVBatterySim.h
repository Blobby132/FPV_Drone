// Cosmetic LiPo battery for the OSD: drains with motor output and sags under load.
// It does not limit thrust (yet); it only feeds the voltage / percentage display.

#pragma once

#include "CoreMinimal.h"
#include "Settings/FPVSettingsTypes.h"

struct FFPVBatteryState
{
	/** Pack voltage under load, lightly filtered for display (V). */
	float PackVoltage = 0.0f;
	/** Average cell voltage under load (V). */
	float CellVoltage = 0.0f;
	/** Remaining capacity 0..1. */
	float RemainingFraction = 1.0f;
	float CurrentAmps = 0.0f;
	float ConsumedMah = 0.0f;
};

class FFPVBatterySim
{
public:
	/** A fresh, fully charged pack. */
	void Reset(const FFPVBatterySettings& Settings);

	/**
	 * @param AverageMotorPower  Mean over the motors of output^1.5 (0..1); electrical power grows
	 *                           faster than thrust, roughly with thrust^1.5.
	 */
	void Update(float Dt, float AverageMotorPower, const FFPVBatterySettings& Settings);

	const FFPVBatteryState& GetState() const { return State; }

	/** Resting LiPo cell voltage for a remaining-capacity fraction (typical discharge curve). */
	static float RestingCellVoltage(float RemainingFraction);

private:
	FFPVBatteryState State;
	bool bHasVoltage = false;
};
