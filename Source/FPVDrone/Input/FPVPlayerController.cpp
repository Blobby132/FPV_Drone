#include "Input/FPVPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Misc/App.h"

#include "Core/FPVGameMode.h"
#include "Drone/FPVDronePawn.h"
#include "Feedback/FPVControllerFeedback.h"
#include "Gameplay/FPVPassThroughTriggerComponent.h"
#include "Input/FPVGamepadKeys.h"
#include "Input/FPVInputConfig.h"
#include "Settings/FPVSettingsSubsystem.h"
#include "UI/FPVSettingsMenu.h"
#include "FPVDrone.h"

namespace FPVControllerConstants
{
	/** Real seconds during which buttons are ignored after the menu opens/closes (see AreFlightButtonsBlocked). */
	inline constexpr double MenuTransitionBlockSeconds = 0.25;

	/** Impacts at least this many times the rumble threshold also flash a message. */
	inline constexpr float ImpactMessageFactor = 2.5f;

	bool BindingsEqual(const FFPVButtonBindings& A, const FFPVButtonBindings& B)
	{
		for (int32 Index = 0; Index < static_cast<int32>(EFPVButtonAction::Count); ++Index)
		{
			const EFPVButtonAction Action = static_cast<EFPVButtonAction>(Index);
			if (A.GetKeyName(Action) != B.GetKeyName(Action))
			{
				return false;
			}
		}
		return true;
	}
}

AFPVPlayerController::AFPVPlayerController()
{
	// The pause menu is driven from PlayerTick, so keep ticking fully while the game is paused.
	bShouldPerformFullTickWhenPaused = true;
	bShowMouseCursor = false;
}

// =============================================================================================
// Settings
// =============================================================================================

const FFPVUserSettings& AFPVPlayerController::GetSettings() const
{
	if (const UFPVSettingsSubsystem* SettingsSubsystem = UFPVSettingsSubsystem::Get(this))
	{
		return SettingsSubsystem->GetSettings();
	}
	return FallbackSettings;
}

void AFPVPlayerController::UpdateSettings(const FFPVUserSettings& NewSettings)
{
	if (UFPVSettingsSubsystem* SettingsSubsystem = UFPVSettingsSubsystem::Get(this))
	{
		// The subsystem broadcasts OnSettingsChanged, which lands in HandleSettingsChanged.
		SettingsSubsystem->SetSettings(NewSettings);
	}
	else
	{
		FallbackSettings = NewSettings;
		FallbackSettings.Sanitize();
		HandleSettingsChanged(FallbackSettings);
	}
}

void AFPVPlayerController::HandleSettingsChanged(const FFPVUserSettings& NewSettings)
{
	ApplySettingsToDrone(NewSettings, false);
	RefreshInputBindings(NewSettings.Bindings);
}

void AFPVPlayerController::ApplySettingsToDrone(const FFPVUserSettings& Settings, bool bResetFlightMode)
{
	AFPVDronePawn* Drone = GetDrone();
	if (Drone == nullptr)
	{
		return;
	}
	Drone->ApplyTuning(Settings.Drone);
	Drone->ApplyCameraSettings(Settings.Camera);
	Drone->ApplyBatterySettings(Settings.Battery);
	if (bResetFlightMode)
	{
		Drone->SetFlightMode(Settings.DefaultFlightMode);
		Drone->SetCameraView(Settings.Camera.DefaultView);
	}
}

void AFPVPlayerController::RefreshInputBindings(const FFPVButtonBindings& Bindings)
{
	if (InputConfig == nullptr || FPVControllerConstants::BindingsEqual(Bindings, AppliedBindings))
	{
		return;
	}
	AppliedBindings = Bindings;
	InputConfig->RebuildMappings(Bindings);
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RequestRebuildControlMappings();
	}
}

// =============================================================================================
// Lifecycle
// =============================================================================================

AFPVDronePawn* AFPVPlayerController::GetDrone() const
{
	return Cast<AFPVDronePawn>(GetPawn());
}

void AFPVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UFPVSettingsSubsystem* SettingsSubsystem = UFPVSettingsSubsystem::Get(this))
	{
		SettingsChangedHandle = SettingsSubsystem->OnSettingsChanged.AddUObject(this, &AFPVPlayerController::HandleSettingsChanged);
	}

	if (UFPVGameplayEvents* Events = UFPVGameplayEvents::Get(this))
	{
		Events->OnTriggerPassed.AddDynamic(this, &AFPVPlayerController::HandleTriggerPassed);
		Events->OnDroneImpact.AddDynamic(this, &AFPVPlayerController::HandleDroneImpact);
	}

	Feedback = NewObject<UFPVControllerFeedback>(this, TEXT("FPVControllerFeedback"));
	Menu = NewObject<UFPVSettingsMenu>(this, TEXT("FPVSettingsMenu"));
	Menu->Initialize(this);

	// SetupInputComponent normally already did this; repeat in case the local player wasn't ready then.
	EnsureInputConfig();
	AddFlightMappingContext();
}

void AFPVPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFPVSettingsSubsystem* SettingsSubsystem = UFPVSettingsSubsystem::Get(this))
	{
		SettingsSubsystem->OnSettingsChanged.Remove(SettingsChangedHandle);
		if (SettingsSubsystem->HasUnsavedChanges())
		{
			// e.g. camera tilt changed with the D-pad and the menu was never opened.
			SettingsSubsystem->SaveToDisk();
		}
	}
	SettingsChangedHandle.Reset();

	if (UFPVGameplayEvents* Events = UFPVGameplayEvents::Get(this))
	{
		Events->OnTriggerPassed.RemoveDynamic(this, &AFPVPlayerController::HandleTriggerPassed);
		Events->OnDroneImpact.RemoveDynamic(this, &AFPVPlayerController::HandleDroneImpact);
	}
	Super::EndPlay(EndPlayReason);
}

void AFPVPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StickProcessor.Reset();
	ApplySettingsToDrone(GetSettings(), true);
}

// =============================================================================================
// Enhanced Input setup
// =============================================================================================

void AFPVPlayerController::EnsureInputConfig()
{
	if (InputConfig == nullptr)
	{
		AppliedBindings = GetSettings().Bindings;
		InputConfig = NewObject<UFPVInputConfig>(this, TEXT("FPVInputConfig"));
		InputConfig->Initialize(AppliedBindings);
	}
}

void AFPVPlayerController::AddFlightMappingContext()
{
	if (InputConfig == nullptr || !IsLocalPlayerController() || IsMenuOpen())
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputConfig->FlightContext, 0);
	}
}

void AFPVPlayerController::SetMenuInputActive(bool bMenuActive)
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem == nullptr || InputConfig == nullptr)
	{
		return;
	}
	if (bMenuActive)
	{
		Subsystem->RemoveMappingContext(InputConfig->FlightContext);
		Subsystem->AddMappingContext(InputConfig->MenuContext, 1);
	}
	else
	{
		Subsystem->RemoveMappingContext(InputConfig->MenuContext);
		Subsystem->AddMappingContext(InputConfig->FlightContext, 0);
	}
}

void AFPVPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputConfig();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInput == nullptr)
	{
		UE_LOG(LogFPVDrone, Error,
			TEXT("Input component is not a UEnhancedInputComponent. Check DefaultInputComponentClass in Config/DefaultInput.ini."));
		return;
	}

	// Sticks and held menu directions are polled every frame (value bindings).
	EnhancedInput->BindActionValue(InputConfig->LeftStickX);
	EnhancedInput->BindActionValue(InputConfig->LeftStickY);
	EnhancedInput->BindActionValue(InputConfig->RightStickX);
	EnhancedInput->BindActionValue(InputConfig->RightStickY);
	EnhancedInput->BindActionValue(InputConfig->MenuUp);
	EnhancedInput->BindActionValue(InputConfig->MenuDown);
	EnhancedInput->BindActionValue(InputConfig->MenuLeft);
	EnhancedInput->BindActionValue(InputConfig->MenuRight);
	EnhancedInput->BindActionValue(InputConfig->MenuStickX);
	EnhancedInput->BindActionValue(InputConfig->MenuStickY);

	// Utility buttons fire once per press.
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleFlightMode), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleFlightMode);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ResetDrone), ETriggerEvent::Started, this, &AFPVPlayerController::OnResetDrone);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleCamera), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleCamera);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::CameraTiltUp), ETriggerEvent::Started, this, &AFPVPlayerController::OnCameraTiltUp);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::CameraTiltDown), ETriggerEvent::Started, this, &AFPVPlayerController::OnCameraTiltDown);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleInputDebug), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleInputDebug);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::OpenMenu), ETriggerEvent::Started, this, &AFPVPlayerController::OnOpenMenu);

	// Menu buttons (these actions trigger while paused).
	EnhancedInput->BindAction(InputConfig->MenuConfirm, ETriggerEvent::Started, this, &AFPVPlayerController::OnMenuConfirm);
	EnhancedInput->BindAction(InputConfig->MenuBack, ETriggerEvent::Started, this, &AFPVPlayerController::OnMenuBack);
	EnhancedInput->BindAction(InputConfig->MenuClose, ETriggerEvent::Started, this, &AFPVPlayerController::OnMenuClose);

	AddFlightMappingContext();
}

// =============================================================================================
// Per-frame input
// =============================================================================================

FFPVStickValues AFPVPlayerController::ReadRawSticks() const
{
	FFPVStickValues Values;
	const UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInput == nullptr || InputConfig == nullptr)
	{
		return Values;
	}
	Values.LeftX = EnhancedInput->GetBoundActionValue(InputConfig->LeftStickX).Get<float>();
	Values.LeftY = EnhancedInput->GetBoundActionValue(InputConfig->LeftStickY).Get<float>();
	Values.RightX = EnhancedInput->GetBoundActionValue(InputConfig->RightStickX).Get<float>();
	Values.RightY = EnhancedInput->GetBoundActionValue(InputConfig->RightStickY).Get<float>();
	return Values;
}

void AFPVPlayerController::PlayerTick(float DeltaTime)
{
	// Super processes this frame's input (updates the Enhanced Input action values).
	Super::PlayerTick(DeltaTime);
	TrackLastPressedKey();

	if (IsMenuOpen())
	{
		// The world is paused: use real time for menu key repeat.
		UpdateMenuInput(static_cast<float>(FApp::GetDeltaTime()));
		return;
	}
	UpdateFlightInput(DeltaTime);
}

void AFPVPlayerController::TrackLastPressedKey()
{
	for (const FKey& Key : FPVGamepadKeys::GetButtons())
	{
		if (WasInputKeyJustPressed(Key))
		{
			LastPressedKey = Key;
		}
	}
}

void AFPVPlayerController::UpdateFlightInput(float DeltaTime)
{
	RawSticks = ReadRawSticks();

	AFPVDronePawn* Drone = GetDrone();
	if (Drone == nullptr || IsPaused())
	{
		return;
	}

	const FFPVUserSettings& Settings = GetSettings();
	const FFPVPilotCommand Command = StickProcessor.Process(RawSticks, Settings.Input, Settings.Drone, Drone->GetFlightMode(), DeltaTime);
	Drone->SetPilotCommand(Command);
}

void AFPVPlayerController::UpdateMenuInput(float RealDeltaSeconds)
{
	const UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (Menu == nullptr || EnhancedInput == nullptr || InputConfig == nullptr)
	{
		return;
	}

	FFPVMenuInput MenuInput;
	MenuInput.bUp = EnhancedInput->GetBoundActionValue(InputConfig->MenuUp).Get<bool>();
	MenuInput.bDown = EnhancedInput->GetBoundActionValue(InputConfig->MenuDown).Get<bool>();
	MenuInput.bLeft = EnhancedInput->GetBoundActionValue(InputConfig->MenuLeft).Get<bool>();
	MenuInput.bRight = EnhancedInput->GetBoundActionValue(InputConfig->MenuRight).Get<bool>();
	MenuInput.StickX = EnhancedInput->GetBoundActionValue(InputConfig->MenuStickX).Get<float>();
	MenuInput.StickY = EnhancedInput->GetBoundActionValue(InputConfig->MenuStickY).Get<float>();
	Menu->Tick(RealDeltaSeconds, MenuInput);
}

// =============================================================================================
// Menu
// =============================================================================================

bool AFPVPlayerController::IsMenuOpen() const
{
	return Menu != nullptr && Menu->IsOpen();
}

bool AFPVPlayerController::AreFlightButtonsBlocked() const
{
	if (IsMenuOpen())
	{
		return true;
	}
	return MenuClosedTime >= 0.0 && FPlatformTime::Seconds() - MenuClosedTime < FPVControllerConstants::MenuTransitionBlockSeconds;
}

bool AFPVPlayerController::AreMenuButtonsBlocked() const
{
	return !IsMenuOpen()
		|| (MenuOpenedTime >= 0.0 && FPlatformTime::Seconds() - MenuOpenedTime < FPVControllerConstants::MenuTransitionBlockSeconds);
}

void AFPVPlayerController::OpenMenu()
{
	if (Menu == nullptr || Menu->IsOpen())
	{
		return;
	}
	Menu->Open();
	MenuOpenedTime = FPlatformTime::Seconds();
	SetMenuInputActive(true);
	SetPause(true);
}

void AFPVPlayerController::CloseMenu()
{
	if (Menu == nullptr || !Menu->IsOpen())
	{
		return;
	}
	Menu->Close();
	MenuClosedTime = FPlatformTime::Seconds();

	if (UFPVSettingsSubsystem* SettingsSubsystem = UFPVSettingsSubsystem::Get(this))
	{
		if (SettingsSubsystem->HasUnsavedChanges())
		{
			SettingsSubsystem->SaveToDisk();
		}
	}

	SetMenuInputActive(false);
	SetPause(false);
}

void AFPVPlayerController::OnOpenMenu()
{
	if (!AreFlightButtonsBlocked())
	{
		OpenMenu();
	}
}

void AFPVPlayerController::OnMenuConfirm()
{
	if (!AreMenuButtonsBlocked())
	{
		Menu->Confirm();
	}
}

void AFPVPlayerController::OnMenuBack()
{
	if (!AreMenuButtonsBlocked() && Menu->Back())
	{
		CloseMenu();
	}
}

void AFPVPlayerController::OnMenuClose()
{
	if (!AreMenuButtonsBlocked() && !Menu->IsCapturingBinding())
	{
		CloseMenu();
	}
}

// =============================================================================================
// Flight buttons
// =============================================================================================

void AFPVPlayerController::OnToggleFlightMode()
{
	if (AreFlightButtonsBlocked())
	{
		return;
	}
	if (AFPVDronePawn* Drone = GetDrone())
	{
		Drone->ToggleFlightMode();
	}
}

void AFPVPlayerController::OnResetDrone()
{
	if (!AreFlightButtonsBlocked())
	{
		RequestResetDrone();
	}
}

void AFPVPlayerController::RequestResetDrone()
{
	StickProcessor.Reset();
	if (AFPVGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFPVGameMode>() : nullptr)
	{
		GameMode->RespawnDrone(this);
	}
	else if (AFPVDronePawn* Drone = GetDrone())
	{
		// Not running our game mode: just level the drone where it is, a little higher up.
		const FVector Location = Drone->GetActorLocation() + FVector(0.0, 0.0, 100.0);
		Drone->ResetDrone(FTransform(FRotator(0.0, Drone->GetActorRotation().Yaw, 0.0), Location));
	}
}

void AFPVPlayerController::OnToggleCamera()
{
	if (AreFlightButtonsBlocked())
	{
		return;
	}
	if (AFPVDronePawn* Drone = GetDrone())
	{
		Drone->ToggleCameraView();
	}
}

void AFPVPlayerController::OnCameraTiltUp()
{
	if (!AreFlightButtonsBlocked())
	{
		AdjustCameraTilt(GetSettings().Camera.TiltStepDeg);
	}
}

void AFPVPlayerController::OnCameraTiltDown()
{
	if (!AreFlightButtonsBlocked())
	{
		AdjustCameraTilt(-GetSettings().Camera.TiltStepDeg);
	}
}

void AFPVPlayerController::AdjustCameraTilt(float DeltaDegrees)
{
	// The tilt is part of the settings, so the menu shows it and it is saved with everything else.
	FFPVUserSettings NewSettings = GetSettings();
	NewSettings.Camera.FpvUptiltDeg = FMath::Clamp(NewSettings.Camera.FpvUptiltDeg + DeltaDegrees, -10.0f, 80.0f);
	UpdateSettings(NewSettings);
	ShowFlashMessage(FString::Printf(TEXT("CAMERA %.0f deg"), NewSettings.Camera.FpvUptiltDeg));
}

void AFPVPlayerController::OnToggleInputDebug()
{
	if (!AreFlightButtonsBlocked())
	{
		bShowInputDebug = !bShowInputDebug;
	}
}

// =============================================================================================
// On-screen messages and gameplay events
// =============================================================================================

void AFPVPlayerController::ShowFlashMessage(const FString& Message)
{
	FlashMessage = Message;
	FlashMessageTime = FPlatformTime::Seconds();
}

bool AFPVPlayerController::GetFlashMessage(FString& OutMessage, float& OutAgeSeconds) const
{
	if (FlashMessageTime < 0.0 || FlashMessage.IsEmpty())
	{
		return false;
	}
	OutAgeSeconds = static_cast<float>(FPlatformTime::Seconds() - FlashMessageTime);
	OutMessage = FlashMessage;
	return true;
}

void AFPVPlayerController::HandleTriggerPassed(AFPVDronePawn* Drone, UFPVPassThroughTriggerComponent* Trigger, bool bForward)
{
	// Free-fly has no course; just acknowledge the gate. A race mode would check order here.
	if (Drone != GetDrone() || Trigger == nullptr)
	{
		return;
	}
	ShowFlashMessage(Trigger->TriggerIndex >= 0
		? FString::Printf(TEXT("GATE %d%s"), Trigger->TriggerIndex + 1, bForward ? TEXT("") : TEXT(" (reverse)"))
		: FString(TEXT("THROUGH!")));
}

void AFPVPlayerController::HandleDroneImpact(AFPVDronePawn* Drone, const FFPVImpactInfo& Impact)
{
	if (Drone != GetDrone())
	{
		return;
	}
	const FFPVFeedbackSettings& FeedbackSettings = GetSettings().Feedback;
	if (Feedback != nullptr)
	{
		Feedback->PlayImpact(this, Impact.ImpactSpeedMps, FeedbackSettings);
	}
	if (Impact.ImpactSpeedMps >= FeedbackSettings.ImpactThresholdMps * FPVControllerConstants::ImpactMessageFactor)
	{
		ShowFlashMessage(FString::Printf(TEXT("IMPACT %.0f m/s"), Impact.ImpactSpeedMps));
	}
}
