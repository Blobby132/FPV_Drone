// Controller feedback (rumble). Kept separate from input and flight code so DualSense extras
// (adaptive triggers, lightbar) can be added here later without touching anything else.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVControllerFeedback.generated.h"

class APlayerController;

UCLASS()
class FPVDRONE_API UFPVControllerFeedback : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Rumble for an impact. Nothing happens below Settings.ImpactThresholdMps; above it, strength
	 * and duration grow with the impact speed (full strength at 4x the threshold).
	 */
	void PlayImpact(APlayerController* Controller, float ImpactSpeedMps, const FFPVFeedbackSettings& Settings) const;
};
