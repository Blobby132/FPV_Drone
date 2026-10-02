// Unit conversions between Unreal (cm, kg, s) and the SI units used by the flight model (m, kg, s).
//
// Unreal: length cm, velocity cm/s, force kg*cm/s^2, torque kg*cm^2/s^2, gravity -980 cm/s^2.
// SI:     length m,  velocity m/s,  force N (kg*m/s^2), torque N*m (kg*m^2/s^2), gravity -9.8 m/s^2.

#pragma once

#include "CoreMinimal.h"

namespace FPVUnits
{
	inline constexpr float CmPerMeter = 100.0f;
	inline constexpr float MetersPerCm = 0.01f;
	inline constexpr float MpsToKmh = 3.6f;

	/** cm or cm/s -> m or m/s */
	FORCEINLINE FVector CmToMeters(const FVector& Cm) { return Cm * static_cast<double>(MetersPerCm); }
	FORCEINLINE float CmToMeters(float Cm) { return Cm * MetersPerCm; }

	/** m or m/s -> cm or cm/s */
	FORCEINLINE FVector MetersToCm(const FVector& Meters) { return Meters * static_cast<double>(CmPerMeter); }

	/** N -> Unreal force units (kg*cm/s^2): 1 N = 100 kg*cm/s^2 */
	FORCEINLINE FVector NewtonsToUnrealForce(const FVector& Newtons) { return Newtons * 100.0; }

	/** N*m -> Unreal torque units (kg*cm^2/s^2): 1 N*m = 10000 kg*cm^2/s^2 */
	FORCEINLINE FVector NewtonMetersToUnrealTorque(const FVector& NewtonMeters) { return NewtonMeters * 10000.0; }

	/** kg*m^2 -> kg*cm^2 */
	inline constexpr float KgM2ToKgCm2 = 10000.0f;
}
