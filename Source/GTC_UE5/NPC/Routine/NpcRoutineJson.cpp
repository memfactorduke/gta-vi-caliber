// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcRoutineJson.h"

#include "../../Systems/Save/SaveJson.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
    // File-local helpers, prefixed to avoid unity-build symbol collisions.

    TSharedRef<FGtcJsonObject> GtcRoutineBlockToObject(const FNpcScheduleBlock& B)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetNumber(TEXT("start"), B.Start);
        O->SetNumber(TEXT("end"), B.End);
        O->SetString(TEXT("activity"), B.Activity);
        O->SetString(TEXT("place"), B.Place);
        return O;
    }

    FNpcScheduleBlock GtcRoutineObjectToBlock(const TSharedPtr<FGtcJsonObject>& O)
    {
        FNpcScheduleBlock B;
        if (O.IsValid())
        {
            B.Start = O->GetNumber(TEXT("start"), 0.0);
            B.End = O->GetNumber(TEXT("end"), 24.0);
            B.Activity = O->GetString(TEXT("activity"));
            B.Place = O->GetString(TEXT("place"));
        }
        return B;
    }

    TSharedRef<FGtcJsonObject> GtcRoutineToObject(const FNpcRoutine& R)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetString(TEXT("id"), R.Id);
        O->SetString(TEXT("name"), R.DisplayName);
        TArray<FGtcJsonValuePtr> Blocks;
        Blocks.Reserve(R.Blocks.Num());
        for (const FNpcScheduleBlock& B : R.Blocks)
        {
            Blocks.Add(FGtcJsonValue::MakeObject(GtcRoutineBlockToObject(B)));
        }
        O->SetArray(TEXT("blocks"), Blocks);
        return O;
    }

    FNpcRoutine GtcRoutineFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FNpcRoutine R;
        if (O.IsValid())
        {
            R.Id = O->GetString(TEXT("id"));
            R.DisplayName = O->GetString(TEXT("name"));
            for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("blocks")))
            {
                if (V.IsValid() && V->IsObject())
                {
                    R.Blocks.Add(GtcRoutineObjectToBlock(V->AsObject()));
                }
            }
        }
        return R;
    }
}

FString NpcRoutineJson::DefaultPathFor(const FString& RoutineId)
{
    const FString Safe = RoutineId.IsEmpty() ? TEXT("routine") : RoutineId;
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Routines"), Safe + TEXT(".routine.json"));
}

FString NpcRoutineJson::DefaultBankPath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Routines"), TEXT("routines.bank.json"));
}

FString NpcRoutineJson::ConfigBankPath()
{
    return FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Routines"), TEXT("routines.bank.json"));
}

FString NpcRoutineJson::RoutineToJson(const FNpcRoutine& Routine)
{
    TSharedRef<FGtcJsonObject> Root = MakeShared<FGtcJsonObject>();
    Root->SetString(TEXT("format"), TEXT("gtc_routine"));
    Root->SetNumber(TEXT("version"), FormatVersion);
    Root->SetObject(TEXT("routine"), GtcRoutineToObject(Routine));
    return GtcJson::Serialize(Root);
}

bool NpcRoutineJson::JsonToRoutine(const FString& Text, FNpcRoutine& Out)
{
    Out = FNpcRoutine();
    const FGtcJsonValuePtr V = GtcJson::Parse(Text);
    if (!V.IsValid() || !V->IsObject())
    {
        return false;
    }
    const TSharedPtr<FGtcJsonObject> Root = V->AsObject();
    if (Root->GetString(TEXT("format")) != TEXT("gtc_routine"))
    {
        return false;
    }
    const TSharedPtr<FGtcJsonObject> RObj = Root->GetObject(TEXT("routine"));
    if (!RObj.IsValid())
    {
        return false;
    }
    Out = GtcRoutineFromObject(RObj);
    return Out.IsValid();
}

bool NpcRoutineJson::SaveRoutineToFile(const FNpcRoutine& Routine, const FString& AbsolutePath)
{
    const FString Dir = FPaths::GetPath(AbsolutePath);
    if (!Dir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);
    }
    return FFileHelper::SaveStringToFile(
        RoutineToJson(Routine), *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool NpcRoutineJson::LoadRoutineFromFile(const FString& AbsolutePath, FNpcRoutine& Out)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
    {
        Out = FNpcRoutine();
        return false;
    }
    return JsonToRoutine(Text, Out);
}

FString NpcRoutineJson::BankToJson(const TArray<FNpcRoutine>& Routines)
{
    TSharedRef<FGtcJsonObject> Root = MakeShared<FGtcJsonObject>();
    Root->SetString(TEXT("format"), TEXT("gtc_routine_bank"));
    Root->SetNumber(TEXT("version"), FormatVersion);
    TArray<FGtcJsonValuePtr> Arr;
    Arr.Reserve(Routines.Num());
    for (const FNpcRoutine& R : Routines)
    {
        Arr.Add(FGtcJsonValue::MakeObject(GtcRoutineToObject(R)));
    }
    Root->SetArray(TEXT("routines"), Arr);
    return GtcJson::Serialize(Root);
}

bool NpcRoutineJson::JsonToBank(const FString& Text, TArray<FNpcRoutine>& Out)
{
    Out.Reset();
    const FGtcJsonValuePtr V = GtcJson::Parse(Text);
    if (!V.IsValid() || !V->IsObject())
    {
        return false;
    }
    const TSharedPtr<FGtcJsonObject> Root = V->AsObject();
    if (Root->GetString(TEXT("format")) != TEXT("gtc_routine_bank"))
    {
        return false;
    }
    for (const FGtcJsonValuePtr& Entry : Root->GetArray(TEXT("routines")))
    {
        if (Entry.IsValid() && Entry->IsObject())
        {
            FNpcRoutine R = GtcRoutineFromObject(Entry->AsObject());
            if (R.IsValid()) // skip junk entries, don't fail the whole bank
            {
                Out.Add(MoveTemp(R));
            }
        }
    }
    return true;
}

bool NpcRoutineJson::SaveBankToFile(const TArray<FNpcRoutine>& Routines, const FString& AbsolutePath)
{
    const FString Dir = FPaths::GetPath(AbsolutePath);
    if (!Dir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);
    }
    return FFileHelper::SaveStringToFile(
        BankToJson(Routines), *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool NpcRoutineJson::LoadBankFromFile(const FString& AbsolutePath, TArray<FNpcRoutine>& Out)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
    {
        Out.Reset();
        return false;
    }
    return JsonToBank(Text, Out);
}
