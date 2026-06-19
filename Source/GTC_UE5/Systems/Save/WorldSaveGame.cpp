// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldSaveGame.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace FWorldSaveGame
{
FString DefaultPath()
{
    // user://savegame.json analogue: under the project's Saved/ dir.
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("savegame.json"));
}

FString Serialize(const TSharedRef<FGtcJsonObject>& State)
{
    // {"version": VERSION, "state": state} — same envelope shape as Godot SaveGame.serialize.
    const TSharedRef<FGtcJsonObject> Wrapper = MakeShared<FGtcJsonObject>();
    Wrapper->SetNumber(TEXT("version"), Version);
    Wrapper->SetObject(TEXT("state"), State);
    return GtcJson::Serialize(Wrapper);
}

TSharedRef<FGtcJsonObject> Deserialize(const FString& Text)
{
    // Empty/blank input is the normal first-run path; parse silently, never log.
    if (Text.TrimStartAndEnd().IsEmpty())
    {
        return MakeShared<FGtcJsonObject>();
    }

    const FGtcJsonValuePtr Root = GtcJson::Parse(Text);
    if (!Root.IsValid() || !Root->IsObject())
    {
        return MakeShared<FGtcJsonObject>();
    }

    const TSharedPtr<FGtcJsonObject> Obj = Root->AsObject();

    // Godot: int(parsed.get("version", 0)) == VERSION and parsed.has("state").
    const int32 ParsedVersion = static_cast<int32>(Obj->GetNumber(TEXT("version"), 0.0));
    if (ParsedVersion != Version || !Obj->Has(TEXT("state")))
    {
        return MakeShared<FGtcJsonObject>();
    }

    // `state` present but not an object (e.g. forged scalar) -> empty, never a crash.
    const TSharedPtr<FGtcJsonObject> State = Obj->GetObject(TEXT("state"));
    if (!State.IsValid())
    {
        return MakeShared<FGtcJsonObject>();
    }
    return State.ToSharedRef();
}

bool Write(const TSharedRef<FGtcJsonObject>& State, const FString& Path)
{
    return FFileHelper::SaveStringToFile(Serialize(State), *Path);
}

bool Write(const TSharedRef<FGtcJsonObject>& State)
{
    return Write(State, DefaultPath());
}

TSharedRef<FGtcJsonObject> Read(const FString& Path)
{
    if (!FPaths::FileExists(Path))
    {
        return MakeShared<FGtcJsonObject>();
    }
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *Path))
    {
        return MakeShared<FGtcJsonObject>();
    }
    return Deserialize(Text);
}

TSharedRef<FGtcJsonObject> Read()
{
    return Read(DefaultPath());
}

bool HasSave(const FString& Path)
{
    return FPaths::FileExists(Path);
}

bool HasSave()
{
    return HasSave(DefaultPath());
}
}
