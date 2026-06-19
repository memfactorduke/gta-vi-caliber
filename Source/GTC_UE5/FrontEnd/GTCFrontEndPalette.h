#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

/**
 * Shared visual language for the game shell — the boot intro, the main menu, the
 * loading cover, and the in-game Esc pause menu. Keeping every surface on one
 * palette is what makes the intro -> menu -> loading -> pause flow read as a
 * single production rather than four unrelated screens.
 *
 * The colours are pulled from the project's own CALIBER boot videos: a hot-magenta
 * and cyan neon wordmark over a Miami sunset, sitting on a near-black night sky.
 * Header-only so the FrontEnd widgets and UI/Menus widgets can all share it
 * without a heavier style dependency.
 */
namespace GTCFrontEndPalette
{
	// --- Neon accents (match the CALIBER wordmark: magenta primary, cyan secondary) ---
	static const FLinearColor Magenta(1.0f, 0.13f, 0.55f, 1.0f);   // primary focus / accent
	static const FLinearColor Cyan(0.16f, 0.93f, 1.0f, 1.0f);      // secondary / data accent
	static const FLinearColor Sunset(1.0f, 0.46f, 0.16f, 1.0f);    // warm tie-in to the sky

	// --- Surfaces ---
	static const FLinearColor Night(0.012f, 0.016f, 0.035f, 1.0f); // deep night base
	static const FLinearColor Scrim(0.0f, 0.005f, 0.02f, 0.62f);   // legibility wash over video
	static const FLinearColor PanelFill(0.02f, 0.03f, 0.06f, 0.92f); // pause-panel body

	// --- Text ---
	static const FLinearColor TextBright(0.98f, 0.99f, 1.0f, 1.0f);
	static const FLinearColor TextDim(0.80f, 0.86f, 0.93f, 0.55f);
	static const FLinearColor Kicker(0.90f, 0.74f, 0.96f, 0.72f);  // section labels (faint magenta)
	static const FLinearColor Footer(0.78f, 0.86f, 0.94f, 0.50f);

	/** Slow neon pulse [0..1], shared so every accent rail breathes in sync. */
	inline float Pulse(float SpeedHz = 1.8f)
	{
		const float T = static_cast<float>(FPlatformTime::Seconds());
		return 0.5f + 0.5f * FMath::Sin(T * SpeedHz);
	}

	/** Magenta accent with a pulsing alpha, for the animated rails under kickers. */
	inline FLinearColor PulsingAccent(float Base = 0.62f, float Amp = 0.22f)
	{
		FLinearColor C = Magenta;
		C.A = Base + Amp * Pulse();
		return C;
	}
}
