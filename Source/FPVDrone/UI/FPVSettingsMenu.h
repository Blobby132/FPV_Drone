// Pause / settings menu: a stack of pages, fully navigable with the controller.
//   D-pad or left stick: move (up/down) and change values (left/right, hold to speed up)
//   Cross: select / toggle / rebind     Circle: back     Options: resume
//
// The menu edits a working copy of the settings and applies every change immediately (the drone,
// camera and input update live). Drawing is done by FPVMenuRenderer from a read-only FFPVMenuView.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "Settings/FPVSettingsTypes.h"
#include "FPVSettingsMenu.generated.h"

class AFPVPlayerController;

enum class EFPVMenuItemType : uint8
{
	/** Runs OnActivate when confirmed. */
	Action,
	/** Opens another page. */
	Page,
	Float,
	Int,
	Bool,
	/** Enum-like value cycling through ChoiceLabels (stored as uint8). */
	Choice,
	/** Gamepad button binding for one EFPVButtonAction. */
	Binding,
	/** Read-only line (computed text). Not selectable. */
	Info
};

/** One row on a menu page. Value pointers point into UFPVSettingsMenu::Working. */
struct FFPVMenuItem
{
	EFPVMenuItemType Type = EFPVMenuItemType::Info;
	FString Label;
	FString Description;

	float* FloatValue = nullptr;
	int32* IntValue = nullptr;
	bool* BoolValue = nullptr;
	uint8* ChoiceValue = nullptr;
	TArray<FString> ChoiceLabels;

	float Min = 0.0f;
	float Max = 1.0f;
	float Step = 0.1f;
	/** Shown value = stored value * DisplayScale (e.g. 100 to show 0..1 as a percentage). */
	float DisplayScale = 1.0f;
	int32 Decimals = 2;
	FString Unit;

	int32 TargetPage = INDEX_NONE;
	EFPVButtonAction BindingAction = EFPVButtonAction::Count;
	TFunction<void()> OnActivate;
	/** Optional computed text (Info rows, or the value text of Action rows). */
	TFunction<FString()> DynamicText;

	bool IsSelectable() const { return Type != EFPVMenuItemType::Info; }
};

struct FFPVMenuPage
{
	FString Title;
	TArray<FFPVMenuItem> Items;
};

/** Read-only snapshot of the menu for drawing. */
struct FFPVMenuViewRow
{
	FString Label;
	FString Value;
	bool bSelectable = true;
	bool bSelected = false;
	/** 0..1 position for numeric values (slider), or < 0 for none. */
	float SliderFraction = -1.0f;
};

struct FFPVMenuView
{
	FString Title;
	TArray<FFPVMenuViewRow> Rows;
	int32 SelectedRow = 0;
	FString Description;
	FString Hints;
	FString Status;
	bool bCapturing = false;
	FString CapturePrompt;
};

/** Held navigation state, sampled by the player controller every frame. */
struct FFPVMenuInput
{
	bool bUp = false;
	bool bDown = false;
	bool bLeft = false;
	bool bRight = false;
	float StickX = 0.0f;
	float StickY = 0.0f;
};

UCLASS()
class FPVDRONE_API UFPVSettingsMenu : public UObject
{
	GENERATED_BODY()

public:
	/** Builds all pages. Call once after creation. */
	void Initialize(AFPVPlayerController* InOwner);

	/** Copies the active settings into the working copy and shows the main page. */
	void Open();
	void Close();
	bool IsOpen() const { return bOpen; }
	bool IsCapturingBinding() const { return bCapturing; }

	/** Navigation repeat and rebind capture. RealDeltaSeconds must be real time (the game is paused). */
	void Tick(float RealDeltaSeconds, const FFPVMenuInput& Input);

	/** Cross */
	void Confirm();
	/** Circle. Returns true if the menu wants to close (back from the main page). */
	bool Back();

	void BuildView(FFPVMenuView& OutView) const;

private:
	struct FRepeatState
	{
		int32 Direction = 0;
		float HeldTime = 0.0f;
		float NextRepeat = 0.0f;
	};

	// ---- Page construction ------------------------------------------------------------------
	void BuildPages();
	int32 AddPage(const FString& Title);
	FFPVMenuItem& AddItem(int32 Page, EFPVMenuItemType Type, const FString& Label, const FString& Description);
	void AddFloat(int32 Page, const FString& Label, float* Value, float Min, float Max, float Step, int32 Decimals,
		const FString& Unit, const FString& Description, float DisplayScale = 1.0f);
	void AddInt(int32 Page, const FString& Label, int32* Value, int32 Min, int32 Max, int32 Step, const FString& Unit, const FString& Description);
	void AddBool(int32 Page, const FString& Label, bool* Value, const FString& Description);
	void AddChoice(int32 Page, const FString& Label, uint8* Value, const TArray<FString>& Labels, const FString& Description);
	void AddPageLink(int32 Page, const FString& Label, int32 TargetPage, const FString& Description);
	void AddAction(int32 Page, const FString& Label, TFunction<void()> OnActivate, const FString& Description, TFunction<FString()> DynamicText = nullptr);
	void AddInfo(int32 Page, TFunction<FString()> DynamicText);
	void AddBinding(int32 Page, EFPVButtonAction Action);
	void AddRatesItems(int32 Page, const FString& AxisName, FFPVAxisRates& Rates);
	void AddPidItems(int32 Page, const FString& AxisName, FFPVPidGains& Gains);

	// ---- Behavior ---------------------------------------------------------------------------
	FFPVMenuPage& CurrentPage();
	const FFPVMenuPage& CurrentPage() const;
	int32 GetSelected() const;
	void SetSelected(int32 Index);
	void MoveSelection(int32 Direction);
	void AdjustSelected(int32 Direction, int32 Multiplier);
	void PushPage(int32 Page);
	/** Applies the working copy to the game (and re-syncs it with the sanitized result). */
	void ApplyWorking();
	void ShowStatus(const FString& Message, float Seconds = 2.5f);

	void StartCapture(EFPVButtonAction Action);
	void TickCapture(float RealDeltaSeconds);
	void FinishCapture(const FKey& Key);

	/** Returns how many steps to apply this frame (0 or 1) and the acceleration multiplier. */
	static int32 UpdateRepeat(FRepeatState& State, int32 Direction, float DeltaSeconds, float RepeatInterval, int32& OutMultiplier);

	FString FormatValue(const FFPVMenuItem& Item) const;
	FString BuildHints(const FFPVMenuItem* Item) const;

	/** Working copy edited by the menu (item pointers point into this). */
	UPROPERTY(Transient)
	FFPVUserSettings Working;

	TWeakObjectPtr<AFPVPlayerController> Owner;
	TArray<FFPVMenuPage> Pages;

	/** Open pages (index into Pages) and the selected row on each. */
	TArray<int32> PageStack;
	TArray<int32> SelectionStack;

	bool bOpen = false;
	FRepeatState VerticalRepeat;
	FRepeatState HorizontalRepeat;

	bool bCapturing = false;
	EFPVButtonAction CaptureAction = EFPVButtonAction::Count;
	float CaptureTimeLeft = 0.0f;
	/** Frames to wait before accepting a captured press (skips the Cross press that started it). */
	int32 CaptureArmFrames = 0;

	FString StatusMessage;
	float StatusTimeLeft = 0.0f;

	/** "Restore defaults" needs a second press within a few seconds. */
	float ConfirmDefaultsTimeLeft = 0.0f;

	int32 MainPage = INDEX_NONE;
};
