#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * The set of apps the in-game phone ships with. Order matters: it drives both the
 * generated icon array and the home-screen layout. Dock apps live at the front.
 */
enum class EGTCApp : uint8
{
	// Dock (bottom row).
	Phone,
	Messages,
	Camera,
	Music,
	// Home grid.
	Maps,
	Weather,
	Clock,
	Wallet,
	Photos,
	Settings,
	Calculator,
	Notes,
	Browser,
	Fitness,
	Stocks,
	Contacts,

	MAX
};

/**
 * All phone artwork, generated at runtime as transient textures so nothing is shipped
 * as a binary asset (no LFS, no copyrighted material — every icon is drawn from scratch
 * with original geometry that merely evokes the function, never copies a real icon).
 *
 * The returned textures are NOT rooted; the caller must keep them referenced (the phone
 * widget adopts them into TStrongObjectPtr) so they survive garbage collection.
 */
struct FGTCPhoneArt
{
	/** One icon per EGTCApp, indexed by (int32)EGTCApp. */
	TArray<UTexture2D*> AppIcons;

	/** Portrait home-screen wallpaper. */
	UTexture2D* Wallpaper = nullptr;

	/** Stylised top-down city map backdrop for the Maps app (water/beach/roads/blocks). */
	UTexture2D* MapBackdrop = nullptr;
};

namespace GTCPhoneArt
{
	/** Build every icon + the wallpaper. Call on the game thread. */
	GTC_UE5_API FGTCPhoneArt Generate();
}
