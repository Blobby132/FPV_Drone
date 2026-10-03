#include "Settings/FPVSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "FPVDrone.h"

void UFPVSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Settings = FFPVUserSettings();
	Settings.Sanitize();

	FFPVUserSettings Loaded;
	if (ReadSettingsFile(Loaded))
	{
		Settings = Loaded;
		UE_LOG(LogFPVDrone, Log, TEXT("Loaded settings from %s"), *GetSettingsFilePath());
	}
	else
	{
		// First run (or unreadable file): write the defaults so there is a file to inspect/edit.
		SaveToDisk();
	}
	bDirty = false;
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
	bDirty = true;
	OnSettingsChanged.Broadcast(Settings);
}

FString UFPVSettingsSubsystem::GetSettingsFilePath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("FPVDrone"), TEXT("Settings.json")));
}

bool UFPVSettingsSubsystem::SaveToDisk()
{
	FString Json;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Settings, Json))
	{
		UE_LOG(LogFPVDrone, Error, TEXT("Could not serialize settings to JSON."));
		return false;
	}

	const FString Path = GetSettingsFilePath();
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	if (!FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogFPVDrone, Error, TEXT("Could not write settings file %s"), *Path);
		return false;
	}

	bDirty = false;
	UE_LOG(LogFPVDrone, Log, TEXT("Saved settings to %s"), *Path);
	return true;
}

bool UFPVSettingsSubsystem::ReadSettingsFile(FFPVUserSettings& OutSettings)
{
	const FString Path = GetSettingsFilePath();
	FString Json;
	if (!FPaths::FileExists(Path) || !FFileHelper::LoadFileToString(Json, *Path))
	{
		return false;
	}

	// Start from defaults: fields missing from the file (e.g. added in a newer version) keep their defaults.
	FFPVUserSettings Parsed;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Parsed, 0, 0))
	{
		UE_LOG(LogFPVDrone, Warning, TEXT("Settings file %s is not valid JSON; using defaults."), *Path);
		return false;
	}

	// Future format migrations go here (compare Parsed.Version with FFPVUserSettings().Version).
	Parsed.Version = FFPVUserSettings().Version;
	Parsed.Sanitize();
	OutSettings = Parsed;
	return true;
}

bool UFPVSettingsSubsystem::LoadFromDisk()
{
	FFPVUserSettings Loaded;
	if (!ReadSettingsFile(Loaded))
	{
		return false;
	}
	Settings = Loaded;
	bDirty = false;
	OnSettingsChanged.Broadcast(Settings);
	return true;
}

void UFPVSettingsSubsystem::ResetToDefaults()
{
	SetSettings(FFPVUserSettings());
}
