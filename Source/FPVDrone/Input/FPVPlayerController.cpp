#include "Input/FPVPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

#include "Core/FPVGameMode.h"
#include "Drone/FPVDronePawn.h"
#include "Gameplay/FPVGameplayEvents.h"
#include "Gameplay/FPVPassThroughTriggerComponent.h"
#include "HAL/PlatformTime.h"
#include "Input/FPVGamepadKeys.h"
#include "Input/FPVInputConfig.h"
#include "Settings/FPVSettingsSubsystem.h"
#include "FPVDrone.h"

AFPVPlayerController::AFPVPlayerController()
{
	// The pause menu is driven from PlayerTick, so keep ticking fully while the game is paused.
	bShouldPerformFullTickWhenPaused = true;
	bShowMouseCursor = false;
}

const FFPVUserSettings& AFPVPlayerController::GetSettings() const
{
	if (const UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(this))
	{
		return Settings->GetSettings();
	}
	return FallbackSettings;
}

void AFPVPlayerController::UpdateSettings(const FFPVUserSettings& NewSettings)
{
	if (UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(this))
	{
		// The subsystem broadcasts OnSettingsChanged, which lands in HandleSettingsChanged.
		Settings->SetSettings(NewSettings);
	}
	else
	{
		FallbackSettings = NewSettings;
		FallbackSettings.Sanitize();
		HandleSettingsChanged(FallbackSettings);
	}
}

AFPVDronePawn* AFPVPlayerController::GetDrone() const
{
	return Cast<AFPVDronePawn>(GetPawn());
}

void AFPVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(this))
	{
		SettingsChangedHandle = Settings->OnSettingsChanged.AddUObject(this, &AFPVPlayerController::HandleSettingsChanged);
	}

	if (UFPVGameplayEvents* Events = UFPVGameplayEvents::Get(this))
	{
		Events->OnTriggerPassed.AddDynamic(this, &AFPVPlayerController::HandleTriggerPassed);
	}

	// SetupInputComponent normally already did this; repeat in case the local player wasn't ready then.
	EnsureInputConfig();
	AddFlightMappingContext();
}

void AFPVPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFPVSettingsSubsystem* Settings = UFPVSettingsSubsystem::Get(this))
	{
		Settings->OnSettingsChanged.Remove(SettingsChangedHandle);
	}
	SettingsChangedHandle.Reset();

	if (UFPVGameplayEvents* Events = UFPVGameplayEvents::Get(this))
	{
		Events->OnTriggerPassed.RemoveDynamic(this, &AFPVPlayerController::HandleTriggerPassed);
	}
	Super::EndPlay(EndPlayReason);
}

void AFPVPlayerController::EnsureInputConfig()
{
	if (InputConfig == nullptr)
	{
		InputConfig = NewObject<UFPVInputConfig>(this, TEXT("FPVInputConfig"));
		InputConfig->Initialize(GetSettings().Bindings);
	}
}

void AFPVPlayerController::AddFlightMappingContext()
{
	if (InputConfig == nullptr || !IsLocalPlayerController())
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
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

	// Sticks are polled every frame (value bindings), not event driven.
	EnhancedInput->BindActionValue(InputConfig->LeftStickX);
	EnhancedInput->BindActionValue(InputConfig->LeftStickY);
	EnhancedInput->BindActionValue(InputConfig->RightStickX);
	EnhancedInput->BindActionValue(InputConfig->RightStickY);

	// Utility buttons fire once per press.
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleFlightMode), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleFlightMode);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ResetDrone), ETriggerEvent::Started, this, &AFPVPlayerController::OnResetDrone);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleCamera), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleCamera);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::CameraTiltUp), ETriggerEvent::Started, this, &AFPVPlayerController::OnCameraTiltUp);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::CameraTiltDown), ETriggerEvent::Started, this, &AFPVPlayerController::OnCameraTiltDown);
	EnhancedInput->BindAction(InputConfig->GetButtonAction(EFPVButtonAction::ToggleInputDebug), ETriggerEvent::Started, this, &AFPVPlayerController::OnToggleInputDebug);

	AddFlightMappingContext();
}

void AFPVPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StickProcessor.Reset();
	ApplySettingsToDrone(GetSettings(), true);
}

void AFPVPlayerController::HandleSettingsChanged(const FFPVUserSettings& NewSettings)
{
	ApplySettingsToDrone(NewSettings, false);
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

void AFPVPlayerController::OnToggleFlightMode()
{
	if (AFPVDronePawn* Drone = GetDrone())
	{
		Drone->ToggleFlightMode();
	}
}

void AFPVPlayerController::OnResetDrone()
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
	if (AFPVDronePawn* Drone = GetDrone())
	{
		Drone->ToggleCameraView();
	}
}

void AFPVPlayerController::OnCameraTiltUp()
{
	AdjustCameraTilt(GetSettings().Camera.TiltStepDeg);
}

void AFPVPlayerController::OnCameraTiltDown()
{
	AdjustCameraTilt(-GetSettings().Camera.TiltStepDeg);
}

void AFPVPlayerController::AdjustCameraTilt(float DeltaDegrees)
{
	// The tilt is part of the settings, so the menu shows it and it is saved with everything else.
	FFPVUserSettings NewSettings = GetSettings();
	NewSettings.Camera.FpvUptiltDeg = FMath::Clamp(NewSettings.Camera.FpvUptiltDeg + DeltaDegrees, -10.0f, 80.0f);
	UpdateSettings(NewSettings);
}

void AFPVPlayerController::OnToggleInputDebug()
{
	bShowInputDebug = !bShowInputDebug;
}

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
