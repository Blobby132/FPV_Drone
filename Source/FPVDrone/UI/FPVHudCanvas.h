// Thin, resolution-independent wrapper over AHUD's canvas drawing functions.
// Layout code is written for a 1920x1080 reference screen; S() scales to the real resolution.

#pragma once

#include "CoreMinimal.h"

class AHUD;
class UFont;

enum class EFPVTextAlign : uint8
{
	Left,
	Center,
	Right
};

class FFPVHudCanvas
{
public:
	FFPVHudCanvas(AHUD& InHud, float InScreenWidth, float InScreenHeight);

	float GetWidth() const { return Width; }
	float GetHeight() const { return Height; }

	/** Reference (1080p) pixels -> screen pixels. */
	float S(float ReferencePixels) const { return ReferencePixels * Scale; }

	static UFont* SmallFont();
	static UFont* MediumFont();
	static UFont* LargeFont();

	/** Text size in screen pixels. TextScale is relative to the reference resolution. */
	FVector2D MeasureText(const FString& Text, UFont* Font, float TextScale = 1.0f) const;

	/** Draws text (with a 1-pixel drop shadow for readability over the 3D view). */
	void Text(const FString& Text, float X, float Y, const FLinearColor& Color, UFont* Font,
		float TextScale = 1.0f, EFPVTextAlign Align = EFPVTextAlign::Left, bool bShadow = true) const;

	void Rect(float X, float Y, float W, float H, const FLinearColor& Color) const;
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness = 1.0f) const;
	void Line(float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Thickness = 1.0f) const;

	/**
	 * Horizontal bar showing Value in [Min, Max]. The fill starts at Origin (e.g. 0 for a -1..1
	 * axis, so it grows left or right from the middle).
	 */
	void Bar(float X, float Y, float W, float H, float Value, float Min, float Max, float Origin,
		const FLinearColor& Fill, const FLinearColor& Background) const;

private:
	AHUD& Hud;
	float Width = 1920.0f;
	float Height = 1080.0f;
	float Scale = 1.0f;
};
