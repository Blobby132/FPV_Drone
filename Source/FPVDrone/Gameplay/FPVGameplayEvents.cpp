#include "Gameplay/FPVGameplayEvents.h"

#include "Engine/World.h"

UFPVGameplayEvents* UFPVGameplayEvents::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UFPVGameplayEvents>() : nullptr;
}
