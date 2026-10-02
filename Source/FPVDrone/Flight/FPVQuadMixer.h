// Quad-X motor layout and mixer.

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"

/**
 * Motor layout, Unreal body axes (+X forward, +Y right, +Z up), viewed from above:
 *
 *        front
 *    3 (FL)   0 (FR)
 *         \ /
 *         / \
 *    2 (RL)   1 (RR)
 *
 * Positions are in units of (ArmLength * cos 45deg). Spin is Betaflight's default "props in":
 * FR and RL spin counter-clockwise (seen from above), RR and FL clockwise.
 */
namespace FPVQuadLayout
{
	inline constexpr float PosX[FPVQuad::NumMotors] = { 1.0f, -1.0f, -1.0f, 1.0f };
	inline constexpr float PosY[FPVQuad::NumMotors] = { 1.0f, 1.0f, -1.0f, -1.0f };

	/** +1 = prop spins counter-clockwise seen from above. Its drag torque on the frame then yaws the nose right. */
	inline constexpr float Spin[FPVQuad::NumMotors] = { 1.0f, -1.0f, 1.0f, -1.0f };

	// Mixer factors, derived from the layout above:
	//  roll right -> left motors (Y < 0) up, right motors down  -> -sign(Y)
	//  nose up    -> front motors (X > 0) up, rear motors down  -> +sign(X)
	//  nose right -> counter-clockwise motors up                -> Spin
	inline constexpr float RollMix[FPVQuad::NumMotors] = { -1.0f, -1.0f, 1.0f, 1.0f };
	inline constexpr float PitchMix[FPVQuad::NumMotors] = { 1.0f, -1.0f, -1.0f, 1.0f };
	inline constexpr float YawMix[FPVQuad::NumMotors] = { 1.0f, -1.0f, 1.0f, -1.0f };

	/** Human readable motor names for debugging. */
	inline const TCHAR* MotorName(int32 Index)
	{
		static const TCHAR* Names[FPVQuad::NumMotors] = { TEXT("FR"), TEXT("RR"), TEXT("RL"), TEXT("FL") };
		return (Index >= 0 && Index < FPVQuad::NumMotors) ? Names[Index] : TEXT("?");
	}
}

struct FFPVMixerResult
{
	/** Motor commands 0..1 (before idle mapping). */
	float Motors[FPVQuad::NumMotors] = { 0.0f, 0.0f, 0.0f, 0.0f };
	/** Collective throttle after airmode adjustment. */
	float Throttle = 0.0f;
	/** True if the requested roll/pitch/yaw could not be delivered in full. */
	bool bSaturated = false;
};

namespace FPVQuadMixer
{
	/**
	 * Betaflight-style mixer. Roll/Pitch/Yaw are PID outputs (fractions of motor range).
	 * If the corrections need more than the full motor range they are scaled down together.
	 * With airmode, the throttle is shifted so the corrections always fit (full authority at
	 * zero throttle); without it, motors simply clip at 0 and 1.
	 */
	FFPVMixerResult Mix(float Throttle, float Roll, float Pitch, float Yaw, bool bAirMode);
}
