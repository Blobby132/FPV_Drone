#include "UI/FPVOsdRenderer.h"

#include "HAL/PlatformTime.h"
#include "Flight/FPVUnits.h"
#include "UI/FPVHudCanvas.h"

namespace FPVOsdColors
{
	const FLinearColor Primary(1.0f, 1.0f, 1.0f, 0.95f);
	const FLinearColor Dim(1.0f, 1.0f, 1.0f, 0.55f);
	const FLinearColor AngleMode(0.35f, 1.0f, 0.45f, 1.0f);
	const FLinearColor AcroMode(1.0f, 0.6f, 0.1f, 1.0f);
	const FLinearColor Good(0.35f, 1.0f, 0.45f, 1.0f);
	const FLinearColor Warn(1.0f, 0.85f, 0.2f, 1.0f);
	const FLinearColor Bad(1.0f, 0.25f, 0.2f, 1.0f);
	const FLinearColor BarBack(0.0f, 0.0f, 0.0f, 0.35f);
}

namespace FPVOsdHelpers
{
	FLinearColor GetBatteryStatusColor(float RemainingFraction)
	{
		if (RemainingFraction > 0.5f)
		{
			return FPVOsdColors::Good;
		}
		return RemainingFraction > 0.2f ? FPVOsdColors::Warn : FPVOsdColors::Bad;
	}

	FString FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	}

	bool BlinkOn()
	{
		return FMath::Fmod(FPlatformTime::Seconds(), 0.8) < 0.5;
	}
}

void FPVOsdRenderer::Draw(const FFPVHudCanvas& Canvas, const FFPVOsdData& Data)
{
	using namespace FPVOsdColors;

	const float W = Canvas.GetWidth();
	const float H = Canvas.GetHeight();
	const float Margin = Canvas.S(40.0f);
	const float RowH = Canvas.S(30.0f);

	// ---- Top left: flight mode, throttle mode, camera tilt --------------------------------------
	const bool bAngle = Data.FlightMode == EFPVFlightMode::Angle;
	Canvas.Text(bAngle ? TEXT("ANGLE") : TEXT("ACRO"), Margin, Margin, bAngle ? AngleMode : AcroMode, FFPVHudCanvas::LargeFont(), 1.2f);
	const TCHAR* ThrottleModeText = Data.ThrottleMode == EFPVThrottleMode::Latched ? TEXT("THR LATCHED") : TEXT("THR HOVER-CENTER");
	Canvas.Text(ThrottleModeText, Margin, Margin + RowH * 1.5f, Dim, FFPVHudCanvas::MediumFont(), 0.8f);
	Canvas.Text(FString::Printf(TEXT("CAM %+.0f deg"), Data.UptiltDeg), Margin, Margin + RowH * 2.3f, Dim, FFPVHudCanvas::MediumFont(), 0.8f);

	// ---- Top center: camera view / warnings --------------------------------------------------
	float CenterY = Margin;
	if (Data.CameraView == EFPVCameraView::Chase)
	{
		Canvas.Text(TEXT("CHASE CAM"), W * 0.5f, CenterY, Warn, FFPVHudCanvas::MediumFont(), 1.0f, EFPVTextAlign::Center);
		CenterY += RowH;
	}
	if (Data.Battery.RemainingFraction < 0.2f && FPVOsdHelpers::BlinkOn())
	{
		Canvas.Text(TEXT("LOW BATTERY"), W * 0.5f, CenterY, Bad, FFPVHudCanvas::MediumFont(), 1.0f, EFPVTextAlign::Center);
	}

	// ---- Top right: battery and timer ---------------------------------------------------------
	const FLinearColor BatteryTextColor = FPVOsdHelpers::GetBatteryStatusColor(Data.Battery.RemainingFraction);
	Canvas.Text(FString::Printf(TEXT("%.1fV"), Data.Battery.PackVoltage), W - Margin, Margin, BatteryTextColor,
		FFPVHudCanvas::LargeFont(), 1.2f, EFPVTextAlign::Right);
	Canvas.Text(FString::Printf(TEXT("%dS  %.2fV/cell  %.0f%%"), Data.CellCount, Data.Battery.CellVoltage, Data.Battery.RemainingFraction * 100.0f),
		W - Margin, Margin + RowH * 1.5f, Dim, FFPVHudCanvas::MediumFont(), 0.8f, EFPVTextAlign::Right);
	Canvas.Text(FString::Printf(TEXT("%.0fA  %.0fmAh"), Data.Battery.CurrentAmps, Data.Battery.ConsumedMah),
		W - Margin, Margin + RowH * 2.3f, Dim, FFPVHudCanvas::MediumFont(), 0.8f, EFPVTextAlign::Right);
	Canvas.Text(FPVOsdHelpers::FormatTime(Data.FlightTimeSeconds), W - Margin, Margin + RowH * 3.1f, Primary, FFPVHudCanvas::MediumFont(), 0.9f, EFPVTextAlign::Right);

	// ---- Center: crosshair (FPV only) ---------------------------------------------------------
	if (Data.CameraView == EFPVCameraView::FPV)
	{
		const float CX = W * 0.5f;
		const float CY = H * 0.5f;
		const float Arm = Canvas.S(14.0f);
		const float Gap = Canvas.S(5.0f);
		const float Thickness = FMath::Max(1.0f, Canvas.S(2.0f));
		Canvas.Line(CX - Gap - Arm, CY, CX - Gap, CY, Primary, Thickness);
		Canvas.Line(CX + Gap, CY, CX + Gap + Arm, CY, Primary, Thickness);
		Canvas.Line(CX, CY + Gap, CX, CY + Gap + Arm * 0.6f, Primary, Thickness);
	}

	// ---- Bottom left: speed --------------------------------------------------------------------
	const float BottomY = H - Margin - RowH * 2.0f;
	Canvas.Text(FString::Printf(TEXT("%.0f km/h"), Data.SpeedMps * FPVUnits::MpsToKmh), Margin, BottomY, Primary, FFPVHudCanvas::LargeFont(), 1.2f);
	Canvas.Text(FString::Printf(TEXT("%.1f m/s"), Data.SpeedMps), Margin, BottomY + RowH * 1.5f, Dim, FFPVHudCanvas::MediumFont(), 0.8f);

	// ---- Bottom right: altitude ---------------------------------------------------------------
	Canvas.Text(FString::Printf(TEXT("ALT %.1f m"), Data.AltitudeM), W - Margin, BottomY, Primary, FFPVHudCanvas::LargeFont(), 1.2f, EFPVTextAlign::Right);

	// ---- Bottom center: throttle bar ----------------------------------------------------------
	const float BarW = Canvas.S(240.0f);
	const float BarH = Canvas.S(10.0f);
	const float BarX = (W - BarW) * 0.5f;
	const float BarY = H - Margin - BarH;
	Canvas.Text(FString::Printf(TEXT("THR %.0f%%"), Data.ThrottlePercent), W * 0.5f, BarY - RowH * 1.2f,
		Data.bMixerSaturated ? Warn : Primary, FFPVHudCanvas::MediumFont(), 1.0f, EFPVTextAlign::Center);
	Canvas.Bar(BarX, BarY, BarW, BarH, Data.ThrottlePercent, 0.0f, 100.0f, 0.0f, Primary, BarBack);

	// ---- Hint ---------------------------------------------------------------------------------
	Canvas.Text(FString::Printf(TEXT("[%s] Menu   [%s] Input debug"), *Data.MenuButtonName, *Data.DebugButtonName),
		Margin, H - Canvas.S(22.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.35f), FFPVHudCanvas::SmallFont(), 1.0f, EFPVTextAlign::Left, false);
}
