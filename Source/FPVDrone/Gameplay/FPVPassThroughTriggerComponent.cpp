#include "Gameplay/FPVPassThroughTriggerComponent.h"

#include "Drone/FPVDronePawn.h"
#include "FPVDrone.h"

namespace FPVTriggerConstants
{
	/** Below this speed through the plane (cm/s) the position decides the side instead of the velocity. */
	inline constexpr double MinCrossingSpeed = 50.0;
}

UFPVPassThroughTriggerComponent::UFPVPassThroughTriggerComponent()
{
	// Query-only overlap volume: never blocks, never simulates.
	SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	InitBoxExtent(FVector(75.0, 100.0, 100.0));
}

void UFPVPassThroughTriggerComponent::BeginPlay()
{
	Super::BeginPlay();
	OnComponentBeginOverlap.AddDynamic(this, &UFPVPassThroughTriggerComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UFPVPassThroughTriggerComponent::HandleEndOverlap);
}

float UFPVPassThroughTriggerComponent::GetSide(const AActor* Actor, bool bEntering) const
{
	const FTransform& Transform = GetComponentTransform();
	const FVector LocalVelocity = Transform.InverseTransformVectorNoScale(Actor->GetVelocity());
	if (FMath::Abs(LocalVelocity.X) > FPVTriggerConstants::MinCrossingSpeed)
	{
		// Moving along +X: entered from the -X side and leaves on the +X side.
		const float Direction = LocalVelocity.X > 0.0 ? 1.0f : -1.0f;
		return bEntering ? -Direction : Direction;
	}
	const FVector LocalPosition = Transform.InverseTransformPosition(Actor->GetActorLocation());
	return LocalPosition.X >= 0.0 ? 1.0f : -1.0f;
}

void UFPVPassThroughTriggerComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AFPVDronePawn>(OtherActor) == nullptr)
	{
		return;
	}
	EntrySides.Add(OtherActor, GetSide(OtherActor, true));
}

void UFPVPassThroughTriggerComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AFPVDronePawn* Drone = Cast<AFPVDronePawn>(OtherActor);
	if (Drone == nullptr)
	{
		return;
	}

	float EntrySide = 0.0f;
	if (!EntrySides.RemoveAndCopyValue(OtherActor, EntrySide))
	{
		return;
	}

	// Passed through only if it left on the opposite side from where it came in.
	const float ExitSide = GetSide(OtherActor, false);
	if (ExitSide == EntrySide)
	{
		return;
	}

	const bool bForward = ExitSide > 0.0f;
	UE_LOG(LogFPVDrone, Verbose, TEXT("Trigger %d passed (%s)."), TriggerIndex, bForward ? TEXT("forward") : TEXT("backward"));
	OnPassed.Broadcast(Drone, this, bForward);
	if (UFPVGameplayEvents* Events = UFPVGameplayEvents::Get(this))
	{
		Events->OnTriggerPassed.Broadcast(Drone, this, bForward);
	}
}
