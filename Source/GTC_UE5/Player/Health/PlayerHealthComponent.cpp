// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerHealthComponent.h"

// ============================================================================
// UPlayerHealthComponent — thin owner + lifecycle + delegate broadcast
// ============================================================================

UPlayerHealthComponent::UPlayerHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // Health/armor store (see NEEDS_DECISION in the header re: rep model).
    SetIsReplicatedByDefault(true);
}

void UPlayerHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    // Godot _ready() analogue: announce the seeded full-health/zero-armor state.
    BroadcastHealth();
    BroadcastArmor();
}

bool UPlayerHealthComponent::ApplyDamage(float Amount)
{
    const double ArmorBefore = Model.Armor;
    const double HealthBefore = Model.Health;
    const bool bKilled = Model.Apply(static_cast<double>(Amount));
    if (Model.Armor != ArmorBefore)
    {
        BroadcastArmor();
    }
    if (Model.Health != HealthBefore)
    {
        BroadcastHealth();
    }
    if (bKilled)
    {
        OnDied.Broadcast();
    }
    return bKilled;
}

void UPlayerHealthComponent::AddArmor(float Amount)
{
    const double ArmorBefore = Model.Armor;
    Model.AddArmor(static_cast<double>(Amount));
    if (Model.Armor != ArmorBefore)
    {
        BroadcastArmor();
    }
}

void UPlayerHealthComponent::Heal(float Amount)
{
    const double HealthBefore = Model.Health;
    Model.Heal(static_cast<double>(Amount));
    if (Model.Health != HealthBefore)
    {
        BroadcastHealth();
    }
}

void UPlayerHealthComponent::TickHealth(float DeltaSeconds)
{
    const double HealthBefore = Model.Health;
    Model.Tick(static_cast<double>(DeltaSeconds));
    if (Model.Health != HealthBefore)
    {
        BroadcastHealth();
    }
}

void UPlayerHealthComponent::Revive()
{
    Model.Revive();
    BroadcastHealth();
    BroadcastArmor();
}

void UPlayerHealthComponent::BroadcastHealth()
{
    OnHealthChanged.Broadcast(static_cast<float>(Model.Health), static_cast<float>(Model.MaxHealth));
}

void UPlayerHealthComponent::BroadcastArmor()
{
    OnArmorChanged.Broadcast(static_cast<float>(Model.Armor), static_cast<float>(Model.MaxArmor));
}
