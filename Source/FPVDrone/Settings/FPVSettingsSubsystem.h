// Owns the active user settings for the whole game session, notifies listeners when they change,
// and saves/loads them as JSON in <Project>/Saved/FPVDrone/Settings.json.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVSettingsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FFPVOnSettingsChanged, const FFPVUserSettings& /*NewSettings*/);

UCLASS()
class FPVDRONE_API UFPVSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Loads the settings file (or writes one with the defaults on first run). */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Convenience accessor; returns nullptr if there is no game instance (e.g. editor preview worlds). */
	static UFPVSettingsSubsystem* Get(const UObject* WorldContextObject);

	const FFPVUserSettings& GetSettings() const { return Settings; }

	/** Replace the active settings (values are sanitized) and notify listeners. Does not save. */
	void SetSettings(const FFPVUserSettings& NewSettings);

	/** Write the active settings to disk. Returns false on failure. */
	bool SaveToDisk();

	/** Re-read the settings file and apply it (notifies listeners). Returns false if there is no valid file. */
	bool LoadFromDisk();

	/** Apply the built-in defaults (notifies listeners). Does not save. */
	void ResetToDefaults();

	/** True if the active settings changed since they were last saved or loaded. */
	bool HasUnsavedChanges() const { return bDirty; }

	/** Absolute path of the settings file. */
	static FString GetSettingsFilePath();

	/** Broadcast whenever the active settings change. */
	FFPVOnSettingsChanged OnSettingsChanged;

private:
	/** Parse a settings file into OutSettings (starting from defaults, so missing fields keep default values). */
	static bool ReadSettingsFile(FFPVUserSettings& OutSettings);

	UPROPERTY()
	FFPVUserSettings Settings;

	bool bDirty = false;
};
