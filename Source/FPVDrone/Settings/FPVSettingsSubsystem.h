// Owns the active user settings for the whole game session and notifies listeners when they change.

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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Convenience accessor; returns nullptr if there is no game instance (e.g. editor preview worlds). */
	static UFPVSettingsSubsystem* Get(const UObject* WorldContextObject);

	const FFPVUserSettings& GetSettings() const { return Settings; }

	/** Replace the active settings (values are sanitized) and notify listeners. */
	void SetSettings(const FFPVUserSettings& NewSettings);

	/** Broadcast whenever the active settings change. */
	FFPVOnSettingsChanged OnSettingsChanged;

private:
	UPROPERTY()
	FFPVUserSettings Settings;
};
