// Creates every Enhanced Input action and mapping context at runtime (no .uasset files).
//
// Two mapping contexts:
//   * Flight: the four stick axes, the utility buttons (rebindable) and a keyboard fallback.
//   * Menu:   navigation for the pause/settings menu. These actions trigger while paused.
// The player controller swaps contexts when the menu opens/closes.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class FPVDRONE_API UFPVInputConfig : public UObject
{
	GENERATED_BODY()

public:
	/** Create all actions and both contexts. Safe to call once; later binding changes use RebuildMappings(). */
	void Initialize(const FFPVButtonBindings& Bindings);

	/** Re-map both contexts for new button bindings. Call RequestRebuildControlMappings() on the subsystem afterwards. */
	void RebuildMappings(const FFPVButtonBindings& Bindings);

	UInputAction* GetButtonAction(EFPVButtonAction Action) const;

	/** Fixed keyboard key for a utility action (debug fallback; not rebindable). */
	static FKey GetKeyboardKey(EFPVButtonAction Action);

	// ---- Flight sticks (Axis1D, raw -1..1) --------------------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LeftStickX;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LeftStickY;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RightStickX;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RightStickY;

	// ---- Utility buttons (Boolean), indexed by EFPVButtonAction -----------------------------
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> ButtonActions;

	// ---- Menu (Boolean / Axis1D, trigger while paused) --------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuUp;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuDown;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuLeft;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuRight;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuConfirm;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuBack;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuClose;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuStickX;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MenuStickY;

	// ---- Contexts ---------------------------------------------------------------------------
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> FlightContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MenuContext;

private:
	UInputAction* CreateAction(const TCHAR* Name, bool bAxis, bool bTriggerWhenPaused);
	void MapAxisKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key, bool bNegate);
	void BuildFlightMappings(const FFPVButtonBindings& Bindings);
	void BuildMenuMappings(const FFPVButtonBindings& Bindings);

	bool bInitialized = false;
};
