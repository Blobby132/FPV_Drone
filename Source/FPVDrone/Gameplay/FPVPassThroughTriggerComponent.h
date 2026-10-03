// A box volume that reports when a drone flies all the way through it (enters on one side and
// leaves on the other). Attach it to anything: gates, windows, finish lines. Reports go to the
// component's own OnPassed delegate and to UFPVGameplayEvents::OnTriggerPassed.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Gameplay/FPVGameplayEvents.h"
#include "FPVPassThroughTriggerComponent.generated.h"

UCLASS(ClassGroup = (FPV), meta = (BlueprintSpawnableComponent))
class FPVDRONE_API UFPVPassThroughTriggerComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UFPVPassThroughTriggerComponent();

	/** Position of this trigger in a course (e.g. gate number). -1 = not part of a course. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV")
	int32 TriggerIndex = -1;

	/** Fired when a drone passes through. bForward = travelling along this component's +X axis. */
	UPROPERTY(BlueprintAssignable, Category = "FPV|Events")
	FFPVOnTriggerPassedSignature OnPassed;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/**
	 * Which side of the trigger plane (local X) the actor is on, -1 or +1. Uses the velocity when
	 * the actor is moving through quickly, because at high speed the overlap may only be detected
	 * once the drone is already past the middle.
	 */
	float GetSide(const AActor* Actor, bool bEntering) const;

	/** Side each overlapping drone entered from. */
	TMap<TWeakObjectPtr<AActor>, float> EntrySides;
};
