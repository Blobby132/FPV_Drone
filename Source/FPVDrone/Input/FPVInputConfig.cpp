#include "Input/FPVInputConfig.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "FPVDrone.h"

namespace FPVInputConfigNames
{
	const TCHAR* GetButtonActionObjectName(EFPVButtonAction Action)
	{
		switch (Action)
		{
		case EFPVButtonAction::ToggleFlightMode:	return TEXT("IA_ToggleFlightMode");
		case EFPVButtonAction::ResetDrone:			return TEXT("IA_ResetDrone");
		case EFPVButtonAction::ToggleCamera:		return TEXT("IA_ToggleCamera");
		case EFPVButtonAction::CameraTiltUp:		return TEXT("IA_CameraTiltUp");
		case EFPVButtonAction::CameraTiltDown:		return TEXT("IA_CameraTiltDown");
		case EFPVButtonAction::OpenMenu:			return TEXT("IA_OpenMenu");
		case EFPVButtonAction::ToggleInputDebug:	return TEXT("IA_ToggleInputDebug");
		default:									return TEXT("IA_Unknown");
		}
	}
}

void UFPVInputConfig::Initialize(const FFPVButtonBindings& Bindings)
{
	if (bInitialized)
	{
		RebuildMappings(Bindings);
		return;
	}
	bInitialized = true;

	// Flight sticks: raw axis values; dead zone / expo / inversion are applied in FFPVStickProcessor.
	LeftStickX = CreateAction(TEXT("IA_LeftStickX"), true, false);
	LeftStickY = CreateAction(TEXT("IA_LeftStickY"), true, false);
	RightStickX = CreateAction(TEXT("IA_RightStickX"), true, false);
	RightStickY = CreateAction(TEXT("IA_RightStickY"), true, false);

	ButtonActions.SetNum(static_cast<int32>(EFPVButtonAction::Count));
	for (int32 Index = 0; Index < ButtonActions.Num(); ++Index)
	{
		ButtonActions[Index] = CreateAction(FPVInputConfigNames::GetButtonActionObjectName(static_cast<EFPVButtonAction>(Index)), false, false);
	}

	// Menu actions must work while the game is paused.
	MenuUp = CreateAction(TEXT("IA_MenuUp"), false, true);
	MenuDown = CreateAction(TEXT("IA_MenuDown"), false, true);
	MenuLeft = CreateAction(TEXT("IA_MenuLeft"), false, true);
	MenuRight = CreateAction(TEXT("IA_MenuRight"), false, true);
	MenuConfirm = CreateAction(TEXT("IA_MenuConfirm"), false, true);
	MenuBack = CreateAction(TEXT("IA_MenuBack"), false, true);
	MenuClose = CreateAction(TEXT("IA_MenuClose"), false, true);
	MenuStickX = CreateAction(TEXT("IA_MenuStickX"), true, true);
	MenuStickY = CreateAction(TEXT("IA_MenuStickY"), true, true);

	FlightContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FPVFlight"));
	MenuContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FPVMenu"));

	RebuildMappings(Bindings);
}

void UFPVInputConfig::RebuildMappings(const FFPVButtonBindings& Bindings)
{
	if (!bInitialized)
	{
		return;
	}
	BuildFlightMappings(Bindings);
	BuildMenuMappings(Bindings);
}

UInputAction* UFPVInputConfig::GetButtonAction(EFPVButtonAction Action) const
{
	const int32 Index = static_cast<int32>(Action);
	return ButtonActions.IsValidIndex(Index) ? ButtonActions[Index].Get() : nullptr;
}

FKey UFPVInputConfig::GetKeyboardKey(EFPVButtonAction Action)
{
	switch (Action)
	{
	case EFPVButtonAction::ToggleFlightMode:	return EKeys::M;
	case EFPVButtonAction::ResetDrone:			return EKeys::R;
	case EFPVButtonAction::ToggleCamera:		return EKeys::C;
	case EFPVButtonAction::CameraTiltUp:		return EKeys::PageUp;
	case EFPVButtonAction::CameraTiltDown:		return EKeys::PageDown;
	case EFPVButtonAction::OpenMenu:			return EKeys::P;
	case EFPVButtonAction::ToggleInputDebug:	return EKeys::I;
	default:									return EKeys::Invalid;
	}
}

UInputAction* UFPVInputConfig::CreateAction(const TCHAR* Name, bool bAxis, bool bTriggerWhenPaused)
{
	// NEEDS VERIFICATION in 5.8: ValueType and bTriggerWhenPaused are public UPROPERTYs on UInputAction
	// (true since UE 5.0). Actions created this way have no triggers, i.e. the default "implicit Down"
	// behavior: Started on press, Triggered while held, Completed on release.
	UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
	Action->ValueType = bAxis ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
	Action->bTriggerWhenPaused = bTriggerWhenPaused;
	return Action;
}

void UFPVInputConfig::MapAxisKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key, bool bNegate)
{
	if (Context == nullptr || Action == nullptr || !Key.IsValid())
	{
		return;
	}
	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
	if (bNegate)
	{
		// Negate defaults to all axes; for a 1D action only X matters.
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
}

void UFPVInputConfig::BuildFlightMappings(const FFPVButtonBindings& Bindings)
{
	UInputMappingContext* Context = FlightContext;
	Context->UnmapAll();

	// Thumbsticks. Unreal reports stick Y as +1 when pushed up/forward.
	MapAxisKey(Context, LeftStickX, EKeys::Gamepad_LeftX, false);
	MapAxisKey(Context, LeftStickY, EKeys::Gamepad_LeftY, false);
	MapAxisKey(Context, RightStickX, EKeys::Gamepad_RightX, false);
	MapAxisKey(Context, RightStickY, EKeys::Gamepad_RightY, false);

	// Keyboard fallback for debugging only: WASD = left stick, arrow keys = right stick.
	MapAxisKey(Context, LeftStickY, EKeys::W, false);
	MapAxisKey(Context, LeftStickY, EKeys::S, true);
	MapAxisKey(Context, LeftStickX, EKeys::D, false);
	MapAxisKey(Context, LeftStickX, EKeys::A, true);
	MapAxisKey(Context, RightStickY, EKeys::Up, false);
	MapAxisKey(Context, RightStickY, EKeys::Down, true);
	MapAxisKey(Context, RightStickX, EKeys::Right, false);
	MapAxisKey(Context, RightStickX, EKeys::Left, true);

	// Utility buttons: the (rebindable) gamepad key plus a fixed keyboard key.
	for (int32 Index = 0; Index < ButtonActions.Num(); ++Index)
	{
		const EFPVButtonAction Action = static_cast<EFPVButtonAction>(Index);
		const FKey GamepadKey(Bindings.GetKeyName(Action));
		if (GamepadKey.IsValid())
		{
			Context->MapKey(ButtonActions[Index], GamepadKey);
		}
		else
		{
			UE_LOG(LogFPVDrone, Warning, TEXT("Binding for '%s' is not a valid key: %s"),
				*FPVSettingsText::GetButtonActionName(Action), *Bindings.GetKeyName(Action).ToString());
		}

		const FKey KeyboardKey = GetKeyboardKey(Action);
		if (KeyboardKey.IsValid())
		{
			Context->MapKey(ButtonActions[Index], KeyboardKey);
		}
	}
}

void UFPVInputConfig::BuildMenuMappings(const FFPVButtonBindings& Bindings)
{
	UInputMappingContext* Context = MenuContext;
	Context->UnmapAll();

	// D-pad and the left stick's virtual "direction buttons" navigate. The raw stick axes are
	// mapped too, in case a controller doesn't generate the virtual stick buttons.
	Context->MapKey(MenuUp, EKeys::Gamepad_DPad_Up);
	Context->MapKey(MenuUp, EKeys::Gamepad_LeftStick_Up);
	Context->MapKey(MenuUp, EKeys::Up);
	Context->MapKey(MenuDown, EKeys::Gamepad_DPad_Down);
	Context->MapKey(MenuDown, EKeys::Gamepad_LeftStick_Down);
	Context->MapKey(MenuDown, EKeys::Down);
	Context->MapKey(MenuLeft, EKeys::Gamepad_DPad_Left);
	Context->MapKey(MenuLeft, EKeys::Gamepad_LeftStick_Left);
	Context->MapKey(MenuLeft, EKeys::Left);
	Context->MapKey(MenuRight, EKeys::Gamepad_DPad_Right);
	Context->MapKey(MenuRight, EKeys::Gamepad_LeftStick_Right);
	Context->MapKey(MenuRight, EKeys::Right);

	MapAxisKey(Context, MenuStickX, EKeys::Gamepad_LeftX, false);
	MapAxisKey(Context, MenuStickY, EKeys::Gamepad_LeftY, false);

	// Cross = confirm, Circle = back (fixed, so the menu can never become unusable).
	Context->MapKey(MenuConfirm, EKeys::Gamepad_FaceButton_Bottom);
	Context->MapKey(MenuConfirm, EKeys::Enter);
	Context->MapKey(MenuConfirm, EKeys::SpaceBar);
	Context->MapKey(MenuBack, EKeys::Gamepad_FaceButton_Right);
	Context->MapKey(MenuBack, EKeys::BackSpace);

	// Whatever button opens the menu also closes it. Options is always accepted as a fallback.
	const FKey OpenMenuKey(Bindings.GetKeyName(EFPVButtonAction::OpenMenu));
	if (OpenMenuKey.IsValid())
	{
		Context->MapKey(MenuClose, OpenMenuKey);
	}
	if (OpenMenuKey != EKeys::Gamepad_Special_Right)
	{
		Context->MapKey(MenuClose, EKeys::Gamepad_Special_Right);
	}
	Context->MapKey(MenuClose, GetKeyboardKey(EFPVButtonAction::OpenMenu));
}
