// World-level event bus for gameplay. Systems announce what happened here and anything interested
// subscribes, so gates, lap timers, race rules, scoring or rumble can be added without the drone,
// the triggers or the game mode knowing about each other.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FPVGameplayEvents.generated.h"

class AActor;
class AFPVDronePawn;
class UFPVPassThroughTriggerComponent;

/** Describes a collision of the drone with the world. */
USTRUCT(BlueprintType)
struct FFPVImpactInfo
{
	GENERATED_BODY()

	/** How hard the hit was: the drone's speed change into the surface (m/s). */
	UPROPERTY(BlueprintReadOnly, Category = "FPV")
	float ImpactSpeedMps = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FPV")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "FPV")
	FVector Normal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "FPV")
	TObjectPtr<AActor> OtherActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFPVOnTriggerPassedSignature, AFPVDronePawn*, Drone, UFPVPassThroughTriggerComponent*, Trigger, bool, bForward);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPVOnDroneImpactSignature, AFPVDronePawn*, Drone, const FFPVImpactInfo&, Impact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFPVOnDroneResetSignature, AFPVDronePawn*, Drone);

UCLASS()
class FPVDRONE_API UFPVGameplayEvents : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns nullptr if the object has no world. */
	static UFPVGameplayEvents* Get(const UObject* WorldContextObject);

	/** A drone flew completely through a pass-through trigger (gate). bForward = along the trigger's +X axis. */
	UPROPERTY(BlueprintAssignable, Category = "FPV|Events")
	FFPVOnTriggerPassedSignature OnTriggerPassed;

	/** A drone hit something hard enough to count as an impact. */
	UPROPERTY(BlueprintAssignable, Category = "FPV|Events")
	FFPVOnDroneImpactSignature OnDroneImpact;

	/** A drone was reset / respawned. */
	UPROPERTY(BlueprintAssignable, Category = "FPV|Events")
	FFPVOnDroneResetSignature OnDroneReset;
};
