#include "Feedback/FPVControllerFeedback.h"

#include "GameFramework/PlayerController.h"

void UFPVControllerFeedback::PlayImpact(APlayerController* Controller, float ImpactSpeedMps, const FFPVFeedbackSettings& Settings) const
{
	if (Controller == nullptr || !Settings.bRumbleEnabled || Settings.RumbleStrength <= 0.0f)
	{
		return;
	}

	const float Threshold = FMath::Max(Settings.ImpactThresholdMps, 0.1f);
	if (ImpactSpeedMps < Threshold)
	{
		return;
	}

	const float Severity = FMath::Clamp((ImpactSpeedMps - Threshold) / (Threshold * 3.0f), 0.0f, 1.0f);
	const float Intensity = FMath::Lerp(0.3f, 1.0f, Severity) * FMath::Clamp(Settings.RumbleStrength, 0.0f, 1.0f);
	const float Duration = FMath::Lerp(0.12f, 0.45f, Severity);

	// NEEDS VERIFICATION in 5.8: the native (non-latent) overload of PlayDynamicForceFeedback.
	// It needs no force feedback asset; all four motors (large/small, left/right) are used.
	Controller->PlayDynamicForceFeedback(Intensity, Duration, true, true, true, true);
}
