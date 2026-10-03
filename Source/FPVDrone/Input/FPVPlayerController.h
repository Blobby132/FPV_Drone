// Owns all player input: creates the Enhanced Input objects at runtime, turns the thumbsticks into
// pilot commands for the drone, and handles the utility buttons (mode, reset, camera, menu, ...).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/FPVStickProcessor.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVPlayerController.generated.h"

class AFPVDronePawn;
class UFPVInputConfig;

UCLASS()
class FPVDRONE_API AFPVPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFPVPlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	AFPVDronePawn* GetDrone() const;

	/** Raw stick values read this frame (before any processing). */
	const FFPVStickValues& GetRawSticks() const { return RawSticks; }

	const FFPVStickProcessor& GetStickProcessor() const { return StickProcessor; }

	/** The active settings (from UFPVSettingsSubsystem, or defaults if unavailable). */
	const FFPVUserSettings& GetSettings() const;

	/** Replace the active settings (goes through the settings subsystem so every listener updates). */
	void UpdateSettings(const FFPVUserSettings& NewSettings);

	bool IsInputDebugVisible() const { return bShowInputDebug; }

	/** Most recently pressed gamepad button (for the input debug overlay). */
	const FKey& GetLastPressedKey() const { return LastPressedKey; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void EnsureInputConfig();
	void AddFlightMappingContext();
	FFPVStickValues ReadRawSticks() const;
	void UpdateFlightInput(float DeltaTime);

	void HandleSettingsChanged(const FFPVUserSettings& NewSettings);
	void ApplySettingsToDrone(const FFPVUserSettings& Settings, bool bResetFlightMode);

	void TrackLastPressedKey();
	void AdjustCameraTilt(float DeltaDegrees);

	// ---- Button handlers --------------------------------------------------------------------
	void OnToggleFlightMode();
	void OnResetDrone();
	void OnToggleCamera();
	void OnCameraTiltUp();
	void OnCameraTiltDown();
	void OnToggleInputDebug();

	UPROPERTY(Transient)
	TObjectPtr<UFPVInputConfig> InputConfig;

	FFPVStickProcessor StickProcessor;
	FFPVStickValues RawSticks;
	bool bShowInputDebug = false;
	FKey LastPressedKey;
	FDelegateHandle SettingsChangedHandle;

	/** Used only if the settings subsystem is unavailable. */
	FFPVUserSettings FallbackSettings;
};
