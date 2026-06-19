// Dev-only: `gtc.PreviewLoading` overlays the loading screen on the current
// viewport so it can be inspected/screenshotted without a real world travel
// (the live one renders on the movie-player thread). Not used at runtime.

#include "SGTCLoadingScreen.h"
#include "../Core/GTCGameInstance.h"

#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"

static void GTCPreviewLoading(UWorld* World)
{
	if (!World)
	{
		return;
	}
	UGameViewportClient* VPC = World->GetGameViewport();
	UGTCGameInstance* GI = World->GetGameInstance<UGTCGameInstance>();
	if (!VPC || !GI)
	{
		return;
	}

	TArray<UTexture2D*> Art;
	for (const TObjectPtr<UTexture2D>& Tex : GI->GetLoadingArt())
	{
		Art.Add(Tex.Get());
	}

	TSharedRef<SGTCLoadingScreen> Screen = SNew(SGTCLoadingScreen).ArtTextures(Art);
	Screen->SetTargetProgress(0.62f);
	VPC->AddViewportWidgetContent(Screen, 11000);
}

static FAutoConsoleCommandWithWorld GTCPreviewLoadingCmd(
	TEXT("gtc.PreviewLoading"),
	TEXT("Dev: overlay the loading screen on the viewport for inspection."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&GTCPreviewLoading));
