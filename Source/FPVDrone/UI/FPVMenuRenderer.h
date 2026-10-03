// Draws the pause / settings menu from a read-only FFPVMenuView.

#pragma once

#include "CoreMinimal.h"

class FFPVHudCanvas;
struct FFPVMenuView;

namespace FPVMenuRenderer
{
	void Draw(const FFPVHudCanvas& Canvas, const FFPVMenuView& View);
}
