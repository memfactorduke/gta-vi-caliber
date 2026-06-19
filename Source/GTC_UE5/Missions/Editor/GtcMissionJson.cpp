// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionJson.h"

#include "GtcMissionDefinition.h"
#include "../../Systems/Save/SaveJson.h"
#include "../../Systems/Save/SaveData.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// All keys are lowercase snake_case to match the existing save JSON (Godot-derived) and
// to read cleanly when a designer opens the file by hand.

namespace
{
    // --- Reflection-driven enum <-> string (clean, hand-editable, round-trip-safe) ---
    // Writes the SHORT enumerator name ("Reach", "SideQuest") so the .mission.json reads
    // naturally and is easy to hand-edit. Reading accepts the short name (case-insensitive)
    // OR the fully-qualified "EType::Entry" form; an unknown name falls back to the default,
    // so files are forward/backward-compatible.
    FString ShortEnumName(const FString& Full)
    {
        int32 Colon = INDEX_NONE;
        return Full.FindLastChar(TEXT(':'), Colon) ? Full.Mid(Colon + 1) : Full;
    }

    template <typename TEnum>
    FString EnumToStr(TEnum Value)
    {
        const UEnum* E = StaticEnum<TEnum>();
        return E ? ShortEnumName(E->GetNameStringByValue(static_cast<int64>(Value))) : FString();
    }

    template <typename TEnum>
    TEnum EnumFromStr(const FString& Name, TEnum Fallback)
    {
        const UEnum* E = StaticEnum<TEnum>();
        if (!E)
        {
            return Fallback;
        }
        // Accept the fully-qualified form directly.
        const int64 Direct = E->GetValueByNameString(Name);
        if (Direct != INDEX_NONE)
        {
            return static_cast<TEnum>(Direct);
        }
        // Otherwise match the short enumerator name (case-insensitive).
        for (int32 i = 0; i < E->NumEnums(); ++i)
        {
            if (ShortEnumName(E->GetNameStringByIndex(i)).Equals(Name, ESearchCase::IgnoreCase))
            {
                return static_cast<TEnum>(E->GetValueByIndex(i));
            }
        }
        return Fallback;
    }

    // FGtcJsonObject has no GetBool convenience getter — read the raw value.
    bool GetBoolField(const FGtcJsonObject& O, const TCHAR* Key, bool Fallback)
    {
        const FGtcJsonValuePtr V = O.Get(Key);
        return (V.IsValid() && V->GetType() == EGtcJsonType::Bool) ? V->AsBool() : Fallback;
    }

    FString GuidStr(const FGuid& G)
    {
        return G.ToString(EGuidFormats::DigitsWithHyphens);
    }

    // --- Sub-struct mappers ---------------------------------------------------------

    TSharedRef<FGtcJsonObject> ObjectiveToObject(const FGtcObjectiveDefinition& Obj)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetString(TEXT("id"), Obj.Id);
        O->SetString(TEXT("text"), Obj.Text.ToString());
        O->SetString(TEXT("kind"), EnumToStr(Obj.Kind));
        O->SetArray(TEXT("waypoint"), GtcSaveData::Vec3ToArray(Obj.Waypoint));
        O->SetBool(TEXT("has_waypoint"), Obj.bHasWaypoint);
        O->SetNumber(TEXT("radius"), Obj.Radius);
        O->SetNumber(TEXT("target_count"), Obj.TargetCount);
        O->SetNumber(TEXT("duration"), Obj.Duration);
        O->SetNumber(TEXT("threshold"), Obj.Threshold);
        O->SetString(TEXT("driver_kind"), Obj.DriverKind);
        O->SetBool(TEXT("optional"), Obj.bOptional);
        return O;
    }

    FGtcObjectiveDefinition ObjectiveFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FGtcObjectiveDefinition Obj;
        if (!O.IsValid())
        {
            return Obj;
        }
        Obj.Id = O->GetString(TEXT("id"));
        Obj.Text = FText::FromString(O->GetString(TEXT("text")));
        Obj.Kind = EnumFromStr(O->GetString(TEXT("kind")), EGtcObjectiveKind::Reach);
        Obj.Waypoint = GtcSaveData::ArrayToVec3(O->Get(TEXT("waypoint")), FVector::ZeroVector);
        Obj.bHasWaypoint = GetBoolField(*O, TEXT("has_waypoint"), true);
        Obj.Radius = O->GetNumber(TEXT("radius"), 600.0);
        Obj.TargetCount = static_cast<int32>(O->GetNumber(TEXT("target_count"), 1.0));
        Obj.Duration = O->GetNumber(TEXT("duration"), 3.0);
        Obj.Threshold = O->GetNumber(TEXT("threshold"), 0.0);
        Obj.DriverKind = O->GetString(TEXT("driver_kind"), TEXT("reach"));
        Obj.bOptional = GetBoolField(*O, TEXT("optional"), false);
        return Obj;
    }

    TSharedRef<FGtcJsonObject> TriggerToObject(const FGtcTriggerDefinition& T)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetString(TEXT("objective_id"), T.ObjectiveId);
        O->SetString(TEXT("type"), EnumToStr(T.Type));
        O->SetArray(TEXT("location"), GtcSaveData::Vec3ToArray(T.Location));
        O->SetNumber(TEXT("radius"), T.Radius);
        O->SetNumber(TEXT("count"), T.Count);
        O->SetNumber(TEXT("time"), T.Time);
        O->SetString(TEXT("target_marker"), GuidStr(T.TargetMarkerId));
        return O;
    }

    FGtcTriggerDefinition TriggerFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FGtcTriggerDefinition T;
        if (!O.IsValid())
        {
            return T;
        }
        T.ObjectiveId = O->GetString(TEXT("objective_id"));
        T.Type = EnumFromStr(O->GetString(TEXT("type")), EGtcTriggerType::EnterRadius);
        T.Location = GtcSaveData::ArrayToVec3(O->Get(TEXT("location")), FVector::ZeroVector);
        T.Radius = O->GetNumber(TEXT("radius"), 300.0);
        T.Count = static_cast<int32>(O->GetNumber(TEXT("count"), 1.0));
        T.Time = O->GetNumber(TEXT("time"), 0.0);
        FGuid::Parse(O->GetString(TEXT("target_marker")), T.TargetMarkerId);
        return T;
    }

    TSharedRef<FGtcJsonObject> RewardToObject(const FGtcRewardDefinition& R)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetNumber(TEXT("money"), R.Money);
        O->SetNumber(TEXT("objective_reward"), R.ObjectiveReward);
        O->SetNumber(TEXT("mission_bonus"), R.MissionBonus);
        O->SetNumber(TEXT("xp"), R.Xp);
        O->SetNumber(TEXT("respect"), R.Respect);
        TArray<FGtcJsonValuePtr> Unlocks;
        for (const FString& U : R.Unlocks)
        {
            Unlocks.Add(FGtcJsonValue::MakeString(U));
        }
        O->SetArray(TEXT("unlocks"), Unlocks);
        return O;
    }

    FGtcRewardDefinition RewardFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FGtcRewardDefinition R;
        if (!O.IsValid())
        {
            return R;
        }
        R.Money = static_cast<int32>(O->GetNumber(TEXT("money"), 0.0));
        R.ObjectiveReward = static_cast<int32>(O->GetNumber(TEXT("objective_reward"), 250.0));
        R.MissionBonus = static_cast<int32>(O->GetNumber(TEXT("mission_bonus"), 1000.0));
        R.Xp = static_cast<int32>(O->GetNumber(TEXT("xp"), 0.0));
        R.Respect = static_cast<int32>(O->GetNumber(TEXT("respect"), 0.0));
        for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("unlocks")))
        {
            if (V.IsValid() && V->GetType() == EGtcJsonType::String)
            {
                R.Unlocks.Add(V->AsString());
            }
        }
        return R;
    }

    TSharedRef<FGtcJsonObject> SideJobToObject(const FGtcSideJobDefinition& S)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        O->SetNumber(TEXT("kind"), S.Kind);
        O->SetArray(TEXT("pickup"), GtcSaveData::Vec3ToArray(S.Pickup));
        O->SetArray(TEXT("dropoff"), GtcSaveData::Vec3ToArray(S.Dropoff));
        O->SetNumber(TEXT("base_reward"), S.BaseReward);
        O->SetNumber(TEXT("par_time"), S.ParTime);
        return O;
    }

    FGtcSideJobDefinition SideJobFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FGtcSideJobDefinition S;
        if (!O.IsValid())
        {
            return S;
        }
        S.Kind = static_cast<int32>(O->GetNumber(TEXT("kind"), 0.0));
        S.Pickup = GtcSaveData::ArrayToVec3(O->Get(TEXT("pickup")), FVector::ZeroVector);
        S.Dropoff = GtcSaveData::ArrayToVec3(O->Get(TEXT("dropoff")), FVector::ZeroVector);
        S.BaseReward = static_cast<int32>(O->GetNumber(TEXT("base_reward"), 0.0));
        S.ParTime = O->GetNumber(TEXT("par_time"), 0.0);
        return S;
    }

    TSharedRef<FGtcJsonObject> RaceToObject(const FGtcStreetRaceDefinition& R)
    {
        TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
        TArray<FGtcJsonValuePtr> Cps;
        for (const FVector& Cp : R.Checkpoints)
        {
            Cps.Add(FGtcJsonValue::MakeArray(GtcSaveData::Vec3ToArray(Cp)));
        }
        O->SetArray(TEXT("checkpoints"), Cps);
        O->SetNumber(TEXT("laps"), R.Laps);
        O->SetNumber(TEXT("base_reward"), R.BaseReward);
        return O;
    }

    FGtcStreetRaceDefinition RaceFromObject(const TSharedPtr<FGtcJsonObject>& O)
    {
        FGtcStreetRaceDefinition R;
        if (!O.IsValid())
        {
            return R;
        }
        for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("checkpoints")))
        {
            R.Checkpoints.Add(GtcSaveData::ArrayToVec3(V, FVector::ZeroVector));
        }
        R.Laps = static_cast<int32>(O->GetNumber(TEXT("laps"), 1.0));
        R.BaseReward = static_cast<int32>(O->GetNumber(TEXT("base_reward"), 0.0));
        return R;
    }
}

// --- Public API ---------------------------------------------------------------------

TSharedRef<FGtcJsonObject> GtcMissionJson::MissionToObject(const FGtcMissionDefinition& D)
{
    TSharedRef<FGtcJsonObject> O = MakeShared<FGtcJsonObject>();
    O->SetString(TEXT("mission_id"), GuidStr(D.MissionId));
    O->SetString(TEXT("id"), D.Id);
    O->SetString(TEXT("title"), D.Title);
    O->SetString(TEXT("description"), D.Description.ToString());
    O->SetString(TEXT("content_type"), EnumToStr(D.ContentType));
    O->SetString(TEXT("giver"), D.Giver);
    O->SetString(TEXT("district"), D.District);
    O->SetNumber(TEXT("time_limit"), D.TimeLimit);
    O->SetBool(TEXT("repeatable"), D.bRepeatable);

    TArray<FGtcJsonValuePtr> Objs;
    for (const FGtcObjectiveDefinition& Obj : D.Objectives)
    {
        Objs.Add(FGtcJsonValue::MakeObject(ObjectiveToObject(Obj)));
    }
    O->SetArray(TEXT("objectives"), Objs);

    TArray<FGtcJsonValuePtr> Trigs;
    for (const FGtcTriggerDefinition& T : D.Triggers)
    {
        Trigs.Add(FGtcJsonValue::MakeObject(TriggerToObject(T)));
    }
    O->SetArray(TEXT("triggers"), Trigs);

    O->SetObject(TEXT("reward"), RewardToObject(D.Reward));

    O->SetString(TEXT("activity_kind"), EnumToStr(D.ActivityKind));
    O->SetObject(TEXT("side_job"), SideJobToObject(D.SideJob));
    O->SetObject(TEXT("race"), RaceToObject(D.Race));

    TArray<FGtcJsonValuePtr> Prereqs;
    for (const FGuid& G : D.Prerequisites)
    {
        Prereqs.Add(FGtcJsonValue::MakeString(GuidStr(G)));
    }
    O->SetArray(TEXT("prerequisites"), Prereqs);

    O->SetBool(TEXT("available_in_world"), D.bAvailableInWorld);
    O->SetArray(TEXT("start_location"), GtcSaveData::Vec3ToArray(D.StartLocation));
    O->SetNumber(TEXT("start_radius"), D.StartRadius);

    return O;
}

FGtcMissionDefinition GtcMissionJson::ObjectToMission(const TSharedPtr<FGtcJsonObject>& O)
{
    FGtcMissionDefinition D;
    if (!O.IsValid() || O->Num() == 0)
    {
        D.bValid = false;
        return D;
    }

    // Missing or malformed mission_id -> assign a fresh unique id so two such files can't
    // collide on the zero GUID (which would let one overwrite the other in the bank).
    if (!FGuid::Parse(O->GetString(TEXT("mission_id")), D.MissionId) || !D.MissionId.IsValid())
    {
        D.MissionId = FGuid::NewGuid();
    }
    D.Id = O->GetString(TEXT("id"));
    D.Title = O->GetString(TEXT("title"));
    D.Description = FText::FromString(O->GetString(TEXT("description")));
    D.ContentType = EnumFromStr(O->GetString(TEXT("content_type")), EGtcMissionContentType::MainMission);
    D.Giver = O->GetString(TEXT("giver"));
    D.District = O->GetString(TEXT("district"));
    D.TimeLimit = O->GetNumber(TEXT("time_limit"), 0.0);
    D.bRepeatable = GetBoolField(*O, TEXT("repeatable"), false);

    for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("objectives")))
    {
        if (V.IsValid() && V->IsObject())
        {
            D.Objectives.Add(ObjectiveFromObject(V->AsObject()));
        }
    }
    for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("triggers")))
    {
        if (V.IsValid() && V->IsObject())
        {
            D.Triggers.Add(TriggerFromObject(V->AsObject()));
        }
    }

    D.Reward = RewardFromObject(O->GetObject(TEXT("reward")));
    D.ActivityKind = EnumFromStr(O->GetString(TEXT("activity_kind")), EGtcActivityKind::None);
    D.SideJob = SideJobFromObject(O->GetObject(TEXT("side_job")));
    D.Race = RaceFromObject(O->GetObject(TEXT("race")));

    for (const FGtcJsonValuePtr& V : O->GetArray(TEXT("prerequisites")))
    {
        if (V.IsValid() && V->GetType() == EGtcJsonType::String)
        {
            FGuid G;
            if (FGuid::Parse(V->AsString(), G))
            {
                D.Prerequisites.Add(G);
            }
        }
    }

    D.bAvailableInWorld = GetBoolField(*O, TEXT("available_in_world"), false);
    D.StartLocation = GtcSaveData::ArrayToVec3(O->Get(TEXT("start_location")), FVector::ZeroVector);
    D.StartRadius = O->GetNumber(TEXT("start_radius"), 400.0);

    D.bValid = true;
    return D;
}

void GtcMissionJson::WriteBank(const TArray<FGtcMissionDefinition>& Bank, const TSharedRef<FGtcJsonObject>& SectionOut)
{
    TArray<FGtcJsonValuePtr> Arr;
    Arr.Reserve(Bank.Num());
    for (const FGtcMissionDefinition& M : Bank)
    {
        Arr.Add(FGtcJsonValue::MakeObject(MissionToObject(M)));
    }
    SectionOut->SetArray(TEXT("missions"), Arr);
}

void GtcMissionJson::ReadBank(const TSharedRef<FGtcJsonObject>& SectionIn, TArray<FGtcMissionDefinition>& OutBank)
{
    OutBank.Reset();
    for (const FGtcJsonValuePtr& V : SectionIn->GetArray(TEXT("missions")))
    {
        if (V.IsValid() && V->IsObject())
        {
            FGtcMissionDefinition D = ObjectToMission(V->AsObject());
            if (D.bValid)
            {
                OutBank.Add(MoveTemp(D));
            }
        }
    }
}

void GtcMissionJson::WriteGuidSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, const TSet<FGuid>& Ids)
{
    TArray<FGtcJsonValuePtr> Arr;
    Arr.Reserve(Ids.Num());
    for (const FGuid& G : Ids)
    {
        Arr.Add(FGtcJsonValue::MakeString(GuidStr(G)));
    }
    Section->SetArray(Key, Arr);
}

void GtcMissionJson::ReadGuidSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, TSet<FGuid>& OutIds)
{
    OutIds.Reset();
    for (const FGtcJsonValuePtr& V : Section->GetArray(Key))
    {
        if (V.IsValid() && V->GetType() == EGtcJsonType::String)
        {
            FGuid G;
            if (FGuid::Parse(V->AsString(), G))
            {
                OutIds.Add(G);
            }
        }
    }
}

void GtcMissionJson::WriteStringSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, const TSet<FString>& Strings)
{
    TArray<FGtcJsonValuePtr> Arr;
    Arr.Reserve(Strings.Num());
    for (const FString& S : Strings)
    {
        Arr.Add(FGtcJsonValue::MakeString(S));
    }
    Section->SetArray(Key, Arr);
}

void GtcMissionJson::ReadStringSet(const TSharedRef<FGtcJsonObject>& Section, const FString& Key, TSet<FString>& OutStrings)
{
    OutStrings.Reset();
    for (const FGtcJsonValuePtr& V : Section->GetArray(Key))
    {
        if (V.IsValid() && V->GetType() == EGtcJsonType::String)
        {
            OutStrings.Add(V->AsString());
        }
    }
}

FString GtcMissionJson::MissionToJson(const FGtcMissionDefinition& D)
{
    TSharedRef<FGtcJsonObject> Root = MakeShared<FGtcJsonObject>();
    Root->SetString(TEXT("format"), TEXT("gtc_mission"));
    Root->SetNumber(TEXT("version"), GtcMissionJson::FormatVersion);
    Root->SetObject(TEXT("mission"), MissionToObject(D));
    return GtcJson::Serialize(Root);
}

bool GtcMissionJson::SaveMissionToFile(const FGtcMissionDefinition& Def, const FString& AbsolutePath)
{
    const FString Dir = FPaths::GetPath(AbsolutePath);
    if (!Dir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);
    }
    return FFileHelper::SaveStringToFile(MissionToJson(Def), *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool GtcMissionJson::LoadMissionFromFile(const FString& AbsolutePath, FGtcMissionDefinition& Out)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *AbsolutePath))
    {
        Out = FGtcMissionDefinition();
        Out.bValid = false;
        return false;
    }
    return JsonToMission(Text, Out);
}

bool GtcMissionJson::JsonToMission(const FString& Text, FGtcMissionDefinition& Out)
{
    const FGtcJsonValuePtr Val = GtcJson::Parse(Text);
    if (!Val.IsValid() || !Val->IsObject())
    {
        Out = FGtcMissionDefinition();
        Out.bValid = false;
        return false;
    }

    const TSharedPtr<FGtcJsonObject> Root = Val->AsObject();
    if (!Root.IsValid() || Root->GetString(TEXT("format")) != TEXT("gtc_mission"))
    {
        Out = FGtcMissionDefinition();
        Out.bValid = false;
        return false;
    }

    Out = ObjectToMission(Root->GetObject(TEXT("mission")));
    return Out.bValid;
}
