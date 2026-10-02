// Global default game mode (see Config/DefaultEngine.ini). Works in any level, including an empty
// one: it spawns the test world itself, then spawns and possesses the drone without needing a
// PlayerStart. Free-fly for now; race/time-trial modes can subclass this and override the virtual
// hooks (SpawnWorldContent, GetDroneSpawnTransform, RespawnDrone) and listen to UFPVGameplayEvents.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPVGameMode.generated.h"

UCLASS(Config = Game)
class FPVDRONE_API AFPVGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPVGameMode();

	virtual void StartPlay() override;

	/** Spawns the drone at GetDroneSpawnTransform() (never needs a PlayerStart). */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Puts the controller's drone back at the spawn point (or spawns a new one if it has none). */
	virtual void RespawnDrone(AController* Controller);

	/** Spawn point: the level's first PlayerStart if enabled and present, else the built-in pad. */
	virtual FTransform GetDroneSpawnTransform(AController* Controller) const;

protected:
	/** Spawns the runtime test environment and lighting. Override for other modes/levels. */
	virtual void SpawnWorldContent();

	/** Spawn the runtime test environment (ground, hills, buildings, trees, gates). */
	UPROPERTY(Config, EditAnywhere, Category = "FPV")
	bool bSpawnTestEnvironment = true;

	/** Add sun / sky atmosphere / sky light / height fog when the level has none. */
	UPROPERTY(Config, EditAnywhere, Category = "FPV")
	bool bSpawnLightingIfMissing = true;

	/** Use the level's PlayerStart (if any) as the spawn point. */
	UPROPERTY(Config, EditAnywhere, Category = "FPV")
	bool bUseLevelPlayerStart = true;

	/** Built-in spawn point (on the test environment's launch pad). */
	UPROPERTY(EditAnywhere, Category = "FPV")
	FVector DefaultSpawnLocation = FVector(0.0, 0.0, 25.0);

	UPROPERTY(EditAnywhere, Category = "FPV")
	FRotator DefaultSpawnRotation = FRotator::ZeroRotator;

	/** Raised before respawning (so an upside-down drone isn't spawned into the ground). */
	UPROPERTY(EditAnywhere, Category = "FPV")
	float RespawnHeightOffsetCm = 0.0f;
};
