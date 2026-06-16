// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeistCrew.h"

HeistCrew::HeistCrew(int32 MaxMemberCount)
{
    MaxMembers = FMath::Max(MaxMemberCount, 0);
}

bool HeistCrew::AddMember(const FString& Role, double Skill, double Cut)
{
    if (Role.IsEmpty())
    {
        return false;
    }
    if (Members.Num() >= MaxMembers)
    {
        return false;
    }
    if (HasRole(Role))
    {
        return false;
    }
    const double SafeCut = FMath::Clamp(Cut, 0.0, 1.0);
    if (TotalCut() + SafeCut > 1.0 + 0.000001)
    {
        return false;
    }
    FMember Member;
    Member.Role = Role;
    Member.Skill = FMath::Clamp(Skill, 0.0, 1.0);
    Member.Cut = SafeCut;
    Members.Add(Member);
    return true;
}

bool HeistCrew::RemoveMember(const FString& Role)
{
    for (int32 I = 0; I < Members.Num(); ++I)
    {
        if (Members[I].Role == Role)
        {
            Members.RemoveAt(I);
            return true;
        }
    }
    return false;
}

int32 HeistCrew::MemberCount() const
{
    return Members.Num();
}

TArray<FString> HeistCrew::Roles() const
{
    TArray<FString> Out;
    Out.Reserve(Members.Num());
    for (const FMember& Member : Members)
    {
        Out.Add(Member.Role);
    }
    return Out;
}

double HeistCrew::TotalCut() const
{
    double Sum = 0.0;
    for (const FMember& Member : Members)
    {
        Sum += Member.Cut;
    }
    return Sum;
}

double HeistCrew::PlayerShare() const
{
    return FMath::Max(1.0 - TotalCut(), 0.0);
}

double HeistCrew::CrewSkill() const
{
    if (Members.Num() == 0)
    {
        return 0.0;
    }
    double Sum = 0.0;
    for (const FMember& Member : Members)
    {
        Sum += Member.Skill;
    }
    return Sum / static_cast<double>(Members.Num());
}

double HeistCrew::SuccessChance(double BaseDifficulty) const
{
    const double Base = FMath::Clamp(BaseDifficulty, 0.0, 1.0);
    const double Raw = Base - SkillFloorPenalty + CrewSkill() * SkillSwing;
    return FMath::Clamp(Raw, 0.0, 1.0);
}

bool HeistCrew::Attempt(double BaseDifficulty, FRandomStream& Rng) const
{
    return Rng.GetFraction() < SuccessChance(BaseDifficulty);
}

bool HeistCrew::AttemptNoRng(double /*BaseDifficulty*/) const
{
    return false;
}

int32 HeistCrew::Payout(int32 Take, bool bSuccess) const
{
    if (!bSuccess || Take <= 0)
    {
        return 0;
    }
    return static_cast<int32>(FMath::Max(static_cast<double>(Take) * PlayerShare(), 0.0));
}

double HeistCrew::ExpectedPayout(int32 Take, double BaseDifficulty) const
{
    if (Take <= 0)
    {
        return 0.0;
    }
    return SuccessChance(BaseDifficulty) * static_cast<double>(Take) * PlayerShare();
}

FRandomStream HeistCrew::MakeRng(int32 Seed)
{
    return FRandomStream(Seed);
}

bool HeistCrew::HasRole(const FString& Role) const
{
    for (const FMember& Member : Members)
    {
        if (Member.Role == Role)
        {
            return true;
        }
    }
    return false;
}
