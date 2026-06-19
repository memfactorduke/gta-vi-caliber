// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveData.h"

namespace GtcSaveData
{
FString Encode(const TSharedRef<FGtcJsonObject>& Snapshot)
{
    const TSharedRef<FGtcJsonObject> Wrapper = MakeShared<FGtcJsonObject>();
    Wrapper->SetNumber(TEXT("version"), Version);
    Wrapper->SetObject(TEXT("data"), Snapshot);
    return GtcJson::Serialize(Wrapper);
}

TSharedRef<FGtcJsonObject> Decode(const FString& Text)
{
    const FGtcJsonValuePtr Root = GtcJson::Parse(Text);
    if (!Root.IsValid() || !Root->IsObject())
    {
        return MakeShared<FGtcJsonObject>();
    }
    const FGtcJsonValuePtr Data = Root->AsObject()->Get(TEXT("data"));
    if (!Data.IsValid() || !Data->IsObject())
    {
        return MakeShared<FGtcJsonObject>();
    }
    return Data->AsObject().ToSharedRef();
}

int32 VersionOf(const FString& Text)
{
    const FGtcJsonValuePtr Root = GtcJson::Parse(Text);
    if (!Root.IsValid() || !Root->IsObject())
    {
        return 0;
    }
    return static_cast<int32>(Root->AsObject()->GetNumber(TEXT("version"), 0.0));
}

TSharedRef<FGtcJsonObject> Migrate(const TSharedRef<FGtcJsonObject>& Snapshot, int32 FromVersion)
{
    // A failed/empty decode must stay empty: otherwise the v1->v2 section-fill below
    // turns {} into {stats:{}, ...} — NON-empty — which slips past the subsystem's
    // "empty == no saved data" guard and runs restore({}) on every tracker, silently
    // wiping stats/progression from a corrupt or empty old-version save.
    if (Snapshot->Num() == 0)
    {
        return MakeShared<FGtcJsonObject>();
    }

    const TSharedRef<FGtcJsonObject> Out = Snapshot->DeepClone();
    if (FromVersion < 2)
    {
        for (const TCHAR* Key : { TEXT("stats"), TEXT("progression"), TEXT("properties") })
        {
            if (!Out->HasObjectField(Key))
            {
                Out->SetObject(Key, MakeShared<FGtcJsonObject>());
            }
        }
    }
    if (FromVersion < 3 && !Out->HasObjectField(TEXT("lifetime_stats")))
    {
        Out->SetObject(TEXT("lifetime_stats"), MakeShared<FGtcJsonObject>());
    }
    if (FromVersion < 4 && !Out->HasObjectField(TEXT("player_skills")))
    {
        Out->SetObject(TEXT("player_skills"), MakeShared<FGtcJsonObject>());
    }
    return Out;
}

TArray<FGtcJsonValuePtr> Vec3ToArray(const FVector& V)
{
    TArray<FGtcJsonValuePtr> Arr;
    Arr.Add(FGtcJsonValue::MakeNumber(V.X));
    Arr.Add(FGtcJsonValue::MakeNumber(V.Y));
    Arr.Add(FGtcJsonValue::MakeNumber(V.Z));
    return Arr;
}

FVector ArrayToVec3(const FGtcJsonValuePtr& Value, const FVector& Fallback)
{
    if (!Value.IsValid() || !Value->IsArray())
    {
        return Fallback;
    }
    const TArray<FGtcJsonValuePtr>& Arr = Value->AsArray();
    if (Arr.Num() != 3)
    {
        return Fallback;
    }
    for (const FGtcJsonValuePtr& Item : Arr)
    {
        if (!Item.IsValid() || !Item->IsNumber())
        {
            return Fallback;
        }
    }
    return FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
}

TSharedRef<FGtcJsonObject> TransformToDict(const FTransform& T)
{
    // Godot stored origin + the three rotation basis columns. UE's rotation matrix axes are
    // the equivalent orthonormal columns; X/Y/Z scaled-axis vectors are those columns.
    const FMatrix M = T.ToMatrixNoScale();
    const TSharedRef<FGtcJsonObject> Obj = MakeShared<FGtcJsonObject>();
    Obj->SetArray(TEXT("origin"), Vec3ToArray(T.GetLocation()));
    Obj->SetArray(TEXT("basis_x"), Vec3ToArray(M.GetScaledAxis(EAxis::X)));
    Obj->SetArray(TEXT("basis_y"), Vec3ToArray(M.GetScaledAxis(EAxis::Y)));
    Obj->SetArray(TEXT("basis_z"), Vec3ToArray(M.GetScaledAxis(EAxis::Z)));
    return Obj;
}

FTransform DictToTransform(const FGtcJsonValuePtr& Value, const FTransform& Fallback)
{
    if (!Value.IsValid() || !Value->IsObject())
    {
        return Fallback;
    }
    const TSharedPtr<FGtcJsonObject> Obj = Value->AsObject();
    if (!Obj.IsValid())
    {
        return Fallback;
    }

    const FMatrix FbM = Fallback.ToMatrixNoScale();
    const FVector AxisX = ArrayToVec3(Obj->Get(TEXT("basis_x")), FbM.GetScaledAxis(EAxis::X));
    const FVector AxisY = ArrayToVec3(Obj->Get(TEXT("basis_y")), FbM.GetScaledAxis(EAxis::Y));
    const FVector AxisZ = ArrayToVec3(Obj->Get(TEXT("basis_z")), FbM.GetScaledAxis(EAxis::Z));
    const FVector Origin = ArrayToVec3(Obj->Get(TEXT("origin")), Fallback.GetLocation());

    FMatrix M(FMatrix::Identity);
    M.SetAxis(0, AxisX);
    M.SetAxis(1, AxisY);
    M.SetAxis(2, AxisZ);
    M.SetOrigin(Origin);
    return FTransform(M);
}

double NumberOr(const FGtcJsonValuePtr& Value, double Fallback)
{
    return (Value.IsValid() && Value->IsNumber()) ? Value->AsNumber() : Fallback;
}
}
