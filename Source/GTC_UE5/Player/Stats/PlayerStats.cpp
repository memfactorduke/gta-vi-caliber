// Copyright (c) 2026 GTC contributors

#include "PlayerStats.h"

#include "Net/UnrealNetwork.h"

// ============================================================================
// FPlayerStats — pure data / logic (1:1 with player_stats.gd)
// ============================================================================

void FPlayerStats::InitDefaults()
{
    // Godot _ready(): seed the wallet, then seed the objective only if none set.
    Money = StartingMoney;
    if (ObjectiveTitle.IsEmpty() && !StartingObjective.IsEmpty())
    {
        SetObjective(StartingObjective, FVector::ZeroVector, false);
    }
}

double FPlayerStats::SoakDamage(double Amount, bool& bOutArmorChanged)
{
    bOutArmorChanged = false;
    if (Amount <= 0.0)
    {
        // maxf(amount, 0.0): non-positive damage soaks nothing, overflow 0.
        return FMath::Max(Amount, 0.0);
    }
    double RemainingArmor = 0.0;
    double Overflow = 0.0;
    Absorb(Armor, Amount, RemainingArmor, Overflow);
    if (RemainingArmor != Armor)
    {
        Armor = RemainingArmor;
        bOutArmorChanged = true;
    }
    return Overflow;
}

double FPlayerStats::SoakDamage(double Amount)
{
    bool bUnused = false;
    return SoakDamage(Amount, bUnused);
}

void FPlayerStats::AddArmor(double Amount)
{
    // Negatives ignored (pickups only ADD); damage drains via SoakDamage.
    Armor = FMath::Clamp(Armor + FMath::Max(Amount, 0.0), 0.0, MaxArmor);
}

void FPlayerStats::AddMoney(int32 Amount)
{
    // Floor at zero so a large negative adjustment can't drive the wallet negative.
    Money = FMath::Max(0, Money + Amount);
}

bool FPlayerStats::SpendMoney(int32 Amount)
{
    if (Amount <= 0 || Money < Amount)
    {
        return false;
    }
    Money -= Amount;
    return true;
}

void FPlayerStats::SetObjective(const FString& Title, const FVector& Waypoint, bool bInHasWaypoint)
{
    ObjectiveTitle = Title;
    ObjectiveWaypoint = Waypoint;
    bHasWaypoint = bInHasWaypoint;
}

void FPlayerStats::ClearObjective()
{
    SetObjective(FString(), FVector::ZeroVector, false);
}

void FPlayerStats::Serialize(int32& OutMoney, double& OutArmor) const
{
    OutMoney = Money;
    OutArmor = Armor;
}

void FPlayerStats::Restore(bool bMoneyValid, double InMoney, bool bArmorValid, double InArmor)
{
    // SaveData.number_or: numeric -> use it (clamped), otherwise keep current.
    // money = maxi(int(number_or(money, money)), 0)
    if (bMoneyValid)
    {
        Money = FMath::Max(static_cast<int32>(InMoney), 0);
    }
    else
    {
        Money = FMath::Max(Money, 0);
    }
    // armor = clampf(number_or(armor, armor), 0.0, max_armor)
    const double ArmorSource = bArmorValid ? InArmor : Armor;
    Armor = FMath::Clamp(ArmorSource, 0.0, MaxArmor);
}

void FPlayerStats::Absorb(double ArmorValue, double Damage, double& OutRemainingArmor, double& OutOverflow)
{
    const double Soaked = FMath::Min(FMath::Max(ArmorValue, 0.0), FMath::Max(Damage, 0.0));
    OutRemainingArmor = ArmorValue - Soaked;
    OutOverflow = FMath::Max(Damage, 0.0) - Soaked;
}

double FPlayerStats::Fraction(double Value, double Maximum)
{
    if (Maximum <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(Value / Maximum, 0.0, 1.0);
}

// ============================================================================
// UPlayerStatsComponent — thin owner + lifecycle + delegate broadcast
// ============================================================================

UPlayerStatsComponent::UPlayerStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // Replicated stats store (see NEEDS_DECISION in the header re: rep model).
    SetIsReplicatedByDefault(true);
}

void UPlayerStatsComponent::BeginPlay()
{
    Super::BeginPlay();
    // Godot _ready() analogue: seed wallet + starting objective, then announce.
    Stats.InitDefaults();
    BroadcastMoney();
    BroadcastObjective();
}

float UPlayerStatsComponent::SoakDamage(float Amount)
{
    bool bArmorChanged = false;
    const double Overflow = Stats.SoakDamage(static_cast<double>(Amount), bArmorChanged);
    if (bArmorChanged)
    {
        BroadcastArmor();
    }
    return static_cast<float>(Overflow);
}

void UPlayerStatsComponent::AddArmor(float Amount)
{
    Stats.AddArmor(static_cast<double>(Amount));
    BroadcastArmor();
}

void UPlayerStatsComponent::AddMoney(int32 Amount)
{
    Stats.AddMoney(Amount);
    BroadcastMoney();
}

bool UPlayerStatsComponent::SpendMoney(int32 Amount)
{
    const bool bSpent = Stats.SpendMoney(Amount);
    if (bSpent)
    {
        BroadcastMoney();
    }
    return bSpent;
}

void UPlayerStatsComponent::SetObjective(const FString& Title, FVector Waypoint, bool bInHasWaypoint)
{
    Stats.SetObjective(Title, Waypoint, bInHasWaypoint);
    BroadcastObjective();
}

void UPlayerStatsComponent::ClearObjective()
{
    Stats.ClearObjective();
    BroadcastObjective();
}

void UPlayerStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Armor (the SOLE armor pool) + Money replicate to the owning client only.
    DOREPLIFETIME_CONDITION(UPlayerStatsComponent, RepArmor, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UPlayerStatsComponent, RepMoney, COND_OwnerOnly);
}

void UPlayerStatsComponent::OnRep_Armor()
{
    Stats.Armor = static_cast<double>(RepArmor);
    OnArmorChanged.Broadcast(static_cast<float>(Stats.Armor), static_cast<float>(Stats.MaxArmor));
}

void UPlayerStatsComponent::OnRep_Money()
{
    Stats.Money = RepMoney;
    OnMoneyChanged.Broadcast(Stats.Money);
}

void UPlayerStatsComponent::BroadcastArmor()
{
    // Keep the replicated mirror in lock-step with the authoritative data.
    RepArmor = static_cast<float>(Stats.Armor);
    OnArmorChanged.Broadcast(static_cast<float>(Stats.Armor), static_cast<float>(Stats.MaxArmor));
}

void UPlayerStatsComponent::BroadcastMoney()
{
    RepMoney = Stats.Money;
    OnMoneyChanged.Broadcast(Stats.Money);
}

void UPlayerStatsComponent::BroadcastObjective()
{
    OnObjectiveChanged.Broadcast(Stats.ObjectiveTitle, Stats.bHasWaypoint);
}
