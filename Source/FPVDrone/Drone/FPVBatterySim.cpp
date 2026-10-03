#include "Drone/FPVBatterySim.h"
#include "Flight/FPVFlightMath.h"

namespace FPVBatteryConstants
{
	/** Flight controller, VTX, receiver, camera (A). */
	inline constexpr float ElectronicsCurrentAmps = 0.6f;
	/** Display filter for the voltage (s). */
	inline constexpr float VoltageFilterTime = 0.25f;
}

float FFPVBatterySim::RestingCellVoltage(float RemainingFraction)
{
	// Typical LiPo resting voltage vs. state of charge, at 10% steps (index 0 = empty).
	static const float Curve[11] = { 3.30f, 3.50f, 3.65f, 3.71f, 3.75f, 3.79f, 3.84f, 3.90f, 3.98f, 4.08f, 4.20f };
	const float Position = FMath::Clamp(RemainingFraction, 0.0f, 1.0f) * 10.0f;
	const int32 Index = FMath::Min(FMath::FloorToInt(Position), 9);
	const float Alpha = Position - static_cast<float>(Index);
	return FMath::Lerp(Curve[Index], Curve[Index + 1], Alpha);
}

void FFPVBatterySim::Reset(const FFPVBatterySettings& Settings)
{
	State = FFPVBatteryState();
	State.RemainingFraction = 1.0f;
	State.CellVoltage = RestingCellVoltage(1.0f);
	State.PackVoltage = State.CellVoltage * static_cast<float>(FMath::Max(Settings.CellCount, 1));
	bHasVoltage = true;
}

void FFPVBatterySim::Update(float Dt, float AverageMotorPower, const FFPVBatterySettings& Settings)
{
	if (Dt <= 0.0f)
	{
		return;
	}

	const float CellCount = static_cast<float>(FMath::Max(Settings.CellCount, 1));
	State.CurrentAmps = FPVBatteryConstants::ElectronicsCurrentAmps
		+ Settings.MaxCurrentAmps * FMath::Clamp(AverageMotorPower, 0.0f, 1.0f);

	// mAh = A * h * 1000 = A * s / 3.6
	State.ConsumedMah += State.CurrentAmps * Dt / 3.6f;
	State.RemainingFraction = FMath::Clamp(1.0f - State.ConsumedMah / FMath::Max(Settings.CapacityMah, 1.0f), 0.0f, 1.0f);

	const float RestingPack = RestingCellVoltage(State.RemainingFraction) * CellCount;
	const float LoadedPack = FMath::Max(RestingPack - State.CurrentAmps * Settings.InternalResistanceOhm, 0.0f);

	if (!bHasVoltage)
	{
		State.PackVoltage = LoadedPack;
		bHasVoltage = true;
	}
	State.PackVoltage += (LoadedPack - State.PackVoltage) * FPVFlightMath::TimeConstantAlpha(Dt, FPVBatteryConstants::VoltageFilterTime);
	State.CellVoltage = State.PackVoltage / CellCount;
}
