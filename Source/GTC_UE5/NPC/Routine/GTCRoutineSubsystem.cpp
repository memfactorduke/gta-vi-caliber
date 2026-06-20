// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCRoutineSubsystem.h"

#include "NpcRoutineJson.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGtcRoutine, Log, All);

namespace
{
    // A small, varied set of individual "real person" days, used out-of-the-box when
    // no bank file exists on disk yet. Citizens seed-pick among these (then layer the
    // usual ±45min jitter + needs overrides), so a fresh world already reads as people
    // living different lives — and the whole set is editable: GTC.Routine.SaveBank
    // writes them out, then hand/AI-edit the JSON and GTC.Routine.Reload. Every routine
    // covers the full 24h (no dead gaps). Vocabulary matches the archetype schedules:
    // activities sleep/work/eat/socialize/loiter/goof_off/commute; places office/diner/
    // gym/bar/home/park/street.
    TArray<FNpcRoutine> GtcBuildDefaultRoutines()
    {
        auto Make = [](const TCHAR* Id, const TCHAR* Name, TArray<FNpcScheduleBlock> Blocks)
        {
            FNpcRoutine R; R.Id = Id; R.DisplayName = Name; R.Blocks = MoveTemp(Blocks); return R;
        };
        return {
            Make(TEXT("early_bird_office"), TEXT("Early-Bird Office Worker"), {
                {6.0, 9.0,  TEXT("goof_off"),  TEXT("gym")},
                {9.0, 12.5, TEXT("work"),      TEXT("office")},
                {12.5, 13.5, TEXT("eat"),      TEXT("diner")},
                {13.5, 17.5, TEXT("work"),     TEXT("office")},
                {17.5, 19.0, TEXT("socialize"), TEXT("bar")},
                {19.0, 22.0, TEXT("loiter"),   TEXT("home")},
                {22.0, 6.0, TEXT("sleep"),     TEXT("home")},
            }),
            Make(TEXT("night_owl_bartender"), TEXT("Night-Owl Bartender"), {
                {4.0, 13.0, TEXT("sleep"),     TEXT("home")},
                {13.0, 16.0, TEXT("loiter"),   TEXT("home")},
                {16.0, 18.0, TEXT("goof_off"), TEXT("gym")},
                {18.0, 21.0, TEXT("eat"),      TEXT("diner")},
                {21.0, 4.0, TEXT("work"),      TEXT("bar")},
            }),
            Make(TEXT("cafe_freelancer"), TEXT("Cafe Freelancer"), {
                {8.0, 9.5,  TEXT("goof_off"),  TEXT("park")},
                {9.5, 13.0, TEXT("work"),      TEXT("diner")},
                {13.0, 14.0, TEXT("eat"),      TEXT("diner")},
                {14.0, 17.0, TEXT("work"),     TEXT("diner")},
                {17.0, 19.0, TEXT("goof_off"), TEXT("gym")},
                {19.0, 23.0, TEXT("socialize"), TEXT("bar")},
                {23.0, 8.0, TEXT("sleep"),     TEXT("home")},
            }),
            Make(TEXT("park_retiree"), TEXT("Park-Bench Retiree"), {
                {6.0, 9.0,  TEXT("loiter"),    TEXT("park")},
                {9.0, 12.0, TEXT("goof_off"),  TEXT("park")},
                {12.0, 13.0, TEXT("eat"),      TEXT("diner")},
                {13.0, 17.0, TEXT("loiter"),   TEXT("park")},
                {17.0, 20.0, TEXT("socialize"), TEXT("bar")},
                {20.0, 6.0, TEXT("sleep"),     TEXT("home")},
            }),
            Make(TEXT("campus_student"), TEXT("Campus Student"), {
                {8.0, 9.0,  TEXT("commute"),   TEXT("street")},
                {9.0, 12.0, TEXT("work"),      TEXT("office")},
                {12.0, 13.0, TEXT("eat"),      TEXT("diner")},
                {13.0, 16.0, TEXT("work"),     TEXT("office")},
                {16.0, 18.0, TEXT("goof_off"), TEXT("gym")},
                {18.0, 24.0, TEXT("socialize"), TEXT("bar")},
                {0.0, 8.0,  TEXT("sleep"),     TEXT("home")},
            }),
            Make(TEXT("remote_homebody"), TEXT("Remote Homebody"), {
                {7.0, 10.0, TEXT("loiter"),    TEXT("home")},
                {10.0, 12.0, TEXT("work"),     TEXT("home")},
                {12.0, 13.0, TEXT("eat"),      TEXT("home")},
                {13.0, 17.0, TEXT("work"),     TEXT("home")},
                {17.0, 19.0, TEXT("goof_off"), TEXT("park")},
                {19.0, 23.0, TEXT("loiter"),   TEXT("home")},
                {23.0, 7.0, TEXT("sleep"),     TEXT("home")},
            }),
        };
    }
}

// --- UWorldSubsystem ---------------------------------------------------------

bool UGTCRoutineSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

void UGTCRoutineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Pick up any hand/AI-authored bank sitting on disk so edits survive a restart.
    LoadBank(FString());
    // No bank on disk yet -> seed the built-in "real person" days so a fresh world
    // already has individual routines (and SaveBank can write them out for editing).
    if (Routines.Num() == 0)
    {
        Routines = GtcBuildDefaultRoutines();
        UE_LOG(LogGtcRoutine, Log, TEXT("No routine bank on disk; seeded %d built-in routines."), Routines.Num());
    }
    else
    {
        UE_LOG(LogGtcRoutine, Log, TEXT("Routine store ready (%d routine%s loaded from disk)."),
            Routines.Num(), Routines.Num() == 1 ? TEXT("") : TEXT("s"));
    }
}

// --- Resolution --------------------------------------------------------------

const FNpcRoutine* UGTCRoutineSubsystem::ResolveRoutineForSeed(int32 Seed) const
{
    if (Routines.Num() == 0)
    {
        return nullptr;
    }
    if (const FString* Assigned = Assignments.Find(Seed))
    {
        if (const FNpcRoutine* R = FNpcRoutineLibrary::Find(Routines, *Assigned))
        {
            return R;
        }
    }
    // No explicit assignment (or it pointed at a deleted routine): give this seed a
    // STABLE individual routine from the bank rather than the flat archetype default.
    return FNpcRoutineLibrary::Find(Routines, FNpcRoutineLibrary::PickIdForSeed(Routines, Seed));
}

bool UGTCRoutineSubsystem::ResolveBlocksForSeed(int32 Seed, TArray<FNpcScheduleBlock>& Out) const
{
    if (const FNpcRoutine* R = ResolveRoutineForSeed(Seed))
    {
        Out = R->Blocks;
        return true;
    }
    return false;
}

// --- CRUD --------------------------------------------------------------------

FNpcRoutine* UGTCRoutineSubsystem::FindMutable(const FString& Id)
{
    for (FNpcRoutine& R : Routines)
    {
        if (R.Id.Equals(Id, ESearchCase::IgnoreCase))
        {
            return &R;
        }
    }
    return nullptr;
}

bool UGTCRoutineSubsystem::CreateRoutine(const FString& Id, const FString& DisplayName)
{
    if (Id.IsEmpty() || FindMutable(Id) != nullptr)
    {
        return false;
    }
    FNpcRoutine R;
    R.Id = Id;
    R.DisplayName = DisplayName.IsEmpty() ? Id : DisplayName;
    Routines.Add(MoveTemp(R));
    return true;
}

bool UGTCRoutineSubsystem::AddBlock(const FString& Id, float StartHour, float EndHour, const FString& Activity, const FString& Place)
{
    FNpcRoutine* R = FindMutable(Id);
    if (!R)
    {
        if (!CreateRoutine(Id, Id))
        {
            return false;
        }
        R = FindMutable(Id);
    }
    if (!R)
    {
        return false;
    }
    R->Blocks.Add(FNpcScheduleBlock(StartHour, EndHour, Activity, Place));
    return true;
}

bool UGTCRoutineSubsystem::RemoveBlock(const FString& Id, int32 Index)
{
    FNpcRoutine* R = FindMutable(Id);
    if (!R || !R->Blocks.IsValidIndex(Index))
    {
        return false;
    }
    R->Blocks.RemoveAt(Index);
    return true;
}

bool UGTCRoutineSubsystem::DeleteRoutine(const FString& Id)
{
    const int32 Removed = Routines.RemoveAll(
        [&Id](const FNpcRoutine& R) { return R.Id.Equals(Id, ESearchCase::IgnoreCase); });
    return Removed > 0;
}

bool UGTCRoutineSubsystem::AssignRoutineToSeed(int32 Seed, const FString& RoutineId)
{
    if (!FNpcRoutineLibrary::Find(Routines, RoutineId))
    {
        return false;
    }
    Assignments.Add(Seed, RoutineId);
    return true;
}

FString UGTCRoutineSubsystem::GetAssignedRoutineId(int32 Seed) const
{
    const FString* Found = Assignments.Find(Seed);
    return Found ? *Found : FString();
}

// --- Persistence -------------------------------------------------------------

bool UGTCRoutineSubsystem::SaveRoutine(const FString& Id, const FString& AbsolutePath)
{
    const FNpcRoutine* R = FNpcRoutineLibrary::Find(Routines, Id);
    if (!R)
    {
        return false;
    }
    const FString Path = AbsolutePath.IsEmpty() ? NpcRoutineJson::DefaultPathFor(Id) : AbsolutePath;
    return NpcRoutineJson::SaveRoutineToFile(*R, Path);
}

bool UGTCRoutineSubsystem::LoadRoutineFile(const FString& AbsolutePath)
{
    FNpcRoutine R;
    if (!NpcRoutineJson::LoadRoutineFromFile(AbsolutePath, R))
    {
        return false;
    }
    // Replace any same-id entry, else append.
    if (FNpcRoutine* Existing = FindMutable(R.Id))
    {
        *Existing = MoveTemp(R);
    }
    else
    {
        Routines.Add(MoveTemp(R));
    }
    return true;
}

bool UGTCRoutineSubsystem::SaveBank(const FString& AbsolutePath)
{
    const FString Path = AbsolutePath.IsEmpty() ? NpcRoutineJson::DefaultBankPath() : AbsolutePath;
    return NpcRoutineJson::SaveBankToFile(Routines, Path);
}

bool UGTCRoutineSubsystem::LoadBank(const FString& AbsolutePath)
{
    const FString Path = AbsolutePath.IsEmpty() ? NpcRoutineJson::DefaultBankPath() : AbsolutePath;
    TArray<FNpcRoutine> Loaded;
    if (!NpcRoutineJson::LoadBankFromFile(Path, Loaded))
    {
        return false;
    }
    Routines = MoveTemp(Loaded);
    return true;
}

bool UGTCRoutineSubsystem::ReloadDefaultBank()
{
    return LoadBank(FString());
}

// --- Console surface: GTC.Routine.* ------------------------------------------

namespace
{
    UGTCRoutineSubsystem* GtcResolveRoutineEditor(UWorld* World)
    {
        UGTCRoutineSubsystem* Sub = World ? World->GetSubsystem<UGTCRoutineSubsystem>() : nullptr;
        if (!Sub)
        {
            UE_LOG(LogGtcRoutine, Warning, TEXT("No routine subsystem in this world."));
        }
        return Sub;
    }

    FAutoConsoleCommandWithWorldAndArgs GRoutineListCmd(
        TEXT("GTC.Routine.List"),
        TEXT("GTC.Routine.List — list every routine in the live bank"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub) { return; }
                const TArray<FNpcRoutine>& All = Sub->GetRoutines();
                UE_LOG(LogGtcRoutine, Display, TEXT("%d routine(s):"), All.Num());
                for (const FNpcRoutine& R : All)
                {
                    UE_LOG(LogGtcRoutine, Display, TEXT("  %s \"%s\" — %d block(s), full-day=%s"),
                        *R.Id, *R.DisplayName, R.Blocks.Num(),
                        FNpcRoutineLibrary::CoversFullDay(R) ? TEXT("yes") : TEXT("NO"));
                }
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineShowCmd(
        TEXT("GTC.Routine.Show"),
        TEXT("GTC.Routine.Show <id> — print a routine's blocks"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || !Args.IsValidIndex(0)) { return; }
                const FNpcRoutine* R = FNpcRoutineLibrary::Find(Sub->GetRoutines(), Args[0]);
                if (!R) { UE_LOG(LogGtcRoutine, Warning, TEXT("No routine '%s'."), *Args[0]); return; }
                UE_LOG(LogGtcRoutine, Display, TEXT("%s \"%s\":"), *R->Id, *R->DisplayName);
                for (int32 i = 0; i < R->Blocks.Num(); ++i)
                {
                    const FNpcScheduleBlock& B = R->Blocks[i];
                    UE_LOG(LogGtcRoutine, Display, TEXT("  [%d] %5.2f-%5.2f  %s @ %s"),
                        i, B.Start, B.End, *B.Activity, *B.Place);
                }
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineNewCmd(
        TEXT("GTC.Routine.New"),
        TEXT("GTC.Routine.New <id> [display name...] — create an empty routine"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || !Args.IsValidIndex(0)) { return; }
                FString Name;
                for (int32 i = 1; i < Args.Num(); ++i) { Name += (i > 1 ? TEXT(" ") : TEXT("")) + Args[i]; }
                const bool bOk = Sub->CreateRoutine(Args[0], Name);
                UE_LOG(LogGtcRoutine, Display, TEXT("New '%s' -> %s"), *Args[0], bOk ? TEXT("ok") : TEXT("FAILED (exists/empty)"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineAddBlockCmd(
        TEXT("GTC.Routine.AddBlock"),
        TEXT("GTC.Routine.AddBlock <id> <start> <end> <activity> <place> — append a block"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || Args.Num() < 5) { UE_LOG(LogGtcRoutine, Warning, TEXT("usage: <id> <start> <end> <activity> <place>")); return; }
                const bool bOk = Sub->AddBlock(Args[0], FCString::Atof(*Args[1]), FCString::Atof(*Args[2]), Args[3], Args[4]);
                UE_LOG(LogGtcRoutine, Display, TEXT("AddBlock %s -> %s"), *Args[0], bOk ? TEXT("ok") : TEXT("FAILED"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineRemoveBlockCmd(
        TEXT("GTC.Routine.RemoveBlock"),
        TEXT("GTC.Routine.RemoveBlock <id> <index> — remove a block"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || Args.Num() < 2) { return; }
                const bool bOk = Sub->RemoveBlock(Args[0], FCString::Atoi(*Args[1]));
                UE_LOG(LogGtcRoutine, Display, TEXT("RemoveBlock %s[%s] -> %s"), *Args[0], *Args[1], bOk ? TEXT("ok") : TEXT("FAILED"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineValidateCmd(
        TEXT("GTC.Routine.Validate"),
        TEXT("GTC.Routine.Validate <id> — report full-day coverage + overlap"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || !Args.IsValidIndex(0)) { return; }
                const FNpcRoutine* R = FNpcRoutineLibrary::Find(Sub->GetRoutines(), Args[0]);
                if (!R) { UE_LOG(LogGtcRoutine, Warning, TEXT("No routine '%s'."), *Args[0]); return; }
                const bool bFull = FNpcRoutineLibrary::CoversFullDay(*R);
                const int32 Overlap = FNpcRoutineLibrary::FirstOverlap(*R);
                UE_LOG(LogGtcRoutine, Display, TEXT("'%s': full-day=%s, overlap=%s"),
                    *R->Id, bFull ? TEXT("yes") : TEXT("NO (gap)"),
                    Overlap == INDEX_NONE ? TEXT("none") : *FString::Printf(TEXT("block %d shadowed"), Overlap));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineAssignCmd(
        TEXT("GTC.Routine.Assign"),
        TEXT("GTC.Routine.Assign <seed> <routineId> — give a citizen seed a routine"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || Args.Num() < 2) { return; }
                const bool bOk = Sub->AssignRoutineToSeed(FCString::Atoi(*Args[0]), Args[1]);
                UE_LOG(LogGtcRoutine, Display, TEXT("Assign seed %s -> '%s' : %s"),
                    *Args[0], *Args[1], bOk ? TEXT("ok") : TEXT("FAILED (no such routine)"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineSaveCmd(
        TEXT("GTC.Routine.Save"),
        TEXT("GTC.Routine.Save <id> [absPath] — write a .routine.json"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || !Args.IsValidIndex(0)) { return; }
                const FString Path = Args.IsValidIndex(1) ? Args[1] : FString();
                const bool bOk = Sub->SaveRoutine(Args[0], Path);
                UE_LOG(LogGtcRoutine, Display, TEXT("Save '%s' -> %s"), *Args[0], bOk ? TEXT("ok") : TEXT("FAILED"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineLoadCmd(
        TEXT("GTC.Routine.Load"),
        TEXT("GTC.Routine.Load <absPath> — load one .routine.json into the bank"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub || !Args.IsValidIndex(0)) { return; }
                const bool bOk = Sub->LoadRoutineFile(Args[0]);
                UE_LOG(LogGtcRoutine, Display, TEXT("Load %s -> %s"), *Args[0], bOk ? TEXT("ok") : TEXT("FAILED"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineSaveBankCmd(
        TEXT("GTC.Routine.SaveBank"),
        TEXT("GTC.Routine.SaveBank [absPath] — write the whole bank (default Saved/Routines)"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub) { return; }
                const bool bOk = Sub->SaveBank(Args.IsValidIndex(0) ? Args[0] : FString());
                UE_LOG(LogGtcRoutine, Display, TEXT("SaveBank -> %s"), bOk ? TEXT("ok") : TEXT("FAILED"));
            }));

    FAutoConsoleCommandWithWorldAndArgs GRoutineReloadCmd(
        TEXT("GTC.Routine.Reload"),
        TEXT("GTC.Routine.Reload — reload the default bank file (hot-reload after editing JSON)"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args, UWorld* World)
            {
                UGTCRoutineSubsystem* Sub = GtcResolveRoutineEditor(World);
                if (!Sub) { return; }
                const bool bOk = Sub->ReloadDefaultBank();
                UE_LOG(LogGtcRoutine, Display, TEXT("Reload -> %s (%d routines)"),
                    bOk ? TEXT("ok") : TEXT("FAILED/none"), Sub->GetRoutines().Num());
            }));
}
