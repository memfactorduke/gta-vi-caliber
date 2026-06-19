// Copyright (c) 2026 GTC contributors

#include "GTCPlayerState.h"

#include "../Stats/PlayerStats.h"

AGTCPlayerState::AGTCPlayerState()
{
    // Canonical owner of the player's armor + money (W3 armor-ownership
    // resolution). Owning the stats component here keeps it alive across pawn
    // respawns and gives armor/money a single COND_OwnerOnly replicated owner.
    StatsComponent = CreateDefaultSubobject<UPlayerStatsComponent>(TEXT("StatsComponent"));
}
