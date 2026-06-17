// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveJson.h"
#include "WorldSerializerSubsystem.generated.h"

class USaveSubsystem;

/**
 * UWorldSerializerSubsystem — the Wave 2 manager that owns a world+player state snapshot
 * (the FWorldSaveGame pure model's payload) and persists it by REGISTERING a save/load hook
 * pair with the merged USaveSubsystem under a single "world" section. On Save the merged
 * subsystem calls our save hook, which writes the live world state into the section; on Load
 * it calls our load hook, which restores the live state from the section. We never reimplement
 * save IO — we drive it entirely through USaveSubsystem::RegisterHook (the merged interface).
 *
 * Why a GameInstanceSubsystem: the world snapshot is per-running-game state that must survive
 * level/world changes, matching USaveSubsystem's own lifetime so registration is stable.
 *
 * Ownership/lifecycle (NON-OWNING hook contract, inherited from USaveSubsystem):
 *  - We own the live WorldState object.
 *  - We register our hooks in Initialize and UNREGISTER them in Deinitialize, so no hook
 *    fires into a dangling `this` after we die. USaveSubsystem captures the delegates
 *    non-owningly; honoring the unregister-before-death contract is our responsibility.
 *
 * --- ROUND-TRIP TOLERANCE CONTRACT (inherited) ---
 * Round-trips are value-within-tolerance / structural, NOT byte-identity (compare floats
 * with GtcTest::Eps).
 */
UCLASS()
class GTC_UE5_API UWorldSerializerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** The section key our world snapshot lives under in the shared save. */
    static const FString WorldSection;

    // UGameInstanceSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** True once our hook is registered with USaveSubsystem. */
    bool IsRegistered() const { return bRegistered; }

    /** The live world+player state snapshot we serialize. Caller-owned key/value population. */
    TSharedRef<FGtcJsonObject> GetWorldState() const { return WorldState; }

    /** Replace the live world state wholesale (e.g. after an external load). */
    void SetWorldState(const TSharedRef<FGtcJsonObject>& NewState) { WorldState = NewState; }

    /**
     * Register our world save/load hook with the given USaveSubsystem. Split out from
     * Initialize so headless tests (which can't drive the subsystem collection) can wire
     * a directly-constructed USaveSubsystem. Idempotent; returns false if Save is null.
     */
    bool RegisterWith(USaveSubsystem* Save);

    /** Unregister our hook from the USaveSubsystem we registered with, if any. */
    void UnregisterFromSave();

private:
    /** Writes our live world state into the section (the save hook body). */
    void OnSaveWorld(const TSharedRef<FGtcJsonObject>& SectionOut);

    /** Restores our live world state from the section (the load hook body). */
    void OnLoadWorld(const TSharedRef<FGtcJsonObject>& SectionIn);

    /** Live world+player state. Owned by this subsystem. */
    TSharedRef<FGtcJsonObject> WorldState = MakeShared<FGtcJsonObject>();

    /** The USaveSubsystem we registered our hook with (non-owning), for clean unregister. */
    UPROPERTY()
    TObjectPtr<USaveSubsystem> RegisteredSave = nullptr;

    bool bRegistered = false;
};
