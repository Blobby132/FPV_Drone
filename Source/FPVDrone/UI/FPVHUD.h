// Draws everything 2D with the canvas: the OSD, the input debug overlay and (later) the pause menu.
// Pure presentation: it reads state from the player controller and the drone, never changes it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPVHUD.generated.h"

class AFPVDronePawn;
class AFPVPlayerController;
struct FFPVOsdData;
struct FFPVInputDebugData;

UCLASS()
class FPVDRONE_API AFPVHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFPVHUD();

	virtual void DrawHUD() override;

private:
	void UpdateRates(const AFPVDronePawn* Drone);
	void BuildOsdData(const AFPVPlayerController& Controller, const AFPVDronePawn& Drone, FFPVOsdData& OutData) const;
	void BuildInputDebugData(const AFPVPlayerController& Controller, const AFPVDronePawn* Drone, FFPVInputDebugData& OutData) const;

	/** Measured physics steps per second (from the flight controller's step counter). */
	float PhysicsHz = 0.0f;
	float SmoothedFps = 0.0f;
	uint64 RateSampleSteps = 0;
	double RateSampleTime = 0.0;
	bool bHasRateSample = false;
};
