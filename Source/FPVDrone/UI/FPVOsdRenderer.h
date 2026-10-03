// Betaflight-style on-screen display: flight mode, speed, altitude, throttle, battery, timer.

#pragma once

#include "CoreMinimal.h"
#include "Drone/FPVBatterySim.h"
#include "Settings/FPVSettingsTypes.h"

class FFPVHudCanvas;

/** Everything the OSD shows, gathered by AFPVHUD each frame. */
struct FFPVOsdData
{
	EFPVFlightMode FlightMode = EFPVFlightMode::Angle;
	EFPVThrottleMode ThrottleMode = EFPVThrottleMode::HoverCentered;
	EFPVCameraView CameraView = EFPVCameraView::FPV;
	float SpeedMps = 0.0f;
	float AltitudeM = 0.0f;
	/** RC throttle, 0..100%. */
	float ThrottlePercent = 0.0f;
	float UptiltDeg = 0.0f;
	FFPVBatteryState Battery;
	int32 CellCount = 6;
	float FlightTimeSeconds = 0.0f;
	bool bMixerSaturated = false;
	/** Button names for the hint line ("Options", "Create"). */
	FString MenuButtonName;
	FString DebugButtonName;
};

namespace FPVOsdRenderer
{
	void Draw(const FFPVHudCanvas& Canvas, const FFPVOsdData& Data);
}
