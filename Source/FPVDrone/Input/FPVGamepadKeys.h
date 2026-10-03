// The gamepad keys the project cares about, with PlayStation-style names
// (Cross, Circle, L1, Options, ...). Used by the input debug overlay and button rebinding.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

namespace FPVGamepadKeys
{
	/** Every digital gamepad button (face buttons, D-pad, shoulders, triggers, stick clicks, Create/Options). */
	const TArray<FKey>& GetButtons();

	/** Analog axes shown on the input debug overlay (sticks and triggers). */
	const TArray<FKey>& GetAxes();

	/** "Cross", "L1", "D-pad Up", ... for gamepad keys; the engine's display name for anything else. */
	FString GetDisplayName(const FKey& Key);
	FString GetDisplayName(FName KeyName);
}
