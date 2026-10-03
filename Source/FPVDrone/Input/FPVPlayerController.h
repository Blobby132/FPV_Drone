// Owns all player input: creates the Enhanced Input objects at runtime, turns the thumbsticks into
// pilot commands for the drone, handles the utility buttons (mode, reset, camera, menu, ...),
// runs the pause/settings menu and plays controller feedback.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/FPVGameplayEvents.h"
#include "Input/FPVStickProcessor.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVPlayerController.generated.h"

class AFPVDronePawn;
class UFPVControllerFeedback;
class UFPVInputConfig;
class UFPVPassThroughTriggerComponent;
class UFPVSettingsMenu;

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

	// ---- Pause / settings menu --------------------------------------------------------------
	bool IsMenuOpen() const;
	const UFPVSettingsMenu* GetMenu() const { return Menu; }
	/** Pauses the game and switches input to the menu. */
	void OpenMenu();
	/** Saves unsaved settings, resumes the game and switches input back to flying. */
	void CloseMenu();

	/** Respawn the drone at the spawn point (Circle / menu). */
	void RequestResetDrone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void EnsureInputConfig();
	void AddFlightMappingContext();
	/** Swap between the flight and menu mapping contexts. */
	void SetMenuInputActive(bool bMenuActive);
	FFPVStickValues ReadRawSticks() const;
	void UpdateFlightInput(float DeltaTime);
	void UpdateMenuInput(float RealDeltaSeconds);

	void HandleSettingsChanged(const FFPVUserSettings& NewSettings);
	void ApplySettingsToDrone(const FFPVUserSettings& Settings, bool bResetFlightMode);
	/** Rebuild the mapping contexts if the button bindings changed. */
	void RefreshInputBindings(const FFPVButtonBindings& Bindings);

	void TrackLastPressedKey();
	void AdjustCameraTilt(float DeltaDegrees);
	void ShowFlashMessage(const FString& Message);

	/**
	 * True while the menu is open and for a moment after it closes, so the button that closed
	 * the menu (e.g. Circle = back) doesn't also trigger its flight action (Circle = reset).
	 */
	bool AreFlightButtonsBlocked() const;
	/** True right after the menu opened, so the press that opened it isn't also handled by the menu. */
	bool AreMenuButtonsBlocked() const;

	// ---- Flight button handlers ---------------------------------------------------------------
	void OnToggleFlightMode();
	void OnResetDrone();
	void OnToggleCamera();
	void OnCameraTiltUp();
	void OnCameraTiltDown();
	void OnToggleInputDebug();
	void OnOpenMenu();

	// ---- Menu button handlers -----------------------------------------------------------------
	void OnMenuConfirm();
	void OnMenuBack();
	void OnMenuClose();

	// ---- Gameplay events ----------------------------------------------------------------------
	UFUNCTION()
	void HandleTriggerPassed(AFPVDronePawn* Drone, UFPVPassThroughTriggerComponent* Trigger, bool bForward);

	UFUNCTION()
	void HandleDroneImpact(AFPVDronePawn* Drone, const FFPVImpactInfo& Impact);

	UPROPERTY(Transient)
	TObjectPtr<UFPVInputConfig> InputConfig;

	UPROPERTY(Transient)
	TObjectPtr<UFPVSettingsMenu> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UFPVControllerFeedback> Feedback;

	FFPVStickProcessor StickProcessor;
	FFPVStickValues RawSticks;
	bool bShowInputDebug = false;
	FKey LastPressedKey;
	FString FlashMessage;
	double FlashMessageTime = -1.0;
	double MenuOpenedTime = -1.0;
	double MenuClosedTime = -1.0;

	/** Bindings the mapping contexts were last built with. */
	FFPVButtonBindings AppliedBindings;

	FDelegateHandle SettingsChangedHandle;

	/** Used only if the settings subsystem is unavailable. */
	FFPVUserSettings FallbackSettings;
};
