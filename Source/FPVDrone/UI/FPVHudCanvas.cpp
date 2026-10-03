#include "UI/FPVHudCanvas.h"

#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/HUD.h"

FFPVHudCanvas::FFPVHudCanvas(AHUD& InHud, float InScreenWidth, float InScreenHeight)
	: Hud(InHud)
	, Width(FMath::Max(InScreenWidth, 1.0f))
	, Height(FMath::Max(InScreenHeight, 1.0f))
	, Scale(FMath::Max(InScreenHeight, 1.0f) / 1080.0f)
{
}

UFont* FFPVHudCanvas::SmallFont()
{
	return GEngine ? GEngine->GetSmallFont() : nullptr;
}

UFont* FFPVHudCanvas::MediumFont()
{
	return GEngine ? GEngine->GetMediumFont() : nullptr;
}

UFont* FFPVHudCanvas::LargeFont()
{
	return GEngine ? GEngine->GetLargeFont() : nullptr;
}

FVector2D FFPVHudCanvas::MeasureText(const FString& InText, UFont* Font, float TextScale) const
{
	float OutWidth = 0.0f;
	float OutHeight = 0.0f;
	Hud.GetTextSize(InText, OutWidth, OutHeight, Font, TextScale * Scale);
	return FVector2D(OutWidth, OutHeight);
}

void FFPVHudCanvas::Text(const FString& InText, float X, float Y, const FLinearColor& Color, UFont* Font,
	float TextScale, EFPVTextAlign Align, bool bShadow) const
{
	float DrawX = X;
	if (Align != EFPVTextAlign::Left)
	{
		const FVector2D Size = MeasureText(InText, Font, TextScale);
		DrawX -= (Align == EFPVTextAlign::Center) ? static_cast<float>(Size.X) * 0.5f : static_cast<float>(Size.X);
	}

	const float FinalScale = TextScale * Scale;
	if (bShadow)
	{
		const float Offset = FMath::Max(1.0f, S(1.5f));
		Hud.DrawText(InText, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.85f), DrawX + Offset, Y + Offset, Font, FinalScale);
	}
	Hud.DrawText(InText, Color, DrawX, Y, Font, FinalScale);
}

void FFPVHudCanvas::Rect(float X, float Y, float W, float H, const FLinearColor& Color) const
{
	Hud.DrawRect(Color, X, Y, W, H);
}

void FFPVHudCanvas::Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness) const
{
	Line(X, Y, X + W, Y, Color, Thickness);
	Line(X + W, Y, X + W, Y + H, Color, Thickness);
	Line(X + W, Y + H, X, Y + H, Color, Thickness);
	Line(X, Y + H, X, Y, Color, Thickness);
}

void FFPVHudCanvas::Line(float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Thickness) const
{
	Hud.DrawLine(X1, Y1, X2, Y2, Color, Thickness);
}

void FFPVHudCanvas::Bar(float X, float Y, float W, float H, float Value, float Min, float Max, float Origin,
	const FLinearColor& Fill, const FLinearColor& Background) const
{
	Rect(X, Y, W, H, Background);
	const float Range = Max - Min;
	if (Range <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float ValueX = X + W * FMath::Clamp((Value - Min) / Range, 0.0f, 1.0f);
	const float OriginX = X + W * FMath::Clamp((Origin - Min) / Range, 0.0f, 1.0f);
	const float Left = FMath::Min(ValueX, OriginX);
	const float Right = FMath::Max(ValueX, OriginX);
	Rect(Left, Y, FMath::Max(Right - Left, 1.0f), H, Fill);
}
