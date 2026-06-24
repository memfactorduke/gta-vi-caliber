// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPoliceDirector.h"
#include "GTCPoliceOfficer.h"
#include "GTCPoliceHelicopter.h"
#include "GTCPoliceCar.h"
#include "GTCRoadblock.h"
#include "GTCSpikeStrip.h"
#include "GTCSwatVan.h"
#include "GTCK9.h"
#include "EngineUtils.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

#include "../PoliceDispatch/PoliceDispatch.h"
#include "../PoliceDispatch/PoliceEscalation.h"
#include "../Pursuit/HelicopterPursuit.h"
#include "../Pursuit/Roadblock.h"
#include "../PoliceDispatch/PoliceSpawnPlan.h"
#include "../../World/Police/PoliceResponse.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../Systems/Arrest/ArrestSubsystem.h"

namespace
{
    // Metres (the pure-core frame) -> Unreal centimetres.
    constexpr double MetresToCm = 100.0;

    // Core planar (Godot XZ, Y ignored) -> Unreal planar (XY, Z ignored).
    FORCEINLINE FVector FromCorePlanar(const FVector& C) { return FVector(C.X, C.Z, 0.0); }
}

AGTCPoliceDirector::AGTCPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGTCPoliceDirector::BeginPlay()
{
    Super::BeginPlay();
    Rng.Initialize(SpawnSeed);
}

void AGTCPoliceDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    UGameInstance* GI = GetGameInstance();

    // Refresh stars + nearest-LIVING-officer distance + the "seen" signal EVERY frame,
    // first, so the wanted/evasion/arrest drivers below all read this frame's picture
    // (never a stale snapshot or a corpse).
    RefreshCombatSnapshot();

    // Drive the wanted clock (heat decay + witness-report completion) and the evasion
    // escape loop; nothing else owns either. Guarded so the flags cede them cleanly.
    if (bDriveWantedClock && GI != nullptr)
    {
        if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            Wanted->TickFrame(DeltaSeconds);

            // Escape loop: feed evasion from police line-of-sight; once the player has
            // been out of sight for the search window they "go cold" and lose the stars.
            if (bDriveEvasion && Wanted->Stars() > 0)
            {
                Wanted->UpdateEvasion(bPlayerSeenByPolice, DeltaSeconds);
                if (Wanted->GetEvasion().IsCold())
                {
                    Wanted->Clear();
                }
            }
        }
    }

    // Stream police on a throttle (spawning/culling need not run every frame).
    StreamAccum += DeltaSeconds;
    if (StreamAccum >= StreamIntervalSec)
    {
        StreamAccum = 0.0;
        StreamPolice();
    }

    // Feed the bust loop every frame so the grapple timer is smooth. NOTE: the loop
    // is wired and unit-consistent (metres, living-officer distance, no false busts),
    // but officers hold at gunfight range (~12-20 m) and the catch range is 2 m, so a
    // bust rarely lands through combat AI alone. Closing it needs a dedicated
    // "apprehend" action (close to contact when the player is low-threat) — a
    // documented follow-up; the kill-the-cops loop is this slice's deliverable.
    if (bDriveArrest && GI != nullptr)
    {
        if (UArrestSubsystem* Arrest = GI->GetSubsystem<UArrestSubsystem>())
        {
            Arrest->Tick(CachedStars, CachedNearestCopDistM, DeltaSeconds);

            // Drain the bust's heat-clear intent — the deferred Wave-3 coupling the
            // arrest subsystem exposes. A landed bust clears the wanted level (the
            // third outcome, alongside escape and death); OnBusted is left for BP to
            // bind the cuff/respawn FX.
            if (Arrest->ConsumeClearHeatRequest())
            {
                if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
                {
                    Wanted->Clear();
                }
            }
        }
    }
}

void AGTCPoliceDirector::RefreshCombatSnapshot()
{
    CachedStars = 0;
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            CachedStars = Wanted->Stars();
        }
    }

    const APawn* Player = GetPlayerPawn();
    const UWorld* World = GetWorld();
    if (Player == nullptr || World == nullptr)
    {
        CachedNearestCopDistM = 1.0e6;
        bPlayerSeenByPolice = false;
        return;
    }
    const FVector PlayerPos = Player->GetActorLocation();
    const FVector PlayerChest = PlayerPos + FVector(0.0, 0.0, 50.0);

    auto HasLosTo = [&](const FVector& From, const AActor* Ignore) -> bool
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCSeenTrace), /*bTraceComplex*/ false, Ignore);
        const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, PlayerChest, ECC_Visibility, Params);
        return !bBlocked || Hit.GetActor() == Player;
    };

    // Nearest LIVING, NON-STUNNED officer across ALL officers (TActorIterator) — not just
    // the director's pool — so van/heli-dropped troopers also count toward the bust/seen
    // checks, while a ragdoll or a flashbanged cop never corners the player.
    double NearestSqCm = TNumericLimits<double>::Max();
    const AGTCPoliceOfficer* Nearest = nullptr;
    int32 Alive = 0;
    for (TActorIterator<AGTCPoliceOfficer> It(World); It; ++It)
    {
        const AGTCPoliceOfficer* O = *It;
        if (O == nullptr || O->IsDead() || O->IsStunned())
        {
            continue;
        }
        ++Alive;
        const double DSq = FVector::DistSquaredXY(O->GetActorLocation(), PlayerPos);
        if (DSq < NearestSqCm)
        {
            NearestSqCm = DSq;
            Nearest = O;
        }
    }
    CachedNearestCopDistM = (Alive > 0) ? (FMath::Sqrt(NearestSqCm) / MetresToCm) : 1.0e6;

    // "Seen" for the escape loop. The chopper needs an ACTUAL clear sightline (so you can
    // break it by ducking under cover at 3+ stars — being airborne alone no longer pins
    // you); on the ground, the nearest officer or a cruiser within vision range with LOS.
    bPlayerSeenByPolice = false;
    if (const AGTCPoliceHelicopter* Heli = Helicopter.Get())
    {
        if (!Heli->IsDead() && HasLosTo(Heli->GetActorLocation(), Heli))
        {
            bPlayerSeenByPolice = true;
        }
    }
    if (!bPlayerSeenByPolice && Nearest != nullptr && CachedNearestCopDistM <= PoliceVisionRangeM)
    {
        bPlayerSeenByPolice = HasLosTo(Nearest->GetActorLocation() + FVector(0.0, 0.0, 60.0), Nearest);
    }
    if (!bPlayerSeenByPolice)
    {
        const double VisionCm = PoliceVisionRangeM * MetresToCm;
        const double VisionSqCm = VisionCm * VisionCm;
        for (const TWeakObjectPtr<AGTCPoliceCar>& Weak : Cars)
        {
            const AGTCPoliceCar* Car = Weak.Get();
            if (Car == nullptr || Car->IsDead())
            {
                continue;
            }
            if (FVector::DistSquaredXY(Car->GetActorLocation(), PlayerPos) <= VisionSqCm
                && HasLosTo(Car->GetActorLocation() + FVector(0.0, 0.0, 80.0), Car))
            {
                bPlayerSeenByPolice = true;
                break;
            }
        }
    }
}

APawn* AGTCPoliceDirector::GetPlayerPawn() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

void AGTCPoliceDirector::StreamPolice()
{
    UWorld* World = GetWorld();
    APawn* Player = GetPlayerPawn();
    if (World == nullptr || Player == nullptr)
    {
        return; // no anchor to stream around
    }
    const FVector PlayerPos = Player->GetActorLocation();
    const int32 Stars = CachedStars; // refreshed this frame by RefreshCombatSnapshot

    // Drop dead/invalid officers (a corpse keeps ragdolling and ends itself via its
    // own SetLifeSpan — it just stops counting toward the live force), and recall
    // officers that should stand down. Count only the LIVING toward the deficit so a
    // ragdoll never suppresses reinforcements. Destroy() outside the loop — it can
    // re-enter and mutate the array.
    int32 Alive = 0;
    TArray<AGTCPoliceOfficer*> ToRetire;
    for (int32 i = Officers.Num() - 1; i >= 0; --i)
    {
        AGTCPoliceOfficer* O = Officers[i].Get();
        if (O == nullptr || O->IsActorBeingDestroyed() || O->IsDead())
        {
            Officers.RemoveAtSwap(i);
            continue;
        }
        const double DistM =
            FMath::Sqrt(FVector::DistSquaredXY(O->GetActorLocation(), PlayerPos)) / MetresToCm;
        if (FPoliceDispatch::ShouldDespawn(Stars, DistM, DespawnRadiusMeters))
        {
            ToRetire.Add(O);
            Officers.RemoveAtSwap(i);
            continue;
        }
        ++Alive;
    }
    for (AGTCPoliceOfficer* O : ToRetire)
    {
        O->Destroy();
    }

    // Top the field back up toward the heat's desired head-count.
    if (Stars > 0)
    {
        const double RadiusM = FPoliceResponse::SpawnRadius(Stars);
        const TArray<FPoliceSpawnSlot> Wave =
            FPoliceSpawnPlan::Build(Stars, Alive, MaxAlive, MaxPerWave, RadiusM, Rng);
        for (const FPoliceSpawnSlot& Slot : Wave)
        {
            // No air unit on foot in this slice: a helicopter slot is embodied as a
            // heavy ground responder (SWAT) so the wave still fills the heat's
            // head-count exactly (no skipped/under-filled wave). The real chopper is
            // a documented follow-up.
            const EPoliceUnitType Type = (Slot.UnitType == EPoliceUnitType::Helicopter)
                ? EPoliceUnitType::Swat
                : Slot.UnitType;
            const FVector WorldPoint = PlayerPos + FromCorePlanar(Slot.PlanarOffset) * MetresToCm;
            SpawnOfficer(static_cast<int32>(Type), WorldPoint);
        }
    }

    // Air support: a single chopper joins or leaves the chase by heat.
    StreamHelicopter(PlayerPos, Stars);
    // Ground pursuit: a small pool of cruisers from 2 stars.
    StreamCars(PlayerPos, Stars);
    // Barriers: maybe throw a roadblock across the road ahead.
    StreamRoadblock(PlayerPos, Stars);
    // Heavy ground response: a SWAT van rolls in at high heat.
    StreamSwatVan(PlayerPos, Stars);
    // K9 pack joins the foot chase at low heat.
    StreamK9(PlayerPos, Stars);
}

void AGTCPoliceDirector::StreamRoadblock(const FVector& PlayerPos, int32 Stars)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    AGTCRoadblock* Active = Roadblock.Get();
    const bool bActive = Active != nullptr && !Active->IsActorBeingDestroyed();

    // Tick the anti-spam gap down ONLY while no block is standing, so the cooldown
    // measures time since the last block was removed (not time since it was thrown).
    if (!bActive && RoadblockCooldown > 0.0)
    {
        RoadblockCooldown -= StreamIntervalSec;
    }

    if (bActive || RoadblockCooldown > 0.0 || Stars <= 0)
    {
        return;
    }

    // Only worth a block while the player is actually fleeing at speed.
    APawn* Player = GetPlayerPawn();
    if (Player == nullptr)
    {
        return;
    }
    FVector Vel = Player->GetVelocity();
    Vel.Z = 0.0;
    const double Speed = Vel.Size();
    if (Speed < 600.0)
    {
        return; // walking/standing — no roadblock
    }

    // Roll whether the heat sets one up (FPoliceEscalation::RoadblockChance).
    if (Rng.FRand() >= FPoliceEscalation::RoadblockChance(Stars))
    {
        RoadblockCooldown = 3.0; // brief re-roll delay
        return;
    }

    const FVector FleeDir = Vel.GetSafeNormal();
    const double Lead = FRoadblock::LeadDistance(Speed);
    FVector Center = FRoadblock::BlockadeCenter(PlayerPos, FleeDir, Lead);
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Center, Projected, FVector(800.0, 800.0, 800.0)))
        {
            Center = Projected.Location;
        }
    }
    Center.Z += 30.0;

    constexpr double RoadWidth = 1300.0;
    // Leave a squeeze-through gap (one fewer than full span) so it's threadable.
    const int32 UnitCount = FMath::Max(1, FRoadblock::UnitsToSpan(RoadWidth) - 1);

    TSubclassOf<AGTCRoadblock> SpawnClass = RoadblockClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCRoadblock::StaticClass();
    }
    const FTransform Xform(FRotator::ZeroRotator, Center);
    if (AGTCRoadblock* Block = World->SpawnActor<AGTCRoadblock>(SpawnClass, Xform))
    {
        Block->Setup(FleeDir, RoadWidth, UnitCount, /*LifeSeconds*/ 25.0);
        Roadblock = Block;
    }

    // Lay a spike strip on the approach (between the player and the block) to funnel
    // a fleeing car into the wall.
    {
        TSubclassOf<AGTCSpikeStrip> SpikeClass = SpikeStripClass;
        if (!SpikeClass)
        {
            SpikeClass = AGTCSpikeStrip::StaticClass();
        }
        FVector SpikePoint = PlayerPos + FleeDir * (Lead * 0.6);
        if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(SpikePoint, Projected, FVector(800.0, 800.0, 800.0)))
            {
                SpikePoint = Projected.Location;
            }
        }
        SpikePoint.Z += 8.0;
        if (AGTCSpikeStrip* Spikes =
                World->SpawnActor<AGTCSpikeStrip>(SpikeClass, FTransform(FRotator::ZeroRotator, SpikePoint)))
        {
            Spikes->Setup(FleeDir, RoadWidth, /*LifeSeconds*/ 25.0);
        }
    }
    RoadblockCooldown = 12.0; // don't spam blocks
}

void AGTCPoliceDirector::StreamCars(const FVector& PlayerPos, int32 Stars)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // Prune dead/invalid cars and recall any that have fallen too far behind.
    int32 Alive = 0;
    for (int32 i = Cars.Num() - 1; i >= 0; --i)
    {
        AGTCPoliceCar* Car = Cars[i].Get();
        if (Car == nullptr || Car->IsActorBeingDestroyed() || Car->IsDead())
        {
            Cars.RemoveAtSwap(i);
            continue;
        }
        const double DistM =
            FMath::Sqrt(FVector::DistSquaredXY(Car->GetActorLocation(), PlayerPos)) / MetresToCm;
        if (FPoliceDispatch::ShouldDespawn(Stars, DistM, DespawnRadiusMeters * 1.5)) // cars roam wider
        {
            Car->Destroy();
            Cars.RemoveAtSwap(i);
            continue;
        }
        ++Alive;
    }

    // Cruisers roll in from 2 stars; trickle up toward MaxCars one per pass.
    constexpr int32 CruiserStars = 2;
    if (Stars >= CruiserStars && Alive < MaxCars)
    {
        TSubclassOf<AGTCPoliceCar> SpawnClass = CarClass;
        if (!SpawnClass)
        {
            SpawnClass = AGTCPoliceCar::StaticClass();
        }
        // Spawn out on the road ring (~70 m), NavMesh-projected, facing the player.
        const double Angle = Rng.FRand() * UE_DOUBLE_TWO_PI;
        FVector Point = PlayerPos + FVector(FMath::Cos(Angle) * 7000.0, FMath::Sin(Angle) * 7000.0, 0.0);
        if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(Point, Projected, FVector(600.0, 600.0, 800.0)))
            {
                Point = Projected.Location;
            }
        }
        Point.Z += 60.0;
        FVector ToPlayer = PlayerPos - Point;
        ToPlayer.Z = 0.0;
        const FTransform Xform(ToPlayer.IsNearlyZero() ? FRotator::ZeroRotator : ToPlayer.Rotation(), Point);
        if (AGTCPoliceCar* Car = World->SpawnActor<AGTCPoliceCar>(SpawnClass, Xform))
        {
            Cars.Add(Car);
        }
    }
}

void AGTCPoliceDirector::StreamHelicopter(const FVector& PlayerPos, int32 Stars)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    AGTCPoliceHelicopter* Heli = Helicopter.Get();
    const bool bAlive = Heli != nullptr && !Heli->IsActorBeingDestroyed() && !Heli->IsDead();

    if (!FHelicopterPursuit::ShouldDeploy(Stars))
    {
        // Stood down: recall a flying chopper (a downed wreck finishes its own lifespan).
        if (bAlive)
        {
            Heli->Destroy();
        }
        Helicopter = nullptr;
        return;
    }
    if (bAlive)
    {
        return; // one chopper is enough
    }

    // Deploy a single unit high above the player; it flies down to its orbit.
    TSubclassOf<AGTCPoliceHelicopter> SpawnClass = HelicopterClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCPoliceHelicopter::StaticClass();
    }
    const FTransform Xform(FRotator::ZeroRotator, PlayerPos + FVector(0.0, 0.0, 3200.0));
    Helicopter = World->SpawnActor<AGTCPoliceHelicopter>(SpawnClass, Xform);
}

void AGTCPoliceDirector::StreamSwatVan(const FVector& PlayerPos, int32 Stars)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    AGTCSwatVan* Van = SwatVan.Get();
    const bool bAlive = Van != nullptr && !Van->IsActorBeingDestroyed() && !Van->IsDead();

    // Heavy SWAT-grade ground response joins at 4+ stars; recall it once heat falls.
    if (Stars < 4)
    {
        if (bAlive)
        {
            Van->Destroy();
        }
        SwatVan = nullptr;
        return;
    }
    if (bAlive)
    {
        return; // one van at a time
    }

    TSubclassOf<AGTCSwatVan> SpawnClass = SwatVanClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCSwatVan::StaticClass();
    }

    // Spawn a good distance off so it visibly rolls in before unloading.
    FVector Point = PlayerPos + FVector(5000.0, 0.0, 0.0);
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Point, Projected, FVector(2000.0, 2000.0, 600.0)))
        {
            Point = Projected.Location;
        }
    }
    Point.Z += 120.0; // van half-height above the road
    const FTransform Xform(FRotator::ZeroRotator, Point);
    SwatVan = World->SpawnActor<AGTCSwatVan>(SpawnClass, Xform);
}

void AGTCPoliceDirector::StreamK9(const FVector& PlayerPos, int32 Stars)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // Prune dead/invalid dogs and recall any that have fallen too far behind.
    int32 Alive = 0;
    for (int32 i = K9s.Num() - 1; i >= 0; --i)
    {
        AGTCK9* Dog = K9s[i].Get();
        if (Dog == nullptr || Dog->IsActorBeingDestroyed() || Dog->IsDead())
        {
            K9s.RemoveAtSwap(i);
            continue;
        }
        const double DistM =
            FMath::Sqrt(FVector::DistSquaredXY(Dog->GetActorLocation(), PlayerPos)) / MetresToCm;
        if (FPoliceDispatch::ShouldDespawn(Stars, DistM, DespawnRadiusMeters))
        {
            Dog->Destroy();
            K9s.RemoveAtSwap(i);
            continue;
        }
        ++Alive;
    }

    // Dogs join from 1 star; trickle up toward MaxK9 one per pass.
    constexpr int32 K9Stars = 1;
    if (Stars >= K9Stars && Alive < MaxK9)
    {
        TSubclassOf<AGTCK9> SpawnClass = K9Class;
        if (!SpawnClass)
        {
            SpawnClass = AGTCK9::StaticClass();
        }
        // Spawn on a near foot ring (~25 m), NavMesh-projected, facing the player.
        const double Angle = Rng.FRand() * UE_DOUBLE_TWO_PI;
        FVector Point = PlayerPos + FVector(FMath::Cos(Angle) * 2500.0, FMath::Sin(Angle) * 2500.0, 0.0);
        if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(Point, Projected, FVector(400.0, 400.0, 600.0)))
            {
                Point = Projected.Location;
            }
        }
        Point.Z += 50.0;
        FVector ToPlayer = PlayerPos - Point;
        ToPlayer.Z = 0.0;
        const FTransform Xform(ToPlayer.IsNearlyZero() ? FRotator::ZeroRotator : ToPlayer.Rotation(), Point);
        AGTCK9* Dog = World->SpawnActorDeferred<AGTCK9>(
            SpawnClass, Xform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (Dog != nullptr)
        {
            Dog->InitializeK9(SpawnCounter++);
            Dog->FinishSpawning(Xform);
            K9s.Add(Dog);
        }
    }
}

void AGTCPoliceDirector::SpawnOfficer(int32 UnitTypeRaw, const FVector& WorldPoint)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    // Drop onto walkable ground when a NavMesh exists; otherwise spawn on the ring so
    // the response is visibly alive even before the level's nav is built (matches the
    // crowd streamer's fallback).
    FVector Point = WorldPoint;
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Point, Projected, FVector(300.0, 300.0, 600.0)))
        {
            Point = Projected.Location;
        }
    }
    Point.Z += 90.0; // clear the capsule half-height above the floor

    // Face the player on spawn so the officer starts oriented into the fight.
    FRotator Rot = FRotator::ZeroRotator;
    if (const APawn* Player = GetPlayerPawn())
    {
        FVector ToPlayer = Player->GetActorLocation() - Point;
        ToPlayer.Z = 0.0;
        if (!ToPlayer.IsNearlyZero())
        {
            Rot = ToPlayer.Rotation();
        }
    }
    const FTransform Xform(Rot, Point);

    TSubclassOf<AGTCPoliceOfficer> SpawnClass = OfficerClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCPoliceOfficer::StaticClass();
    }

    AGTCPoliceOfficer* Officer = World->SpawnActorDeferred<AGTCPoliceOfficer>(
        SpawnClass, Xform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (Officer == nullptr)
    {
        return;
    }
    // Stamp the loadout/toughness before BeginPlay so the officer comes up fully formed.
    Officer->InitializeUnit(static_cast<EPoliceUnitType>(UnitTypeRaw), SpawnCounter++);
    Officer->FinishSpawning(Xform);
    Officers.Add(Officer);
}
