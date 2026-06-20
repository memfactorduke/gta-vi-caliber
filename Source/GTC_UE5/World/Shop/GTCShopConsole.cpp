// Copyright Epic Games, Inc. All Rights Reserved.

// GTC.Shop.* developer console commands — exercise the whole buy / respray / upgrade ->
// wallet path against the real player without placing an actor, so the economy loop is
// verifiable headlessly. Mirrors the GtcMissionEditorSubsystem console idiom
// (translation-unit-static FAutoConsoleCommandWithWorldAndArgs, every outcome UE_LOG'd
// under LogGtcShop).

#include "GTCShopBridge.h"
#include "StorefrontTransaction.h"

#include "../../Player/Stats/PlayerStats.h"
#include "../../Systems/Economy/ShopModel.h"
#include "../../Systems/PaySprayLoot/PaySpray.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../Vehicles/Ownership/VehicleModShop.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorldAndArgs GShopWalletCmd(
    TEXT("GTC.Shop.Wallet"),
    TEXT("GTC.Shop.Wallet — print the player's current money"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
    {
        if (UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(World))
        {
            UE_LOG(LogGtcShop, Display, TEXT("Wallet: %d"), Stats->GetMoney());
        }
        else
        {
            UE_LOG(LogGtcShop, Warning, TEXT("No player wallet."));
        }
    }));

static FAutoConsoleCommandWithWorldAndArgs GShopBuyCmd(
    TEXT("GTC.Shop.Buy"),
    TEXT("GTC.Shop.Buy <itemId> — buy from the default catalogue (pistol/smg/rifle/ammo_box/body_armor/sedan/sportscar)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(World);
        if (Stats == nullptr)
        {
            UE_LOG(LogGtcShop, Warning, TEXT("No player wallet."));
            return;
        }
        const FString Id = Args.IsValidIndex(0) ? Args[0] : TEXT("pistol");

        const TArray<FShopCatalogueItem> Catalogue = ShopModel::DefaultCatalogue();
        const FShopCatalogueItem* Item =
            Catalogue.FindByPredicate([&Id](const FShopCatalogueItem& E) { return E.Id == Id; });
        if (Item == nullptr)
        {
            UE_LOG(LogGtcShop, Warning, TEXT("Unknown item '%s'."), *Id);
            return;
        }

        const ShopModel Shop(Catalogue);
        const FStorefrontBuyDecision D = FStorefrontTransaction::ResolveBuy(Shop, *Item, Stats->GetMoney());
        if (!D.bSuccess)
        {
            UE_LOG(LogGtcShop, Display, TEXT("Buy '%s' denied — %s"), *Id, *D.Reason);
            return;
        }
        Stats->SpendMoney(D.Cost);
        if (D.Grant == EStorefrontGrantKind::Armor)
        {
            Stats->AddArmor(Stats->GetMaxArmor());
        }
        UE_LOG(LogGtcShop, Display, TEXT("Bought '%s' (%s) for %d; balance %d."),
            *D.ItemName, FStorefrontTransaction::GrantKindName(D.Grant), D.Cost, Stats->GetMoney());
    }));

static FAutoConsoleCommandWithWorldAndArgs GShopResprayCmd(
    TEXT("GTC.Shop.Respray"),
    TEXT("GTC.Shop.Respray [base] [perStar] — pay-n-spray clear of the current wanted level"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(World);
        if (Stats == nullptr)
        {
            UE_LOG(LogGtcShop, Warning, TEXT("No player wallet."));
            return;
        }
        UWantedSubsystem* Wanted = GTCShop::FindWanted(World);
        const int32 Stars = Wanted ? Wanted->Stars() : 0;
        const int32 Base = Args.IsValidIndex(0) ? FCString::Atoi(*Args[0]) : 0;
        const int32 PerStar = Args.IsValidIndex(1) ? FCString::Atoi(*Args[1]) : PaySpray::DefaultPerStar;

        PaySpray Spray;
        const PaySpray::FResolveResult R = Spray.Resolve(Stars, Stats->GetMoney(), /*bSeen=*/false, Base, PerStar);
        if (!R.bAllowed)
        {
            UE_LOG(LogGtcShop, Display, TEXT("Respray denied — %s"), *R.Reason);
            return;
        }
        if (R.Cost > 0)
        {
            Stats->SpendMoney(R.Cost);
        }
        if (Wanted != nullptr)
        {
            Wanted->Clear();
        }
        UE_LOG(LogGtcShop, Display, TEXT("Resprayed (was %d star) for %d; balance %d."), Stars, R.Cost, Stats->GetMoney());
    }));

static FAutoConsoleCommandWithWorldAndArgs GShopUpgradeCmd(
    TEXT("GTC.Shop.Upgrade"),
    TEXT("GTC.Shop.Upgrade <category> — buy the next vehicle-mod tier (engine/brakes/armor/tires). "
         "Stateless: each call works a fresh stock vehicle (level 0 -> 1)."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UPlayerStatsComponent* Stats = GTCShop::FindPlayerStats(World);
        if (Stats == nullptr)
        {
            UE_LOG(LogGtcShop, Warning, TEXT("No player wallet."));
            return;
        }
        const FString Category = Args.IsValidIndex(0) ? Args[0] : TEXT("engine");

        VehicleModShop ModShop;
        const VehicleModShop::FModUpgradeResult R = ModShop.Upgrade(Category, Stats->GetMoney());
        if (!R.bSuccess)
        {
            UE_LOG(LogGtcShop, Display, TEXT("Upgrade '%s' denied — %s"), *Category, *R.Reason);
            return;
        }
        Stats->SpendMoney(R.Cost);
        UE_LOG(LogGtcShop, Display, TEXT("Upgraded '%s' to level %d for %d; speed x%.2f; balance %d."),
            *Category, R.NewLevel, R.Cost, ModShop.TopSpeedMultiplier(), Stats->GetMoney());
    }));
