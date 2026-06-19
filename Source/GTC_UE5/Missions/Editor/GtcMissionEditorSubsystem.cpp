// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionEditorSubsystem.h"

#include "GtcMissionBank.h"
#include "GtcMissionCompiler.h"
#include "GtcMissionJson.h"
#include "GtcMissionReward.h"
#include "GtcMissionObjectiveMarker.h"
#include "../Coordinators/MissionController.h"
#include "../Coordinators/MissionReward.h"
#include "../../Player/Stats/PlayerStats.h"
#include "../../Player/Health/PlayerHealthComponent.h"
#include "../../Systems/Progression/ProgressionTracker.h"
#include "../../Systems/Save/SaveSubsystem.h"
#include "../../Systems/Save/SaveJson.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogGtcMissionEditor, Log, All);

namespace
{
    // The GameInstance backing this world subsystem, or null.
    UGameInstance* GameInstanceOf(const UWorldSubsystem* Self)
    {
        const UWorld* W = Self ? Self->GetWorld() : nullptr;
        return W ? W->GetGameInstance() : nullptr;
    }
}

bool UGtcMissionEditorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    // Present in the running game, PIE, and the open editor world (god-mode placement).
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

void UGtcMissionEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Persist the authored bank inside the savegame — but only from a real game/PIE world, so the
    // editor-preview world's instance doesn't also register and fight over the shared section hook.
    UWorld* W = GetWorld();
    if (W && W->IsGameWorld())
    {
        if (UGameInstance* GI = W->GetGameInstance())
        {
            if (USaveSubsystem* Save = GI->GetSubsystem<USaveSubsystem>())
            {
                FSaveHook OnSave;
                OnSave.BindUObject(this, &UGtcMissionEditorSubsystem::OnSaveMissions);
                FLoadHook OnLoad;
                OnLoad.BindUObject(this, &UGtcMissionEditorSubsystem::OnLoadMissions);
                Save->RegisterHook(TEXT("mission_editor"), OnSave, OnLoad);
                bSaveHookRegistered = true;
            }
        }
    }
}

void UGtcMissionEditorSubsystem::Deinitialize()
{
    // The controller is a GameInstance subsystem that outlives this world subsystem; drop our
    // delegate bindings so its multicast can't call into a destroyed editor subsystem.
    bPlaying = false;
    bWorldTriggersEnabled = false;
    UnbindRewards();
    ClearObjectiveMarkers();
    ClearGiverMarkers();

    if (bSaveHookRegistered)
    {
        if (UGameInstance* GI = GameInstanceOf(this))
        {
            if (USaveSubsystem* Save = GI->GetSubsystem<USaveSubsystem>())
            {
                Save->UnregisterHook(TEXT("mission_editor"));
            }
        }
        bSaveHookRegistered = false;
    }

    Super::Deinitialize();
}

FGuid UGtcMissionEditorSubsystem::CreateMission(EGtcMissionContentType Type)
{
    return GtcMissionBank::CreateMission(Missions, Type);
}

bool UGtcMissionEditorSubsystem::AddObjective(FGuid MissionId, const FGtcObjectiveDefinition& Objective)
{
    return GtcMissionBank::AddObjective(Missions, MissionId, Objective);
}

bool UGtcMissionEditorSubsystem::RemoveObjective(FGuid MissionId, const FString& ObjectiveId)
{
    return GtcMissionBank::RemoveObjective(Missions, MissionId, ObjectiveId);
}

bool UGtcMissionEditorSubsystem::ReorderObjective(FGuid MissionId, int32 FromIndex, int32 ToIndex)
{
    return GtcMissionBank::ReorderObjective(Missions, MissionId, FromIndex, ToIndex);
}

bool UGtcMissionEditorSubsystem::RemoveMission(FGuid MissionId)
{
    return GtcMissionBank::RemoveMission(Missions, MissionId);
}

bool UGtcMissionEditorSubsystem::ValidateMission(FGuid MissionId, TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const
{
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, MissionId);
    if (!M)
    {
        OutErrors.Add(TEXT("Mission not found."));
        return false;
    }
    const FGtcMissionValidation V = GtcMissionBank::Validate(*M);
    OutErrors = V.Errors;
    OutWarnings = V.Warnings;
    return V.IsValid();
}

bool UGtcMissionEditorSubsystem::SaveMissionToFile(FGuid MissionId, const FString& AbsolutePath) const
{
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, MissionId);
    return M ? GtcMissionJson::SaveMissionToFile(*M, AbsolutePath) : false;
}

bool UGtcMissionEditorSubsystem::LoadMissionFromFile(const FString& AbsolutePath)
{
    FGtcMissionDefinition Loaded;
    if (!GtcMissionJson::LoadMissionFromFile(AbsolutePath, Loaded))
    {
        return false;
    }
    if (FGtcMissionDefinition* Existing = GtcMissionBank::Find(Missions, Loaded.MissionId))
    {
        *Existing = MoveTemp(Loaded);
    }
    else
    {
        Missions.Add(MoveTemp(Loaded));
    }
    return true;
}

bool UGtcMissionEditorSubsystem::PlayMission(FGuid MissionId)
{
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, MissionId);
    if (!M)
    {
        return false;
    }

    // Activities (Side Job / Street Race) run through their own pure models — wired later.
    if (M->ContentType == EGtcMissionContentType::Activity)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionEditor] Activity playback not yet wired; '%s' is an activity."), *M->Title);
        return false;
    }

    const FGtcMissionValidation Validation = GtcMissionBank::Validate(*M);
    if (!Validation.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionEditor] '%s' has %d validation error(s); not playing."),
            *M->Title, Validation.Errors.Num());
        return false;
    }

    const FGtcCompiledMission Compiled = GtcMissionCompiler::CompileMission(*M);
    if (!Compiled.bValid)
    {
        return false;
    }

    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMissionController* Controller = GI ? GI->GetSubsystem<UMissionController>() : nullptr;
    if (!Controller)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MissionEditor] No MissionController available to play '%s'."), *M->Title);
        return false;
    }

    // Copy the compiled mission into the controller's code-mutated config, then (re)build.
    Controller->Title = Compiled.Title;
    Controller->ObjectiveDefs.Reset();
    Controller->Waypoints.Reset();
    for (const FGtcCompiledObjective& O : Compiled.Objectives)
    {
        UMissionController::FObjectiveDef Def;
        Def.Id = O.Id;
        Def.Text = O.Text;
        Controller->ObjectiveDefs.Add(MoveTemp(Def));
        if (O.bHasWaypoint)
        {
            Controller->Waypoints.Add(O.Id, O.Waypoint);
        }
    }
    Controller->TimeLimit = Compiled.TimeLimit;

    // Reset rebuilds from the new defs; Begin (idempotent) guarantees it's active.
    Controller->Reset();
    Controller->Begin();

    // Wire reward payout for this run.
    ConfigureAndBindRewards(*M, Controller);

    // Arm per-frame auto-completion for reach/hold objectives that have a waypoint. Objectives
    // without one (or count/interact kinds) aren't added, so they aren't auto-satisfied — they
    // complete via gameplay events / GTC.Mission.Complete.
    ObjectiveDriver.Bind();
    ObjectiveDriver.Defs = GtcMissionCompiler::CompileObjectiveDrivers(*M);
    PlayingMissionId = MissionId;
    bPlaying = true;
    return true;
}

void UGtcMissionEditorSubsystem::Tick(float DeltaTime)
{
    // A mission that ended last frame defers its teardown to here, so we never remove a
    // delegate from inside its own broadcast.
    if (bTeardownPending)
    {
        UnbindRewards();
        ObjectiveDriver.Defs.Reset();
        ObjectiveDriver.Bind();
        ActiveCountObjId.Reset();
        ActiveCountProgress = 0;
        ActiveTimerObjId.Reset();
        ActiveTimerElapsed = 0.0;
        // A mission just ended — re-show world givers so the finished one drops out and any
        // newly-unlocked missions appear.
        if (bWorldTriggersEnabled)
        {
            SpawnGiverMarkers();
        }
    }
    if (!bPlaying)
    {
        // Between missions: start one when the player walks into its giver blip.
        if (bWorldTriggersEnabled)
        {
            CheckGiverProximity();
        }
        return;
    }

    UGameInstance* GameInst = GameInstanceOf(this);
    UMissionController* Controller = GameInst ? GameInst->GetSubsystem<UMissionController>() : nullptr;
    if (!Controller || !Controller->IsActive())
    {
        bPlaying = false;
        return;
    }

    // Advance the controller's fail-timer + death-fail logic with the real player death state.
    bool bPlayerDead = false;
    if (const UPlayerHealthComponent* Health = FindPlayerHealth())
    {
        bPlayerDead = Health->IsDead();
    }
    Controller->Tick(bPlayerDead, DeltaTime);
    if (!Controller->IsActive())
    {
        bPlaying = false;
        return;
    }

    // Auto-complete the active objective.
    const FString ActiveId = Controller->CurrentObjectiveId();
    if (ActiveId.IsEmpty())
    {
        return;
    }

    const FGtcMissionDefinition* PlayMissionDef = GtcMissionBank::Find(Missions, PlayingMissionId);
    const FGtcObjectiveDefinition* ActiveObj = PlayMissionDef
        ? PlayMissionDef->Objectives.FindByPredicate([&ActiveId](const FGtcObjectiveDefinition& O) { return O.Id == ActiveId; })
        : nullptr;

    // Survive: complete after holding out for Duration seconds (death already fails the mission).
    if (ActiveObj && ActiveObj->Kind == EGtcObjectiveKind::Survive)
    {
        if (ActiveId != ActiveTimerObjId)
        {
            ActiveTimerObjId = ActiveId;
            ActiveTimerElapsed = 0.0;
        }
        ActiveTimerElapsed += DeltaTime;
        if (ActiveTimerElapsed >= FMath::Max(0.1, ActiveObj->Duration))
        {
            ActiveTimerObjId.Reset();
            ActiveTimerElapsed = 0.0;
            Controller->Complete(ActiveId);
        }
        return;
    }

    // Reach/hold: complete from player proximity to the objective waypoint.
    FVector PlayerPos;
    if (!GetPlayerLocation(PlayerPos))
    {
        return;
    }
    const FVector Target = Controller->CurrentWaypoint(PlayerPos);
    if (ObjectiveDriver.TickObjective(ActiveId, PlayerPos, Target, DeltaTime))
    {
        Controller->Complete(ActiveId);
    }
}

TStatId UGtcMissionEditorSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGtcMissionEditorSubsystem, STATGROUP_Tickables);
}

bool UGtcMissionEditorSubsystem::GetPlayerLocation(FVector& OutLocation) const
{
    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    APawn* P = PC ? PC->GetPawn() : nullptr;
    if (!P)
    {
        return false;
    }
    OutLocation = P->GetActorLocation();
    return true;
}

UPlayerHealthComponent* UGtcMissionEditorSubsystem::FindPlayerHealth() const
{
    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        return nullptr;
    }
    if (PC->PlayerState)
    {
        if (UPlayerHealthComponent* C = PC->PlayerState->FindComponentByClass<UPlayerHealthComponent>())
        {
            return C;
        }
    }
    if (APawn* P = PC->GetPawn())
    {
        if (UPlayerHealthComponent* C = P->FindComponentByClass<UPlayerHealthComponent>())
        {
            return C;
        }
    }
    return nullptr;
}

void UGtcMissionEditorSubsystem::OnSaveMissions(const TSharedRef<FGtcJsonObject>& SectionOut)
{
    GtcMissionJson::WriteBank(Missions, SectionOut);
    GtcMissionJson::WriteGuidSet(SectionOut, TEXT("completed"), CompletedMissions);
    GtcMissionJson::WriteStringSet(SectionOut, TEXT("unlocks"), GrantedUnlocks);
}

void UGtcMissionEditorSubsystem::OnLoadMissions(const TSharedRef<FGtcJsonObject>& SectionIn)
{
    GtcMissionJson::ReadBank(SectionIn, Missions);
    GtcMissionJson::ReadGuidSet(SectionIn, TEXT("completed"), CompletedMissions);
    GtcMissionJson::ReadStringSet(SectionIn, TEXT("unlocks"), GrantedUnlocks);
}

void UGtcMissionEditorSubsystem::ConfigureAndBindRewards(const FGtcMissionDefinition& Mission, UMissionController* Controller)
{
    UnbindRewards();
    if (!Controller)
    {
        return;
    }

    const FGtcRewardPlan Plan = GtcMissionReward::Plan(Mission.Reward);
    ActiveHeadlineMoney = Plan.HeadlineMoney;

    UGameInstance* GameInst = GameInstanceOf(this);

    if (UMissionReward* Reward = GameInst ? GameInst->GetSubsystem<UMissionReward>() : nullptr)
    {
        Reward->ObjectiveReward = Plan.ObjectiveReward;
        Reward->MissionBonus = Plan.MissionBonus;
        if (const UPlayerStatsComponent* Stats = FindPlayerStats())
        {
            Reward->SetWallet(Stats->GetMoney());
        }
        // Bind the reward subsystem BEFORE our own handlers so payout is credited first.
        Reward->BindController(Controller);
    }

    if (UProgressionTracker* Progression = GameInst ? GameInst->GetSubsystem<UProgressionTracker>() : nullptr)
    {
        if (Plan.MissionXp > 0)
        {
            Progression->MissionXp = Plan.MissionXp;
        }
    }

    RewardObjectiveHandle = Controller->OnObjectiveCompleted.AddUObject(this, &UGtcMissionEditorSubsystem::HandleObjectiveCompleted);
    RewardMissionHandle = Controller->OnMissionCompleted.AddUObject(this, &UGtcMissionEditorSubsystem::HandleMissionCompleted);
    RewardFailHandle = Controller->OnMissionFailed.AddUObject(this, &UGtcMissionEditorSubsystem::HandleMissionFailed);
    RewardBoundController = Controller;
}

void UGtcMissionEditorSubsystem::UnbindRewards()
{
    if (UMissionController* Controller = RewardBoundController.Get())
    {
        Controller->OnObjectiveCompleted.Remove(RewardObjectiveHandle);
        Controller->OnMissionCompleted.Remove(RewardMissionHandle);
        Controller->OnMissionFailed.Remove(RewardFailHandle);
    }
    RewardObjectiveHandle.Reset();
    RewardMissionHandle.Reset();
    RewardFailHandle.Reset();
    RewardBoundController.Reset();

    if (UGameInstance* GameInst = GameInstanceOf(this))
    {
        if (UMissionReward* Reward = GameInst->GetSubsystem<UMissionReward>())
        {
            Reward->UnbindController();
        }
    }
    ActiveHeadlineMoney = 0;
    bTeardownPending = false;
}

void UGtcMissionEditorSubsystem::HandleObjectiveCompleted(const FString& /*ObjectiveId*/)
{
    DrainPayoutToWallet();
    if (UGameInstance* GameInst = GameInstanceOf(this))
    {
        if (UProgressionTracker* Progression = GameInst->GetSubsystem<UProgressionTracker>())
        {
            Progression->AwardObjective();
        }
    }
}

void UGtcMissionEditorSubsystem::HandleMissionCompleted()
{
    // Drain any remaining accrued payout (per-objective rewards + the mission bonus).
    DrainPayoutToWallet();

    // Plus the authored headline lump.
    if (ActiveHeadlineMoney > 0)
    {
        if (UPlayerStatsComponent* Stats = FindPlayerStats())
        {
            Stats->AddMoney(ActiveHeadlineMoney);
        }
    }

    if (UGameInstance* GameInst = GameInstanceOf(this))
    {
        if (UProgressionTracker* Progression = GameInst->GetSubsystem<UProgressionTracker>())
        {
            Progression->AwardMission();
        }
    }

    // Record completion so prerequisite-locked missions can unlock, and grant this mission's
    // reward unlocks (persisted + queryable via HasUnlock so gameplay can gate content on them).
    CompletedMissions.Add(PlayingMissionId);
    if (const FGtcMissionDefinition* CompletedDef = GtcMissionBank::Find(Missions, PlayingMissionId))
    {
        for (const FString& Unlock : CompletedDef->Reward.Unlocks)
        {
            GrantedUnlocks.Add(Unlock);
        }
    }

    // Defer the actual unbind to the next tick — we're inside OnMissionCompleted's broadcast.
    bPlaying = false;
    bTeardownPending = true;
}

void UGtcMissionEditorSubsystem::HandleMissionFailed()
{
    // No payout on failure; defer teardown out of the broadcast.
    bPlaying = false;
    bTeardownPending = true;
}

void UGtcMissionEditorSubsystem::DrainPayoutToWallet()
{
    UGameInstance* GameInst = GameInstanceOf(this);
    UMissionReward* Reward = GameInst ? GameInst->GetSubsystem<UMissionReward>() : nullptr;
    if (!Reward)
    {
        return;
    }
    const int32 Payout = Reward->ConsumePendingPayout();
    if (Payout > 0)
    {
        if (UPlayerStatsComponent* Stats = FindPlayerStats())
        {
            Stats->AddMoney(Payout);
        }
    }
}

UPlayerStatsComponent* UGtcMissionEditorSubsystem::FindPlayerStats() const
{
    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        return nullptr;
    }
    if (PC->PlayerState)
    {
        if (UPlayerStatsComponent* C = PC->PlayerState->FindComponentByClass<UPlayerStatsComponent>())
        {
            return C;
        }
    }
    if (APawn* P = PC->GetPawn())
    {
        if (UPlayerStatsComponent* C = P->FindComponentByClass<UPlayerStatsComponent>())
        {
            return C;
        }
    }
    return nullptr;
}

// --- God-mode authoring surface ---------------------------------------------------------

TArray<FGtcMissionSummary> UGtcMissionEditorSubsystem::GetMissionSummaries() const
{
    TArray<FGtcMissionSummary> Out;
    Out.Reserve(Missions.Num());
    for (const FGtcMissionDefinition& M : Missions)
    {
        FGtcMissionSummary S;
        S.MissionId = M.MissionId;
        S.Id = M.Id;
        S.Title = M.Title;
        S.ContentType = M.ContentType;
        S.ObjectiveCount = M.Objectives.Num();
        S.bValid = GtcMissionBank::Validate(M).IsValid();
        Out.Add(MoveTemp(S));
    }
    return Out;
}

bool UGtcMissionEditorSubsystem::SetObjectiveWaypoint(FGuid MissionId, const FString& ObjectiveId, FVector WorldLocation)
{
    const bool bOk = GtcMissionBank::SetObjectiveWaypoint(Missions, MissionId, ObjectiveId, WorldLocation);
    // If this mission's markers are currently shown, refresh them so the move is visible.
    if (bOk && MarkedMissionId == MissionId)
    {
        SpawnMarkersForMission(MissionId);
    }
    return bOk;
}

int32 UGtcMissionEditorSubsystem::SpawnMarkersForMission(FGuid MissionId)
{
    ClearObjectiveMarkers();

    UWorld* W = GetWorld();
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, MissionId);
    if (!W || !M)
    {
        return 0;
    }

    int32 Count = 0;
    for (const FGtcObjectiveDefinition& O : M->Objectives)
    {
        if (!O.bHasWaypoint)
        {
            continue;
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AGtcMissionObjectiveMarker* Marker = W->SpawnActor<AGtcMissionObjectiveMarker>(O.Waypoint, FRotator::ZeroRotator, Params);
        if (Marker)
        {
            Marker->MissionId = MissionId;
            Marker->ObjectiveId = O.Id;
            Marker->Kind = O.Kind;
            Marker->SetLabel(O.Id.IsEmpty() ? O.Text.ToString() : O.Id);
            SpawnedMarkers.Add(Marker);
            ++Count;
        }
    }
    MarkedMissionId = MissionId;
    return Count;
}

void UGtcMissionEditorSubsystem::ClearObjectiveMarkers()
{
    for (TWeakObjectPtr<AGtcMissionObjectiveMarker>& Weak : SpawnedMarkers)
    {
        if (AGtcMissionObjectiveMarker* Marker = Weak.Get())
        {
            Marker->Destroy();
        }
    }
    SpawnedMarkers.Reset();
    MarkedMissionId.Invalidate();
}

int32 UGtcMissionEditorSubsystem::ArmWorldTriggers()
{
    bWorldTriggersEnabled = true;
    SpawnGiverMarkers();
    return GiverMarkers.Num();
}

void UGtcMissionEditorSubsystem::DisarmWorldTriggers()
{
    bWorldTriggersEnabled = false;
    ClearGiverMarkers();
}

bool UGtcMissionEditorSubsystem::SetMissionStart(FGuid MissionId, FVector WorldLocation)
{
    FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, MissionId);
    if (!M)
    {
        return false;
    }
    M->StartLocation = WorldLocation;
    M->bAvailableInWorld = true;
    if (bWorldTriggersEnabled)
    {
        SpawnGiverMarkers();
    }
    return true;
}

void UGtcMissionEditorSubsystem::SpawnGiverMarkers()
{
    ClearGiverMarkers();
    UWorld* W = GetWorld();
    if (!W)
    {
        return;
    }
    for (const FGtcMissionDefinition& M : Missions)
    {
        if (!GtcMissionBank::IsMissionAvailable(M, CompletedMissions))
        {
            continue;
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AGtcMissionObjectiveMarker* Marker = W->SpawnActor<AGtcMissionObjectiveMarker>(M.StartLocation, FRotator::ZeroRotator, Params);
        if (Marker)
        {
            Marker->MissionId = M.MissionId;
            Marker->ObjectiveId = TEXT("START");
            Marker->SetLabel(M.Title.IsEmpty() ? TEXT("Mission") : M.Title);
            Marker->SetLabelColor(FColor(80, 180, 255)); // blue giver blip
            GiverMarkers.Add(Marker);
        }
    }
}

void UGtcMissionEditorSubsystem::ClearGiverMarkers()
{
    for (TWeakObjectPtr<AGtcMissionObjectiveMarker>& Weak : GiverMarkers)
    {
        if (AGtcMissionObjectiveMarker* Marker = Weak.Get())
        {
            Marker->Destroy();
        }
    }
    GiverMarkers.Reset();
}

int32 UGtcMissionEditorSubsystem::NotifyObjectiveProgress(int32 Amount)
{
    if (!bPlaying || Amount <= 0)
    {
        return -1;
    }
    UGameInstance* GameInst = GameInstanceOf(this);
    UMissionController* Controller = GameInst ? GameInst->GetSubsystem<UMissionController>() : nullptr;
    if (!Controller || !Controller->IsActive())
    {
        return -1;
    }
    const FString ActiveId = Controller->CurrentObjectiveId();
    if (ActiveId.IsEmpty())
    {
        return -1;
    }
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, PlayingMissionId);
    if (!M)
    {
        return -1;
    }
    const FGtcObjectiveDefinition* O = M->Objectives.FindByPredicate(
        [&ActiveId](const FGtcObjectiveDefinition& Obj) { return Obj.Id == ActiveId; });
    if (!O || !GtcMissionBank::IsCountObjective(O->Kind))
    {
        return -1;
    }

    // Reset the tally when the active objective changes since the last call.
    if (ActiveId != ActiveCountObjId)
    {
        ActiveCountObjId = ActiveId;
        ActiveCountProgress = 0;
    }
    ActiveCountProgress += Amount;

    const int32 Target = FMath::Max(1, O->TargetCount);
    if (ActiveCountProgress >= Target)
    {
        ActiveCountProgress = 0;
        ActiveCountObjId.Reset();
        Controller->Complete(ActiveId);
        return 0;
    }
    return Target - ActiveCountProgress;
}

bool UGtcMissionEditorSubsystem::NotifyInteract()
{
    if (!bPlaying)
    {
        return false;
    }
    UGameInstance* GameInst = GameInstanceOf(this);
    UMissionController* Controller = GameInst ? GameInst->GetSubsystem<UMissionController>() : nullptr;
    if (!Controller || !Controller->IsActive())
    {
        return false;
    }
    const FString ActiveId = Controller->CurrentObjectiveId();
    if (ActiveId.IsEmpty())
    {
        return false;
    }
    const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, PlayingMissionId);
    const FGtcObjectiveDefinition* O = M
        ? M->Objectives.FindByPredicate([&ActiveId](const FGtcObjectiveDefinition& Obj) { return Obj.Id == ActiveId; })
        : nullptr;
    if (!O || O->Kind != EGtcObjectiveKind::Interact)
    {
        return false;
    }

    // If the objective has a location, require the player to be standing at it.
    if (O->bHasWaypoint)
    {
        FVector PlayerPos;
        if (!GetPlayerLocation(PlayerPos))
        {
            return false;
        }
        const FVector Target = Controller->CurrentWaypoint(PlayerPos);
        if (FVector::Dist(PlayerPos, Target) > FMath::Max(1.0, O->Radius))
        {
            return false;
        }
    }

    Controller->Complete(ActiveId);
    return true;
}

void UGtcMissionEditorSubsystem::CheckGiverProximity()
{
    if (GiverMarkers.Num() == 0)
    {
        return;
    }
    FVector PlayerPos;
    if (!GetPlayerLocation(PlayerPos))
    {
        return;
    }
    for (const TWeakObjectPtr<AGtcMissionObjectiveMarker>& Weak : GiverMarkers)
    {
        AGtcMissionObjectiveMarker* Giver = Weak.Get();
        if (!Giver)
        {
            continue;
        }
        const FGtcMissionDefinition* M = GtcMissionBank::Find(Missions, Giver->MissionId);
        if (!M)
        {
            continue;
        }
        if (FVector::Dist(PlayerPos, Giver->GetActorLocation()) <= M->StartRadius)
        {
            const FGuid Id = Giver->MissionId;
            ClearGiverMarkers(); // hide the blips while the mission runs
            PlayMission(Id);
            return;
        }
    }
}

// --- Console commands: a full god-mode workflow with no Blueprint assets required -------
//
// All under the "GTC.Mission." prefix. Address missions by their list index (see
// GTC.Mission.List). Registered once at module load via file-scope statics.

namespace
{
    UGtcMissionEditorSubsystem* ResolveEditor(UWorld* World)
    {
        return World ? World->GetSubsystem<UGtcMissionEditorSubsystem>() : nullptr;
    }

    // Resolve subsystem + the MissionId at the index given in Args[ArgPos]. Logs on failure.
    bool ResolveMission(UWorld* World, const TArray<FString>& Args, int32 ArgPos,
        UGtcMissionEditorSubsystem*& OutSub, FGuid& OutId)
    {
        OutSub = ResolveEditor(World);
        if (!OutSub)
        {
            UE_LOG(LogGtcMissionEditor, Warning, TEXT("No mission editor subsystem in this world."));
            return false;
        }
        if (!Args.IsValidIndex(ArgPos))
        {
            UE_LOG(LogGtcMissionEditor, Warning, TEXT("Expected a mission index argument."));
            return false;
        }
        const int32 Index = FCString::Atoi(*Args[ArgPos]);
        if (!OutSub->Missions.IsValidIndex(Index))
        {
            UE_LOG(LogGtcMissionEditor, Warning, TEXT("No mission at index %d (have %d)."), Index, OutSub->Missions.Num());
            return false;
        }
        OutId = OutSub->Missions[Index].MissionId;
        return true;
    }

    FString ContentTypeName(EGtcMissionContentType Type)
    {
        switch (Type)
        {
        case EGtcMissionContentType::SideQuest: return TEXT("side-quest");
        case EGtcMissionContentType::Activity:  return TEXT("activity");
        default:                                return TEXT("main");
        }
    }
}

static FAutoConsoleCommandWithWorldAndArgs GMissionNewCmd(
    TEXT("GTC.Mission.New"),
    TEXT("GTC.Mission.New [main|side|activity] — create a new mission, returns its index"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }

        EGtcMissionContentType Type = EGtcMissionContentType::MainMission;
        if (Args.IsValidIndex(0))
        {
            const FString A = Args[0].ToLower();
            if (A == TEXT("side")) { Type = EGtcMissionContentType::SideQuest; }
            else if (A == TEXT("activity")) { Type = EGtcMissionContentType::Activity; }
        }
        const FGuid Id = Sub->CreateMission(Type);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Created %s mission #%d (id=%s). Edit with GTC.Mission.AddObjective."),
            *ContentTypeName(Type), Sub->Missions.Num() - 1, *Id.ToString());
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionListCmd(
    TEXT("GTC.Mission.List"),
    TEXT("GTC.Mission.List — list all authored missions"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }

        const TArray<FGtcMissionSummary> Summaries = Sub->GetMissionSummaries();
        UE_LOG(LogGtcMissionEditor, Display, TEXT("%d mission(s):"), Summaries.Num());
        for (int32 i = 0; i < Summaries.Num(); ++i)
        {
            const FGtcMissionSummary& S = Summaries[i];
            UE_LOG(LogGtcMissionEditor, Display, TEXT("  #%d  \"%s\"  [%s]  objectives=%d  %s"),
                i, *S.Title, *ContentTypeName(S.ContentType), S.ObjectiveCount,
                S.bValid ? TEXT("OK") : TEXT("** INVALID **"));
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionAddObjectiveCmd(
    TEXT("GTC.Mission.AddObjective"),
    TEXT("GTC.Mission.AddObjective <index> <objId> [text...] — append an objective"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }

        FGtcObjectiveDefinition O;
        O.Id = Args.IsValidIndex(1) ? Args[1] : TEXT("objective");
        FString Text;
        for (int32 i = 2; i < Args.Num(); ++i)
        {
            Text += (i > 2 ? TEXT(" ") : TEXT("")) + Args[i];
        }
        O.Text = FText::FromString(Text.IsEmpty() ? O.Id : Text);
        const bool bOk = Sub->AddObjective(Id, O);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("AddObjective '%s' -> %s"), *O.Id, bOk ? TEXT("ok") : TEXT("FAILED"));
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionPlaceHereCmd(
    TEXT("GTC.Mission.PlaceHere"),
    TEXT("GTC.Mission.PlaceHere <index> <objId> — set an objective's waypoint to the player's location"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }
        if (!Args.IsValidIndex(1)) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("Expected <objId>.")); return; }

        FVector Loc = FVector::ZeroVector;
        if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
        {
            if (APawn* P = PC->GetPawn())
            {
                Loc = P->GetActorLocation();
            }
        }
        const bool bOk = Sub->SetObjectiveWaypoint(Id, Args[1], Loc);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("PlaceHere '%s' at %s -> %s"),
            *Args[1], *Loc.ToCompactString(), bOk ? TEXT("ok") : TEXT("FAILED"));
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionValidateCmd(
    TEXT("GTC.Mission.Validate"),
    TEXT("GTC.Mission.Validate <index> — print validation errors/warnings"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }

        TArray<FString> Errors, Warnings;
        const bool bValid = Sub->ValidateMission(Id, Errors, Warnings);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Validate -> %s (%d error(s), %d warning(s))"),
            bValid ? TEXT("VALID") : TEXT("INVALID"), Errors.Num(), Warnings.Num());
        for (const FString& E : Errors)   { UE_LOG(LogGtcMissionEditor, Warning, TEXT("  ERROR: %s"), *E); }
        for (const FString& W : Warnings) { UE_LOG(LogGtcMissionEditor, Display, TEXT("  warn:  %s"), *W); }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionPlayCmd(
    TEXT("GTC.Mission.Play"),
    TEXT("GTC.Mission.Play <index> — load and start a narrative mission on the live runtime"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }
        const bool bOk = Sub->PlayMission(Id);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Play -> %s"), bOk ? TEXT("started") : TEXT("FAILED (see warnings)"));
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionSetStartCmd(
    TEXT("GTC.Mission.SetStart"),
    TEXT("GTC.Mission.SetStart <index> — mark a mission available in-world with its start at your location"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }
        FVector Loc = FVector::ZeroVector;
        if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
        {
            if (APawn* P = PC->GetPawn()) { Loc = P->GetActorLocation(); }
        }
        const bool bOk = Sub->SetMissionStart(Id, Loc);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("SetStart at %s -> %s"), *Loc.ToCompactString(), bOk ? TEXT("ok") : TEXT("FAILED"));
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionArmCmd(
    TEXT("GTC.Mission.Arm"),
    TEXT("GTC.Mission.Arm — show world start-markers; walk into one to start that mission"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        if (UGtcMissionEditorSubsystem* Sub = ResolveEditor(World))
        {
            const int32 N = Sub->ArmWorldTriggers();
            UE_LOG(LogGtcMissionEditor, Display, TEXT("Armed %d world mission trigger(s)."), N);
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionDisarmCmd(
    TEXT("GTC.Mission.Disarm"),
    TEXT("GTC.Mission.Disarm — hide world start-markers"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        if (UGtcMissionEditorSubsystem* Sub = ResolveEditor(World))
        {
            Sub->DisarmWorldTriggers();
            UE_LOG(LogGtcMissionEditor, Display, TEXT("World triggers disarmed."));
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionProgressCmd(
    TEXT("GTC.Mission.Progress"),
    TEXT("GTC.Mission.Progress [n] — advance the active Collect/Eliminate objective by n (default 1)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }
        const int32 N = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) : 1;
        const int32 Remaining = Sub->NotifyObjectiveProgress(N);
        if (Remaining < 0)
        {
            UE_LOG(LogGtcMissionEditor, Warning, TEXT("No active count (Collect/Eliminate) objective."));
        }
        else
        {
            UE_LOG(LogGtcMissionEditor, Display, TEXT("Progress +%d -> %d remaining."), N, Remaining);
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionUnlocksCmd(
    TEXT("GTC.Mission.Unlocks"),
    TEXT("GTC.Mission.Unlocks — list feature/item ids granted by completed missions"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }
        const TArray<FString> Unlocks = Sub->GetUnlocks();
        UE_LOG(LogGtcMissionEditor, Display, TEXT("%d unlock(s) granted:"), Unlocks.Num());
        for (const FString& U : Unlocks)
        {
            UE_LOG(LogGtcMissionEditor, Display, TEXT("  %s"), *U);
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionInteractCmd(
    TEXT("GTC.Mission.Interact"),
    TEXT("GTC.Mission.Interact — complete the active Interact objective (must be in range)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }
        const bool bOk = Sub->NotifyInteract();
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Interact -> %s"), bOk ? TEXT("done") : TEXT("no interact objective in range"));
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionCompleteCmd(
    TEXT("GTC.Mission.Complete"),
    TEXT("GTC.Mission.Complete [objId] — complete the active (or named) objective; for count/interact kinds"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGameInstance* GameInst = World ? World->GetGameInstance() : nullptr;
        UMissionController* Controller = GameInst ? GameInst->GetSubsystem<UMissionController>() : nullptr;
        if (!Controller || !Controller->IsActive())
        {
            UE_LOG(LogGtcMissionEditor, Warning, TEXT("No active mission to complete."));
            return;
        }
        const FString ObjId = Args.IsValidIndex(0) ? Args[0] : Controller->CurrentObjectiveId();
        Controller->Complete(ObjId);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Completed objective '%s'."), *ObjId);
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionSaveCmd(
    TEXT("GTC.Mission.Save"),
    TEXT("GTC.Mission.Save <index> [absolutePath] — write an openable .mission.json"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }

        const int32 Index = FCString::Atoi(*Args[0]);
        FString Path = Args.IsValidIndex(1)
            ? Args[1]
            : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Missions"), Sub->Missions[Index].Id + TEXT(".mission.json"));
        const bool bOk = Sub->SaveMissionToFile(Id, Path);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Save -> %s (%s)"), bOk ? TEXT("ok") : TEXT("FAILED"), *Path);
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionLoadCmd(
    TEXT("GTC.Mission.Load"),
    TEXT("GTC.Mission.Load <absolutePath> — load a .mission.json into the bank"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = ResolveEditor(World);
        if (!Sub) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("No subsystem.")); return; }
        if (!Args.IsValidIndex(0)) { UE_LOG(LogGtcMissionEditor, Warning, TEXT("Expected <path>.")); return; }
        const bool bOk = Sub->LoadMissionFromFile(Args[0]);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Load -> %s (now %d mission(s))"), bOk ? TEXT("ok") : TEXT("FAILED"), Sub->Missions.Num());
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionSpawnCmd(
    TEXT("GTC.Mission.Spawn"),
    TEXT("GTC.Mission.Spawn <index> — spawn visible markers at the mission's objective waypoints"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UGtcMissionEditorSubsystem* Sub = nullptr;
        FGuid Id;
        if (!ResolveMission(World, Args, 0, Sub, Id)) { return; }
        const int32 N = Sub->SpawnMarkersForMission(Id);
        UE_LOG(LogGtcMissionEditor, Display, TEXT("Spawned %d marker(s)."), N);
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionClearCmd(
    TEXT("GTC.Mission.Clear"),
    TEXT("GTC.Mission.Clear — destroy all spawned objective markers"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        if (UGtcMissionEditorSubsystem* Sub = ResolveEditor(World))
        {
            Sub->ClearObjectiveMarkers();
            UE_LOG(LogGtcMissionEditor, Display, TEXT("Cleared markers."));
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GMissionToggleCmd(
    TEXT("GTC.Mission.Toggle"),
    TEXT("GTC.Mission.Toggle — toggle god/creator mode"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
    {
        if (UGtcMissionEditorSubsystem* Sub = ResolveEditor(World))
        {
            Sub->ToggleGodMode();
            UE_LOG(LogGtcMissionEditor, Display, TEXT("God mode: %s"), Sub->bGodModeActive ? TEXT("ON") : TEXT("OFF"));
        }
    }));
