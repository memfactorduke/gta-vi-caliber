// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillsCoordinator.h"

void USkillsCoordinator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    EnsureModel();
}

void USkillsCoordinator::EnsureModel() const
{
    if (!_Skills.IsValid())
    {
        _Skills = MakeUnique<FPlayerSkills>();
    }
}

void USkillsCoordinator::Deinitialize()
{
    _Skills.Reset();
    Super::Deinitialize();
}

double USkillsCoordinator::TrainActivity(const FString& Id, double Effort)
{
    EnsureModel();
    return _Skills->Train(Id, Effort);
}

double USkillsCoordinator::Level(const FString& Id) const
{
    EnsureModel();
    return _Skills->Level(Id);
}

FString USkillsCoordinator::Tier(const FString& Id) const
{
    EnsureModel();
    return _Skills->Tier(Id);
}

double USkillsCoordinator::Bonus(const FString& Id) const
{
    EnsureModel();
    return _Skills->Bonus(Id);
}

double USkillsCoordinator::OverallMastery() const
{
    EnsureModel();
    return _Skills->OverallMastery();
}

double USkillsCoordinator::PlanarDistance(const FVector& A, const FVector& B)
{
    // Godot _planar_distance: Vector2(a.x, a.z).distance_to(Vector2(b.x, b.z)).
    const double Dx = A.X - B.X;
    const double Dz = A.Z - B.Z;
    return FMath::Sqrt(Dx * Dx + Dz * Dz);
}

double USkillsCoordinator::TrainFromOnFootMove(const FVector& From, const FVector& To, bool bVisible)
{
    const double Meters = PlanarDistance(From, To);
    if (bVisible && Meters > 0.01)
    {
        return TrainActivity(TEXT("stamina"), Meters * StaminaEffortPerMeter);
    }
    return 0.0;
}

double USkillsCoordinator::TrainFromVehicleMove(const FVector& From, const FVector& To)
{
    const double Meters = PlanarDistance(From, To);
    if (Meters > 0.01)
    {
        return TrainActivity(TEXT("driving"), Meters * DrivingEffortPerMeter);
    }
    return 0.0;
}

double USkillsCoordinator::OnHitConfirmed()
{
    return TrainActivity(TEXT("shooting"), ShootingHitEffort);
}

TArray<TPair<FString, double>> USkillsCoordinator::SerializeState() const
{
    EnsureModel();
    return _Skills->ToDict();
}

void USkillsCoordinator::Restore(const TArray<TPair<FString, double>>& Data)
{
    EnsureModel();
    _Skills->LoadDict(Data);
}
