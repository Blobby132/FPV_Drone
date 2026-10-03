#include "UI/FPVMenuRenderer.h"

#include "UI/FPVHudCanvas.h"
#include "UI/FPVSettingsMenu.h"

namespace FPVMenuColors
{
	const FLinearColor ScreenDim(0.0f, 0.0f, 0.0f, 0.55f);
	const FLinearColor Panel(0.04f, 0.05f, 0.07f, 0.92f);
	const FLinearColor Border(1.0f, 0.55f, 0.1f, 0.9f);
	const FLinearColor Title(1.0f, 0.6f, 0.15f, 1.0f);
	const FLinearColor Label(0.9f, 0.9f, 0.9f, 1.0f);
	const FLinearColor Value(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor InfoText(0.6f, 0.75f, 0.9f, 1.0f);
	const FLinearColor SelectedRow(1.0f, 0.55f, 0.1f, 0.28f);
	const FLinearColor SliderBack(1.0f, 1.0f, 1.0f, 0.12f);
	const FLinearColor SliderFill(1.0f, 0.6f, 0.15f, 0.9f);
	const FLinearColor Description(0.75f, 0.75f, 0.75f, 1.0f);
	const FLinearColor Hints(0.55f, 0.55f, 0.55f, 1.0f);
	const FLinearColor Status(0.4f, 1.0f, 0.5f, 1.0f);
	const FLinearColor CaptureBox(0.0f, 0.0f, 0.0f, 0.9f);
}

void FPVMenuRenderer::Draw(const FFPVHudCanvas& Canvas, const FFPVMenuView& View)
{
	using namespace FPVMenuColors;

	const float ScreenW = Canvas.GetWidth();
	const float ScreenH = Canvas.GetHeight();
	Canvas.Rect(0.0f, 0.0f, ScreenW, ScreenH, ScreenDim);

	const float PanelW = Canvas.S(1100.0f);
	const float PanelH = Canvas.S(860.0f);
	const float PanelX = (ScreenW - PanelW) * 0.5f;
	const float PanelY = (ScreenH - PanelH) * 0.5f;
	const float Pad = Canvas.S(28.0f);
	const float RowH = Canvas.S(34.0f);

	Canvas.Rect(PanelX, PanelY, PanelW, PanelH, Panel);
	Canvas.Frame(PanelX, PanelY, PanelW, PanelH, Border, FMath::Max(1.0f, Canvas.S(2.0f)));

	// Title
	Canvas.Text(View.Title, PanelX + Pad, PanelY + Pad, Title, FFPVHudCanvas::LargeFont(), 1.0f);

	// Rows (scrolled so the selection stays visible)
	const float ListTop = PanelY + Pad + Canvas.S(64.0f);
	const float ListBottom = PanelY + PanelH - Canvas.S(170.0f);
	const int32 VisibleRows = FMath::Max(1, FMath::FloorToInt((ListBottom - ListTop) / RowH));
	const int32 RowCount = View.Rows.Num();
	int32 FirstRow = 0;
	if (RowCount > VisibleRows)
	{
		FirstRow = FMath::Clamp(View.SelectedRow - VisibleRows / 2, 0, RowCount - VisibleRows);
	}
	const int32 LastRow = FMath::Min(RowCount, FirstRow + VisibleRows);

	const float LabelX = PanelX + Pad;
	const float ValueRight = PanelX + PanelW - Pad;
	const float SliderW = Canvas.S(180.0f);
	const float SliderX = ValueRight - Canvas.S(330.0f);
	for (int32 Index = FirstRow; Index < LastRow; ++Index)
	{
		const FFPVMenuViewRow& Row = View.Rows[Index];
		const float Y = ListTop + RowH * static_cast<float>(Index - FirstRow);

		if (Row.bSelected)
		{
			Canvas.Rect(PanelX + Canvas.S(8.0f), Y - Canvas.S(3.0f), PanelW - Canvas.S(16.0f), RowH, SelectedRow);
		}

		if (!Row.bSelectable)
		{
			Canvas.Text(Row.Label, LabelX, Y, InfoText, FFPVHudCanvas::MediumFont(), 0.85f, EFPVTextAlign::Left, false);
			continue;
		}

		Canvas.Text(Row.Label, LabelX, Y, Label, FFPVHudCanvas::MediumFont(), 0.9f, EFPVTextAlign::Left, false);
		if (Row.SliderFraction >= 0.0f)
		{
			Canvas.Bar(SliderX, Y + RowH * 0.3f, SliderW, RowH * 0.25f, Row.SliderFraction, 0.0f, 1.0f, 0.0f, SliderFill, SliderBack);
		}
		Canvas.Text(Row.Value, ValueRight, Y, Value, FFPVHudCanvas::MediumFont(), 0.9f, EFPVTextAlign::Right, false);
	}

	// Scroll indicators
	if (FirstRow > 0)
	{
		Canvas.Text(TEXT("...more above"), ValueRight, ListTop - RowH, Hints, FFPVHudCanvas::SmallFont(), 1.0f, EFPVTextAlign::Right, false);
	}
	if (LastRow < RowCount)
	{
		Canvas.Text(TEXT("...more below"), ValueRight, ListTop + RowH * static_cast<float>(VisibleRows), Hints, FFPVHudCanvas::SmallFont(), 1.0f, EFPVTextAlign::Right, false);
	}

	// Description, status and button hints at the bottom of the panel.
	const float FooterY = PanelY + PanelH - Canvas.S(140.0f);
	Canvas.Line(PanelX + Pad, FooterY - Canvas.S(10.0f), PanelX + PanelW - Pad, FooterY - Canvas.S(10.0f), SliderBack);
	Canvas.Text(View.Description, LabelX, FooterY, Description, FFPVHudCanvas::MediumFont(), 0.8f, EFPVTextAlign::Left, false);
	if (!View.Status.IsEmpty())
	{
		Canvas.Text(View.Status, LabelX, FooterY + Canvas.S(36.0f), Status, FFPVHudCanvas::MediumFont(), 0.8f, EFPVTextAlign::Left, false);
	}
	Canvas.Text(View.Hints, LabelX, PanelY + PanelH - Canvas.S(44.0f), Hints, FFPVHudCanvas::MediumFont(), 0.75f, EFPVTextAlign::Left, false);

	// Rebind prompt
	if (View.bCapturing)
	{
		const float BoxW = Canvas.S(900.0f);
		const float BoxH = Canvas.S(110.0f);
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = (ScreenH - BoxH) * 0.5f;
		Canvas.Rect(BoxX, BoxY, BoxW, BoxH, CaptureBox);
		Canvas.Frame(BoxX, BoxY, BoxW, BoxH, Border, FMath::Max(1.0f, Canvas.S(2.0f)));
		Canvas.Text(View.CapturePrompt, ScreenW * 0.5f, BoxY + BoxH * 0.5f - Canvas.S(14.0f), Value, FFPVHudCanvas::MediumFont(), 0.95f, EFPVTextAlign::Center, false);
	}
}
