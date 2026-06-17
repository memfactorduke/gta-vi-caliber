// Copyright (c) 2026 GTC contributors

#include "WorldSerializerSubsystem.h"
#include "SaveSubsystem.h"

const FString UWorldSerializerSubsystem::WorldSection = TEXT("world");

void UWorldSerializerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Ensure USaveSubsystem exists before us, then register our world hook with it.
    if (UGameInstance* GI = GetGameInstance())
    {
        Collection.InitializeDependency(USaveSubsystem::StaticClass());
        RegisterWith(GI->GetSubsystem<USaveSubsystem>());
    }
}

void UWorldSerializerSubsystem::Deinitialize()
{
    // Honor the non-owning hook contract: drop our hook before `this` dies.
    UnregisterFromSave();
    Super::Deinitialize();
}

bool UWorldSerializerSubsystem::RegisterWith(USaveSubsystem* Save)
{
    if (Save == nullptr)
    {
        return false;
    }

    FSaveHook OnSave;
    OnSave.BindUObject(this, &UWorldSerializerSubsystem::OnSaveWorld);
    FLoadHook OnLoad;
    OnLoad.BindUObject(this, &UWorldSerializerSubsystem::OnLoadWorld);

    if (!Save->RegisterHook(WorldSection, OnSave, OnLoad))
    {
        return false;
    }

    RegisteredSave = Save;
    bRegistered = true;
    return true;
}

void UWorldSerializerSubsystem::UnregisterFromSave()
{
    if (RegisteredSave)
    {
        RegisteredSave->UnregisterHook(WorldSection);
    }
    RegisteredSave = nullptr;
    bRegistered = false;
}

void UWorldSerializerSubsystem::OnSaveWorld(const TSharedRef<FGtcJsonObject>& SectionOut)
{
    // Copy our live world state into the section (preserving Godot-observable key order).
    for (const FString& Key : WorldState->OrderedKeys())
    {
        const FGtcJsonValuePtr Value = WorldState->Get(Key);
        SectionOut->Set(Key, Value.IsValid() ? Value->DeepClone() : Value);
    }
}

void UWorldSerializerSubsystem::OnLoadWorld(const TSharedRef<FGtcJsonObject>& SectionIn)
{
    // Restore live world state from the section. A deep clone keeps us independent of the
    // decoded snapshot. An empty section (no saved "world") restores to empty, mirroring
    // Godot snapshot.get(key, {}).
    WorldState = SectionIn->DeepClone();
}
