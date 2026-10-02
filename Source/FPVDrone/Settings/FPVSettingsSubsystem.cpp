#include "Settings/FPVSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UFPVSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = FFPVUserSettings();
	Settings.Sanitize();
}

UFPVSettingsSubsystem* UFPVSettingsSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFPVSettingsSubsystem>() : nullptr;
}

void UFPVSettingsSubsystem::SetSettings(const FFPVUserSettings& NewSettings)
{
	Settings = NewSettings;
	Settings.Sanitize();
	OnSettingsChanged.Broadcast(Settings);
}
