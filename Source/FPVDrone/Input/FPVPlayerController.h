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
class UFPVPassThroughTriggerComponent;

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

	/** Short message to flash on screen (e.g. "GATE 3"); returns false when there is none. */
	bool GetFlashMessage(FString& OutMessage, float& OutAgeSeconds) const;

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
	void ShowFlashMessage(const FString& Message);

	UFUNCTION()
	void HandleTriggerPassed(AFPVDronePawn* Drone, UFPVPassThroughTriggerComponent* Trigger, bool bForward);

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
	FString FlashMessage;
	double FlashMessageTime = -1.0;
	FDelegateHandle SettingsChangedHandle;

	/** Used only if the settings subsystem is unavailable. */
	FFPVUserSettings FallbackSettings;
};
