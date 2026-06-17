// Copyright (c) 2026 GTC contributors

#include "CharacterRosterSubsystem.h"

void UCharacterRosterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Self-wiring: stand up the default dual-protagonist roster (the reference CharacterSwitcher._init
    // -> CharacterRoster.new(), which seeds DefaultCharacters() with the first lead active).
    Roster = FCharacterRoster();
}

void UCharacterRosterSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UCharacterRosterSubsystem::RequestSwitch(const FString& Id, double Now)
{
    // Wave-3 PlayerStats wallet write-back/load is DEFERRED (see header); here we drive only
    // the owned roster's neutral snapshot. On success, mirror the the reference character_switched signal.
    if (!Roster.SwitchTo(Id, Now))
    {
        return false;
    }
    OnCharacterSwitched.Broadcast(Id);
    return true;
}
