#include "UI/FPVInputDebugRenderer.h"

#include "Flight/FPVQuadMixer.h"
#include "UI/FPVHudCanvas.h"

namespace FPVDebugColors
{
	const FLinearColor Panel(0.0f, 0.0f, 0.0f, 0.65f);
	const FLinearColor Header(1.0f, 0.8f, 0.25f, 1.0f);
	const FLinearColor TextColor(0.92f, 0.92f, 0.92f, 1.0f);
	const FLinearColor Dim(0.65f, 0.65f, 0.65f, 1.0f);
	const FLinearColor Raw(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor Shaped(0.3f, 1.0f, 0.4f, 1.0f);
	const FLinearColor BarBack(1.0f, 1.0f, 1.0f, 0.12f);
	const FLinearColor ButtonUp(1.0f, 1.0f, 1.0f, 0.08f);
	const FLinearColor ButtonDown(0.2f, 0.85f, 0.3f, 0.85f);
	const FLinearColor Warning(1.0f, 0.4f, 0.3f, 1.0f);
}

namespace FPVDebugDraw
{
	/** Draws a square stick visualizer: raw position (white) and processed position (green). */
	void DrawStickBox(const FFPVHudCanvas& Canvas, float X, float Y, float Size, const FString& Label,
		float RawX, float RawY, float ShapedX, float ShapedY)
	{
		using namespace FPVDebugColors;
		Canvas.Rect(X, Y, Size, Size, BarBack);
		Canvas.Frame(X, Y, Size, Size, Dim);
		Canvas.Line(X + Size * 0.5f, Y, X + Size * 0.5f, Y + Size, BarBack);
		Canvas.Line(X, Y + Size * 0.5f, X + Size, Y + Size * 0.5f, BarBack);

		const float Dot = Canvas.S(8.0f);
		auto DrawDot = [&](float ValueX, float ValueY, const FLinearColor& Color)
		{
			// Screen Y grows downwards; stick Y is +1 when pushed up.
			const float PX = X + (FMath::Clamp(ValueX, -1.0f, 1.0f) * 0.5f + 0.5f) * Size;
			const float PY = Y + (-FMath::Clamp(ValueY, -1.0f, 1.0f) * 0.5f + 0.5f) * Size;
			Canvas.Rect(PX - Dot * 0.5f, PY - Dot * 0.5f, Dot, Dot, Color);
		};
		DrawDot(RawX, RawY, Raw);
		DrawDot(ShapedX, ShapedY, Shaped);

		Canvas.Text(Label, X + Size * 0.5f, Y + Size + Canvas.S(4.0f), Dim, FFPVHudCanvas::SmallFont(), 1.0f, EFPVTextAlign::Center, false);
	}
}

void FPVInputDebugRenderer::Draw(const FFPVHudCanvas& Canvas, const FFPVInputDebugData& Data)
{
	using namespace FPVDebugColors;

	UFont* Font = FFPVHudCanvas::SmallFont();
	const float X0 = Canvas.S(40.0f);
	const float Y0 = Canvas.S(170.0f);
	const float PanelW = Canvas.S(960.0f);
	const float PanelH = Canvas.S(600.0f);
	const float Pad = Canvas.S(14.0f);
	const float Row = Canvas.S(22.0f);
	const float ColW = (PanelW - Pad * 3.0f) * 0.5f;
	const float ColA = X0 + Pad;
	const float ColB = X0 + Pad * 2.0f + ColW;

	Canvas.Rect(X0, Y0, PanelW, PanelH, Panel);
	Canvas.Text(TEXT("INPUT DEBUG"), ColA, Y0 + Pad, Header, FFPVHudCanvas::MediumFont(), 0.9f, EFPVTextAlign::Left, false);
	Canvas.Text(TEXT("White = raw axis, green = after dead zone / curve / smoothing.  Stick Y should be +1 when pushed UP."),
		ColA + Canvas.S(170.0f), Y0 + Pad + Canvas.S(6.0f), Dim, Font, 1.0f, EFPVTextAlign::Left, false);

	// ============================ Column A: raw hardware view ==================================
	float Y = Y0 + Pad + Canvas.S(40.0f);

	// Stick visualizers.
	const float BoxSize = Canvas.S(130.0f);
	FPVDebugDraw::DrawStickBox(Canvas, ColA, Y, BoxSize, TEXT("Left stick"),
		Data.ActionSticks.LeftX, Data.ActionSticks.LeftY, Data.ShapedSticks.LeftX, Data.ShapedSticks.LeftY);
	DrawStickBox(Canvas, ColA + BoxSize + Pad * 2.0f, Y, BoxSize, TEXT("Right stick"),
		Data.ActionSticks.RightX, Data.ActionSticks.RightY, Data.ShapedSticks.RightX, Data.ShapedSticks.RightY);
	Y += BoxSize + Row * 1.6f;

	// Raw axes.
	Canvas.Text(TEXT("RAW AXES (engine key values)"), ColA, Y, Header, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	const float NameW = Canvas.S(80.0f);
	const float ValueW = Canvas.S(70.0f);
	for (const FFPVDebugAxis& Axis : Data.RawAxes)
	{
		const bool bTrigger = Axis.Name.Contains(TEXT("2"));
		Canvas.Text(Axis.Name, ColA, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Text(FString::Printf(TEXT("%+.3f"), Axis.Value), ColA + NameW, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Bar(ColA + NameW + ValueW, Y + Row * 0.25f, ColW - NameW - ValueW, Row * 0.5f,
			Axis.Value, bTrigger ? 0.0f : -1.0f, 1.0f, 0.0f, Raw, BarBack);
		Y += Row;
	}
	Y += Row * 0.5f;

	// Buttons.
	Canvas.Text(TEXT("BUTTONS"), ColA, Y, Header, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	const int32 Columns = 4;
	const float CellW = ColW / static_cast<float>(Columns);
	for (int32 Index = 0; Index < Data.Buttons.Num(); ++Index)
	{
		const FFPVDebugButton& Button = Data.Buttons[Index];
		const float CellX = ColA + CellW * static_cast<float>(Index % Columns);
		const float CellY = Y + Row * static_cast<float>(Index / Columns);
		Canvas.Rect(CellX + Canvas.S(1.0f), CellY + Canvas.S(1.0f), CellW - Canvas.S(2.0f), Row - Canvas.S(2.0f), Button.bDown ? ButtonDown : ButtonUp);
		Canvas.Text(Button.Name, CellX + Canvas.S(6.0f), CellY + Canvas.S(2.0f), TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
	}
	Y += Row * static_cast<float>((Data.Buttons.Num() + Columns - 1) / Columns) + Row * 0.3f;
	Canvas.Text(FString::Printf(TEXT("Last pressed: %s"), *Data.LastPressedButton), ColA, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);

	// ============================ Column B: processed / flight view ============================
	Y = Y0 + Pad + Canvas.S(40.0f);

	Canvas.Text(TEXT("ENHANCED INPUT  ->  PROCESSED"), ColB, Y, Header, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	struct FStickRow { const TCHAR* Name; float Action; float Shaped; };
	const FStickRow StickRows[] =
	{
		{ TEXT("Left X"), Data.ActionSticks.LeftX, Data.ShapedSticks.LeftX },
		{ TEXT("Left Y"), Data.ActionSticks.LeftY, Data.ShapedSticks.LeftY },
		{ TEXT("Right X"), Data.ActionSticks.RightX, Data.ShapedSticks.RightX },
		{ TEXT("Right Y"), Data.ActionSticks.RightY, Data.ShapedSticks.RightY },
	};
	for (const FStickRow& StickRow : StickRows)
	{
		Canvas.Text(StickRow.Name, ColB, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Text(FString::Printf(TEXT("%+.3f  ->  %+.3f"), StickRow.Action, StickRow.Shaped), ColB + NameW, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		const float BarX = ColB + NameW + Canvas.S(160.0f);
		Canvas.Bar(BarX, Y + Row * 0.25f, ColB + ColW - BarX, Row * 0.5f, StickRow.Shaped, -1.0f, 1.0f, 0.0f, Shaped, BarBack);
		Y += Row;
	}
	Y += Row * 0.5f;

	Canvas.Text(TEXT("PILOT COMMAND"), ColB, Y, Header, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	Canvas.Text(FString::Printf(TEXT("Throttle %5.1f%%   Roll %+.2f   Pitch %+.2f   Yaw %+.2f"),
		Data.Command.Throttle * 100.0f, Data.Command.Roll, Data.Command.Pitch, Data.Command.Yaw), ColB, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	Canvas.Text(FString::Printf(TEXT("%s   Throttle: %s   (hover point %.1f%%)"),
		*Data.StickModeText, *Data.ThrottleModeText, Data.HoverThrottle * 100.0f), ColB, Y, Dim, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row * 1.5f;

	Canvas.Text(TEXT("FLIGHT CONTROLLER (deg/s)          setpoint        gyro"), ColB, Y, Header, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	if (!Data.bHasDrone)
	{
		Canvas.Text(TEXT("No drone possessed."), ColB, Y, Warning, Font, 1.0f, EFPVTextAlign::Left, false);
		return;
	}
	const TCHAR* AxisNames[3] = { TEXT("Roll"), TEXT("Pitch"), TEXT("Yaw") };
	const FVector& Setpoint = Data.Telemetry.RateSetpointDegS;
	const FVector& Gyro = Data.Telemetry.GyroDegS;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Canvas.Text(AxisNames[Axis], ColB, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Text(FString::Printf(TEXT("%+8.1f"), Setpoint[Axis]), ColB + Canvas.S(250.0f), Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Text(FString::Printf(TEXT("%+8.1f"), Gyro[Axis]), ColB + Canvas.S(360.0f), Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Y += Row;
	}
	Y += Row * 0.3f;

	// Motor outputs.
	const float MotorW = ColW / static_cast<float>(FPVQuad::NumMotors);
	for (int32 Index = 0; Index < FPVQuad::NumMotors; ++Index)
	{
		const float MX = ColB + MotorW * static_cast<float>(Index);
		const float Output = Data.Telemetry.MotorOutputs[Index];
		Canvas.Text(FString::Printf(TEXT("%s %3.0f%%"), FPVQuadLayout::MotorName(Index), Output * 100.0f), MX, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
		Canvas.Bar(MX, Y + Row, MotorW - Canvas.S(10.0f), Row * 0.5f, Output, 0.0f, 1.0f, 0.0f, Shaped, BarBack);
	}
	Y += Row * 2.2f;

	Canvas.Text(FString::Printf(TEXT("Mixer throttle %.1f%%   Thrust %.1f N   %s"),
		Data.Telemetry.MixerThrottle * 100.0f, Data.Telemetry.TotalThrustN, Data.Telemetry.bMixerSaturated ? TEXT("SATURATED") : TEXT("")),
		ColB, Y, Data.Telemetry.bMixerSaturated ? Warning : TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
	Y += Row;
	Canvas.Text(FString::Printf(TEXT("Physics %.0f Hz (dt %.2f ms)   Render %.0f fps"),
		Data.PhysicsHz, Data.Telemetry.LastPhysicsDt * 1000.0f, Data.Fps), ColB, Y, TextColor, Font, 1.0f, EFPVTextAlign::Left, false);
}
