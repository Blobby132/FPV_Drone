// Input debug overlay: raw gamepad axes/buttons exactly as Unreal reports them, the values after
// the project's stick processing, the resulting pilot command, and flight-controller telemetry.
// Meant for diagnosing controllers that report axes unexpectedly (wrong sign, swapped axes,
// leftover dead zones, missing buttons).

#pragma once

#include "CoreMinimal.h"
#include "Flight/FPVFlightTypes.h"
#include "Input/FPVStickProcessor.h"

class FFPVHudCanvas;

struct FFPVDebugAxis
{
	FString Name;
	float Value = 0.0f;
};

struct FFPVDebugButton
{
	FString Name;
	bool bDown = false;
};

struct FFPVInputDebugData
{
	/** Raw key values from the player input (PlayerController::GetInputAnalogKeyState). */
	TArray<FFPVDebugAxis> RawAxes;
	/** Raw button states (PlayerController::IsInputKeyDown). */
	TArray<FFPVDebugButton> Buttons;
	FString LastPressedButton;

	/** Enhanced Input action values the flight code reads. */
	FFPVStickValues ActionSticks;
	/** After dead zone, inversion, curve and smoothing. */
	FFPVStickValues ShapedSticks;

	FFPVPilotCommand Command;
	FFPVFlightTelemetry Telemetry;
	bool bHasDrone = false;

	FString StickModeText;
	FString ThrottleModeText;
	float HoverThrottle = 0.0f;
	float PhysicsHz = 0.0f;
	float Fps = 0.0f;
};

namespace FPVInputDebugRenderer
{
	void Draw(const FFPVHudCanvas& Canvas, const FFPVInputDebugData& Data);
}
