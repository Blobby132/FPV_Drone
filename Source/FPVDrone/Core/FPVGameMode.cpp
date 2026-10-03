#include "Core/FPVGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#include "Drone/FPVDronePawn.h"
#include "Input/FPVPlayerController.h"
#include "UI/FPVHUD.h"
#include "World/FPVTestEnvironment.h"
#include "FPVDrone.h"

AFPVGameMode::AFPVGameMode()
{
	DefaultPawnClass = AFPVDronePawn::StaticClass();
	PlayerControllerClass = AFPVPlayerController::StaticClass();
	HUDClass = AFPVHUD::StaticClass();
	bStartPlayersAsSpectators = false;
}

void AFPVGameMode::StartPlay()
{
	// Spawn the world before actors receive BeginPlay.
	SpawnWorldContent();
	Super::StartPlay();
}

void AFPVGameMode::SpawnWorldContent()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (bSpawnTestEnvironment)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AFPVTestEnvironment* Environment = World->SpawnActor<AFPVTestEnvironment>(
			AFPVTestEnvironment::StaticClass(), FTransform::Identity, SpawnParams);
		if (Environment != nullptr)
		{
			Environment->BuildEnvironment();
		}
		else
		{
			UE_LOG(LogFPVDrone, Error, TEXT("Failed to spawn the test environment."));
		}
	}
}

FTransform AFPVGameMode::GetDroneSpawnTransform(AController* Controller) const
{
	if (bUseLevelPlayerStart)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				// Level only; keep the drone upright (yaw only).
				const APlayerStart* Start = *It;
				return FTransform(FRotator(0.0, Start->GetActorRotation().Yaw, 0.0), Start->GetActorLocation());
			}
		}
	}
	return FTransform(DefaultSpawnRotation, DefaultSpawnLocation);
}

void AFPVGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	RestartPlayerAtTransform(NewPlayer, GetDroneSpawnTransform(NewPlayer));
}

void AFPVGameMode::RespawnDrone(AController* Controller)
{
	if (Controller == nullptr)
	{
		return;
	}

	FTransform SpawnTransform = GetDroneSpawnTransform(Controller);
	SpawnTransform.AddToTranslation(FVector(0.0, 0.0, RespawnHeightOffsetCm));

	if (AFPVDronePawn* Drone = Cast<AFPVDronePawn>(Controller->GetPawn()))
	{
		Drone->ResetDrone(SpawnTransform);
	}
	else
	{
		RestartPlayerAtTransform(Controller, SpawnTransform);
	}
}
