// Runtime lighting for levels that have none: sun (directional light), sky atmosphere, real-time
// captured sky light and exponential height fog. The game mode removes any part the level already
// provides, so this never doubles up lights in a lit level.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPVSkyLighting.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

UCLASS()
class FPVDRONE_API AFPVSkyLighting : public AActor
{
	GENERATED_BODY()

public:
	AFPVSkyLighting();

	/** Keep only the requested parts; the others are destroyed. */
	void Configure(bool bKeepSun, bool bKeepAtmosphere, bool bKeepSkyLight, bool bKeepFog);

private:
	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UExponentialHeightFogComponent> Fog;
};
