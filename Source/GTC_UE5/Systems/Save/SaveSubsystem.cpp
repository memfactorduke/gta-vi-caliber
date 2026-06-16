// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSubsystem.h"

#include "SaveData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Hooks.Reset();
    Index.Reset();
}

void USaveSubsystem::Deinitialize()
{
    // Drop every hook so none survive the GameInstance (hooks are non-owning; this is the
    // subsystem's only lifecycle guarantee toward dangling captures).
    Hooks.Reset();
    Index.Reset();
    Super::Deinitialize();
}

bool USaveSubsystem::RegisterHook(const FString& Section, const FSaveHook& OnSave, const FLoadHook& OnLoad)
{
    if (Section.IsEmpty())
    {
        return false;
    }
    if (const int32* Existing = Index.Find(Section))
    {
        Hooks[*Existing].OnSave = OnSave;
        Hooks[*Existing].OnLoad = OnLoad;
        return true;
    }
    const int32 NewIndex = Hooks.Num();
    Hooks.Add(FHookEntry{ Section, OnSave, OnLoad });
    Index.Add(Section, NewIndex);
    return true;
}

bool USaveSubsystem::UnregisterHook(const FString& Section)
{
    const int32* Found = Index.Find(Section);
    if (Found == nullptr)
    {
        return false;
    }
    const int32 RemoveAt = *Found;
    Hooks.RemoveAt(RemoveAt);
    // Rebuild the index: the array shifted, so cached indices past RemoveAt are stale.
    Index.Reset();
    for (int32 i = 0; i < Hooks.Num(); ++i)
    {
        Index.Add(Hooks[i].Section, i);
    }
    return true;
}

int32 USaveSubsystem::HookCount() const
{
    return Hooks.Num();
}

bool USaveSubsystem::HasHook(const FString& Section) const
{
    return Index.Contains(Section);
}

TArray<FString> USaveSubsystem::Sections() const
{
    TArray<FString> Out;
    Out.Reserve(Hooks.Num());
    for (const FHookEntry& Entry : Hooks)
    {
        Out.Add(Entry.Section);
    }
    return Out;
}

FString USaveSubsystem::BuildSaveText()
{
    const TSharedRef<FGtcJsonObject> Snapshot = MakeShared<FGtcJsonObject>();
    for (const FHookEntry& Entry : Hooks)
    {
        const TSharedRef<FGtcJsonObject> Section = MakeShared<FGtcJsonObject>();
        Entry.OnSave.ExecuteIfBound(Section);
        Snapshot->SetObject(Entry.Section, Section);
    }
    return GtcSaveData::Encode(Snapshot);
}

bool USaveSubsystem::ApplySaveText(const FString& Text)
{
    const int32 SaveVersion = GtcSaveData::VersionOf(Text);
    const TSharedRef<FGtcJsonObject> Migrated =
        GtcSaveData::Migrate(GtcSaveData::Decode(Text), SaveVersion);
    if (Migrated->Num() == 0)
    {
        // Empty == no saved data: restore nothing (corrupt/empty save can't wipe state).
        return false;
    }
    for (const FHookEntry& Entry : Hooks)
    {
        // Missing section -> hand the hook an empty object ("no saved data for me").
        const TSharedPtr<FGtcJsonObject> Section = Migrated->GetObject(Entry.Section);
        const TSharedRef<FGtcJsonObject> In = Section.IsValid()
            ? Section.ToSharedRef()
            : MakeShared<FGtcJsonObject>();
        Entry.OnLoad.ExecuteIfBound(In);
    }
    return true;
}

bool USaveSubsystem::SaveToFile(const FString& AbsolutePath)
{
    return FFileHelper::SaveStringToFile(BuildSaveText(), *AbsolutePath);
}

bool USaveSubsystem::LoadFromFile(const FString& AbsolutePath)
{
    if (!FPaths::FileExists(AbsolutePath))
    {
        return false;
    }
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
    {
        return false;
    }
    return ApplySaveText(Text);
}

FString USaveSubsystem::DefaultSavePath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("savegame.json"));
}
