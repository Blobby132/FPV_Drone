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
	if (bResetFlightMode)
	{
		Drone->SetFlightMode(Settings.DefaultFlightMode);
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
	UpdateFlightInput(DeltaTime);
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
