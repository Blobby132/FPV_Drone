#include "UI/FPVHUD.h"

#include "Engine/Canvas.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"

#include "Drone/FPVDronePawn.h"
#include "Flight/FPVFlightMath.h"
#include "Input/FPVGamepadKeys.h"
#include "Input/FPVPlayerController.h"
#include "UI/FPVHudCanvas.h"
#include "UI/FPVInputDebugRenderer.h"
#include "UI/FPVMenuRenderer.h"
#include "UI/FPVOsdRenderer.h"
#include "UI/FPVSettingsMenu.h"

AFPVHUD::AFPVHUD()
{
	// DrawHUD runs every frame anyway; no tick needed.
	PrimaryActorTick.bCanEverTick = false;
}

void AFPVHUD::DrawHUD()
{
	Super::DrawHUD();

	if (Canvas == nullptr)
	{
		return;
	}

	AFPVPlayerController* Controller = Cast<AFPVPlayerController>(GetOwningPlayerController());
	if (Controller == nullptr)
	{
		return;
	}
	AFPVDronePawn* Drone = Controller->GetDrone();

	UpdateRates(Drone);

	const FFPVHudCanvas HudCanvas(*this, Canvas->ClipX, Canvas->ClipY);

	if (Drone != nullptr)
	{
		FFPVOsdData OsdData;
		BuildOsdData(*Controller, *Drone, OsdData);
		FPVOsdRenderer::Draw(HudCanvas, OsdData);
	}

	FString FlashMessage;
	float FlashAge = 0.0f;
	constexpr float FlashDuration = 1.5f;
	if (Controller->GetFlashMessage(FlashMessage, FlashAge) && FlashAge < FlashDuration)
	{
		const float Alpha = FMath::Clamp((FlashDuration - FlashAge) / 0.4f, 0.0f, 1.0f);
		HudCanvas.Text(FlashMessage, HudCanvas.GetWidth() * 0.5f, HudCanvas.GetHeight() * 0.28f,
			FLinearColor(1.0f, 0.85f, 0.2f, Alpha), FFPVHudCanvas::LargeFont(), 1.4f, EFPVTextAlign::Center);
	}

	if (Controller->IsInputDebugVisible())
	{
		FFPVInputDebugData DebugData;
		BuildInputDebugData(*Controller, Drone, DebugData);
		FPVInputDebugRenderer::Draw(HudCanvas, DebugData);
	}

	// The menu draws last, on top of everything.
	if (const UFPVSettingsMenu* Menu = Controller->GetMenu())
	{
		if (Menu->IsOpen())
		{
			FFPVMenuView MenuView;
			Menu->BuildView(MenuView);
			FPVMenuRenderer::Draw(HudCanvas, MenuView);
		}
	}
}

void AFPVHUD::UpdateRates(const AFPVDronePawn* Drone)
{
	const float FrameTime = static_cast<float>(FApp::GetDeltaTime());
	if (FrameTime > 0.0f)
	{
		const float InstantFps = 1.0f / FrameTime;
		SmoothedFps = SmoothedFps <= 0.0f ? InstantFps : FMath::Lerp(SmoothedFps, InstantFps, 0.1f);
	}

	if (Drone == nullptr)
	{
		bHasRateSample = false;
		return;
	}

	// Physics rate = flight-controller steps per real second, sampled every half second.
	const uint64 Steps = Drone->GetTelemetry().PhysicsStepCount;
	const double Now = FPlatformTime::Seconds();
	if (!bHasRateSample || Steps < RateSampleSteps)
	{
		RateSampleSteps = Steps;
		RateSampleTime = Now;
		bHasRateSample = true;
		return;
	}
	const double Elapsed = Now - RateSampleTime;
	if (Elapsed >= 0.5)
	{
		PhysicsHz = static_cast<float>(static_cast<double>(Steps - RateSampleSteps) / Elapsed);
		RateSampleSteps = Steps;
		RateSampleTime = Now;
	}
}

void AFPVHUD::BuildOsdData(const AFPVPlayerController& Controller, const AFPVDronePawn& Drone, FFPVOsdData& OutData) const
{
	const FFPVUserSettings& Settings = Controller.GetSettings();
	const FFPVFlightTelemetry Telemetry = Drone.GetTelemetry();

	OutData.FlightMode = Drone.GetFlightMode();
	OutData.ThrottleMode = Settings.Drone.Throttle.Mode;
	OutData.CameraView = Drone.GetCameraView();
	OutData.SpeedMps = static_cast<float>(Drone.GetVelocityMps().Size());
	OutData.AltitudeM = Drone.GetAltitudeMeters();
	OutData.ThrottlePercent = Drone.GetLastCommand().Throttle * 100.0f;
	OutData.UptiltDeg = Settings.Camera.FpvUptiltDeg;
	OutData.Battery = Drone.GetBatteryState();
	OutData.CellCount = Settings.Battery.CellCount;
	OutData.FlightTimeSeconds = Drone.GetFlightTimeSeconds();
	OutData.bMixerSaturated = Telemetry.bMixerSaturated;
	OutData.MenuButtonName = FPVGamepadKeys::GetDisplayName(Settings.Bindings.OpenMenu);
	OutData.DebugButtonName = FPVGamepadKeys::GetDisplayName(Settings.Bindings.ToggleInputDebug);
}

void AFPVHUD::BuildInputDebugData(const AFPVPlayerController& Controller, const AFPVDronePawn* Drone, FFPVInputDebugData& OutData) const
{
	for (const FKey& Key : FPVGamepadKeys::GetAxes())
	{
		FFPVDebugAxis& Axis = OutData.RawAxes.AddDefaulted_GetRef();
		Axis.Name = FPVGamepadKeys::GetDisplayName(Key);
		Axis.Value = Controller.GetInputAnalogKeyState(Key);
	}
	for (const FKey& Key : FPVGamepadKeys::GetButtons())
	{
		FFPVDebugButton& Button = OutData.Buttons.AddDefaulted_GetRef();
		Button.Name = FPVGamepadKeys::GetDisplayName(Key);
		Button.bDown = Controller.IsInputKeyDown(Key);
	}
	OutData.LastPressedButton = FPVGamepadKeys::GetDisplayName(Controller.GetLastPressedKey());

	OutData.ActionSticks = Controller.GetRawSticks();
	OutData.ShapedSticks = Controller.GetStickProcessor().GetShapedSticks();

	const FFPVUserSettings& Settings = Controller.GetSettings();
	OutData.StickModeText = Settings.Input.StickMode == EFPVStickMode::Mode1 ? TEXT("Mode 1") : TEXT("Mode 2");
	OutData.ThrottleModeText = Settings.Drone.Throttle.Mode == EFPVThrottleMode::Latched
		? FString::Printf(TEXT("Latched (held %.0f%%)"), Controller.GetStickProcessor().GetLatchedThrottle() * 100.0f)
		: FString(TEXT("Hover-centered"));
	OutData.HoverThrottle = FPVFlightMath::GetEffectiveHoverThrottle(Settings.Drone);
	OutData.PhysicsHz = PhysicsHz;
	OutData.Fps = SmoothedFps;

	OutData.bHasDrone = Drone != nullptr;
	if (Drone != nullptr)
	{
		OutData.Command = Drone->GetLastCommand();
		OutData.Telemetry = Drone->GetTelemetry();
	}
}
