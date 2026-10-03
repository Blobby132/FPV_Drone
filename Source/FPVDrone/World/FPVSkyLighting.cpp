#include "World/FPVSkyLighting.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"

AFPVSkyLighting::AFPVSkyLighting()
{
	PrimaryActorTick.bCanEverTick = false;

	// Everything is fully dynamic (the project has static lighting disabled).
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	// Sun: ~10 lux is a typical outdoor value with UE5's physical units; 40 degrees above the horizon.
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-40.0, 35.0, 0.0));
	Sun->Intensity = 10.0f;
	Sun->SetAtmosphereSunLight(true);
	Sun->SetCastShadows(true);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	Atmosphere->SetupAttachment(Root);

	// Real-time capture so the sky light follows the atmosphere (no baking needed).
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.01f);
	Fog->SetFogHeightFalloff(0.2f);
}

void AFPVSkyLighting::Configure(bool bKeepSun, bool bKeepAtmosphere, bool bKeepSkyLight, bool bKeepFog)
{
	if (!bKeepSun && Sun != nullptr)
	{
		Sun->DestroyComponent();
		Sun = nullptr;
	}
	if (!bKeepAtmosphere && Atmosphere != nullptr)
	{
		Atmosphere->DestroyComponent();
		Atmosphere = nullptr;
	}
	if (!bKeepSkyLight && SkyLight != nullptr)
	{
		SkyLight->DestroyComponent();
		SkyLight = nullptr;
	}
	if (!bKeepFog && Fog != nullptr)
	{
		Fog->DestroyComponent();
		Fog = nullptr;
	}
}
