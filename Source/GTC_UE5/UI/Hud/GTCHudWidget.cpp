// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCHudWidget.h"

#include "GtcHudFormat.h"
#include "../../Player/Stats/PlayerStats.h"
#include "../../Player/Health/PlayerHealthComponent.h"

void UGTCHudWidget::BindToComponents(UPlayerStatsComponent* InStats, UPlayerHealthComponent* InHealth)
{
    // Idempotent: drop any prior subscriptions before re-wiring.
    UnbindComponents();

    BoundStats = InStats;
    BoundHealth = InHealth;

    if (InStats)
    {
        InStats->OnArmorChanged.AddDynamic(this, &UGTCHudWidget::HandleArmorChanged);
        InStats->OnMoneyChanged.AddDynamic(this, &UGTCHudWidget::HandleMoneyChanged);

        // Seed from current state so the HUD reads correctly from frame one.
        ArmorFraction = static_cast<float>(GtcHud::SafeFraction(InStats->GetArmor(), InStats->GetMaxArmor()));
        Money = InStats->GetMoney();
    }

    if (InHealth)
    {
        InHealth->OnHealthChanged.AddDynamic(this, &UGTCHudWidget::HandleHealthChanged);

        HealthFraction = static_cast<float>(GtcHud::ClampFraction(InHealth->GetHealthFraction()));
    }

    NotifyHudUpdated();
}

void UGTCHudWidget::UnbindComponents()
{
    if (UPlayerStatsComponent* Stats = BoundStats.Get())
    {
        Stats->OnArmorChanged.RemoveDynamic(this, &UGTCHudWidget::HandleArmorChanged);
        Stats->OnMoneyChanged.RemoveDynamic(this, &UGTCHudWidget::HandleMoneyChanged);
    }
    if (UPlayerHealthComponent* Health = BoundHealth.Get())
    {
        Health->OnHealthChanged.RemoveDynamic(this, &UGTCHudWidget::HandleHealthChanged);
    }

    BoundStats = nullptr;
    BoundHealth = nullptr;
}

void UGTCHudWidget::NativeDestruct()
{
    UnbindComponents();
    Super::NativeDestruct();
}

void UGTCHudWidget::HandleArmorChanged(float Armor, float MaxArmor)
{
    ArmorFraction = static_cast<float>(GtcHud::SafeFraction(Armor, MaxArmor));
    NotifyHudUpdated();
}

void UGTCHudWidget::HandleHealthChanged(float Health, float MaxHealth)
{
    HealthFraction = static_cast<float>(GtcHud::SafeFraction(Health, MaxHealth));
    NotifyHudUpdated();
}

void UGTCHudWidget::HandleMoneyChanged(int32 Amount)
{
    Money = Amount;
    NotifyHudUpdated();
}

void UGTCHudWidget::NotifyHudUpdated()
{
    MoneyText = GtcHud::FormatMoney(Money);
    // Fire the WBP render hook. BlueprintImplementableEvent is always safe to call even
    // when no Blueprint overrides it (this headless base has no implementation).
    OnHudUpdated();
}
