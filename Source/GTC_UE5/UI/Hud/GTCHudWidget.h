// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GTCHudWidget.generated.h"

class UPlayerStatsComponent;
class UPlayerHealthComponent;

/**
 * UGTCHudWidget — the C++ BASE for the GTC gameplay HUD (the visual WBP is built
 * later, editor-open, and reparents to this class).
 *
 * Responsibility split:
 *  - This C++ base SUBSCRIBES to the merged Wave 2 player-component change delegates
 *    (UPlayerStatsComponent::OnArmorChanged / OnMoneyChanged and
 *    UPlayerHealthComponent::OnHealthChanged), CACHES the current armor/health bar
 *    fractions and the wallet, and fires OnHudUpdated so the WBP can repaint. It does
 *    NOT own any visual widget: the meta=(BindWidget) ProgressBars / TextBlocks live
 *    in the future WBP. There are deliberately NO asserts / IsValid-requires on those
 *    widgets here — a missing bar must never crash the base.
 *  - The WBP (deferred to editor-open) holds the ProgressBars / TextBlocks bound by
 *    name and implements OnHudUpdated to push these cached values into them.
 *
 * Binding lifecycle: BindToComponents() is the single wiring entry point. It is
 * idempotent (unbinds any previous components first) so it is safe to re-call when the
 * possessed pawn / player-state changes. Bindings are torn down on NativeDestruct.
 *
 * DEFERRED-TO-EDITOR (WBP) — not part of this headless base:
 *  - the visual Widget Blueprint, its meta=(BindWidget) ProgressBars/TextBlocks, and
 *    the OnHudUpdated implementation that renders the cached values.
 *  - live in-PIE binding to the real possessed pawn's components (the base exposes
 *    BindToComponents for the pawn/controller to call; auto-discovery of the owning
 *    pawn's components is a PIE wiring step, not this base's concern).
 */
UCLASS()
class GTC_UE5_API UGTCHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Wire this HUD to a stats + health component (either may be null). Idempotent:
     * unbinds any previously-bound components first, then subscribes to the change
     * delegates and seeds the cached values from the components' current state so the
     * HUD reads correctly from frame one. Fires OnHudUpdated once after seeding.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|HUD")
    void BindToComponents(UPlayerStatsComponent* InStats, UPlayerHealthComponent* InHealth);

    /** Drop all delegate subscriptions and clear the cached component pointers. */
    UFUNCTION(BlueprintCallable, Category = "GTC|HUD")
    void UnbindComponents();

    // --- Cached bound values (the WBP renders these) ---------------------------

    /** Armor bar fill fraction in [0,1]. Updated from OnArmorChanged. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|HUD")
    float ArmorFraction = 0.0f;

    /** Health bar fill fraction in [0,1]. Updated from OnHealthChanged. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|HUD")
    float HealthFraction = 0.0f;

    /** Wallet balance. Updated from OnMoneyChanged. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|HUD")
    int32 Money = 0;

    /** Pre-formatted wallet string (e.g. "$1,500"), kept in sync with Money. */
    UPROPERTY(BlueprintReadOnly, Category = "GTC|HUD")
    FString MoneyText;

    // --- WBP render hook -------------------------------------------------------

    /**
     * Fired after any cached value changes (and once on bind). The WBP implements this
     * to push ArmorFraction / HealthFraction / MoneyText into its bound widgets. A pure
     * notification — the base has already updated the cached values before broadcasting.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GTC|HUD")
    void OnHudUpdated();

protected:
    virtual void NativeDestruct() override;

    // --- Delegate handlers (UFUNCTION so dynamic delegates can bind) -----------

    UFUNCTION()
    void HandleArmorChanged(float Armor, float MaxArmor);

    UFUNCTION()
    void HandleHealthChanged(float Health, float MaxHealth);

    UFUNCTION()
    void HandleMoneyChanged(int32 Amount);

private:
    /** The currently-bound stats component (weak: the pawn/state owns its lifetime). */
    UPROPERTY(Transient)
    TWeakObjectPtr<UPlayerStatsComponent> BoundStats;

    /** The currently-bound health component (weak: the pawn/state owns its lifetime). */
    UPROPERTY(Transient)
    TWeakObjectPtr<UPlayerHealthComponent> BoundHealth;

    /** Recompute MoneyText from Money and broadcast OnHudUpdated. */
    void NotifyHudUpdated();
};
