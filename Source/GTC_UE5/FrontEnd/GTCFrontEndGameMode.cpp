#include "GTCFrontEndGameMode.h"
#include "GTCFrontEndController.h"
#include "GameFramework/SpectatorPawn.h"

AGTCFrontEndGameMode::AGTCFrontEndGameMode()
{
	PlayerControllerClass = AGTCFrontEndController::StaticClass();

	// No gameplay pawn on the menu — a tiny spectator keeps the view valid while
	// the full-screen UI covers it.
	DefaultPawnClass = ASpectatorPawn::StaticClass();
}
