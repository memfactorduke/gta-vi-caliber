#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GTCFrontEndGameMode.generated.h"

/**
 * Game mode for the front-end map. Spawns no pawn — the player sits in the
 * AGTCFrontEndController, which builds the intro/menu UI and opens the world
 * map on "Start Game".
 */
UCLASS()
class GTC_UE5_API AGTCFrontEndGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGTCFrontEndGameMode();
};
