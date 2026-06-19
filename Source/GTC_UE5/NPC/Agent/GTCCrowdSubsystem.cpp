// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCCrowdSubsystem.h"

#include "GTCCitizen.h"
#include "../Decision/NpcBrain.h"
#include "../Decision/NpcSchedule.h"
#include "../Decision/NpcWitness.h"
#include "../../World/Places/GTCPlaceRegistrySubsystem.h"
#include "../../Vehicles/GTCTrafficSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Math/UnrealMathUtility.h"

void UGTCCrowdSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    HourOfDay = FNpcSchedule::WrapHour(StartHour);
    if (!CitizenClass)
    {
        CitizenClass = AGTCCitizen::StaticClass();
    }

    // Lay down the persistent cast up front, then settle it to the start hour so the
    // first citizens you meet already have a morning behind them, not factory-fresh
    // drives. POI seeding happens lazily once we have a player anchor.
    if (bPersistentPopulation && PersistentPopulationSize > 0)
    {
        // GTC: a fresh random cast each session unless a fixed seed was authored.
        // PopulationSeed == 0 (the default) => roll a new seed from the launch clock;
        // set any non-zero PopulationSeed to pin a reproducible cast across runs.
        if (PopulationSeed == 0)
        {
            PopulationSeed = static_cast<int32>(FPlatformTime::Cycles());
        }
        Population.Seed(PersistentPopulationSize, PopulationSeed);
        Population.Advance(HourOfDay, FMath::Fmod(HourOfDay, 24.0));
    }
}

void UGTCCrowdSubsystem::Deinitialize()
{
    Citizens.Reset();
    Super::Deinitialize();
}

bool UGTCCrowdSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    // Live gameplay only — no editor-preview / inactive worlds.
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

double UGTCCrowdSubsystem::GetHoursPerSecond() const
{
    return 24.0 / FMath::Max(1.0, SecondsPerDay);
}

void UGTCCrowdSubsystem::SetHourOfDay(double Hour)
{
    HourOfDay = FNpcSchedule::WrapHour(Hour);
}

TStatId UGTCCrowdSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTCCrowdSubsystem, STATGROUP_Tickables);
}

void UGTCCrowdSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    // Advance the master clock.
    HourOfDay = FNpcSchedule::WrapHour(HourOfDay + static_cast<double>(DeltaTime) * GetHoursPerSecond());

    // Live the off-screen cast forward on the same clock (its own coarse cadence).
    AdvancePopulation(DeltaTime);

    // Compact any citizens that died elsewhere (weak ptr gone stale).
    Citizens.RemoveAll([](const TWeakObjectPtr<AGTCCitizen>& C) { return !C.IsValid(); });

    // Stream the crowd on its own cadence (not every frame).
    StreamAccum += DeltaTime;
    if (bStreamPopulation && StreamAccum >= StreamIntervalSec)
    {
        StreamAccum = 0.0;
        StreamPopulation(DeltaTime);
    }
}

APawn* UGTCCrowdSubsystem::GetPlayerPawn() const
{
    if (const UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

FGTCThreatSnapshot UGTCCrowdSubsystem::GetPlayerThreat() const
{
    FGTCThreatSnapshot Snap;
    if (APawn* Pawn = GetPlayerPawn())
    {
        Snap.bValid = true;
        Snap.Position = Pawn->GetActorLocation();
        FVector Vel = Pawn->GetVelocity();
        Vel.Z = 0.0; // planar — contact reads happen on the ground plane
        Snap.Velocity = Vel;
        Snap.Speed = Vel.Size(); // cm/s, planar
        Snap.bArmed = bPlayerArmed;
    }
    return Snap;
}

void UGTCCrowdSubsystem::RegisterCitizen(AGTCCitizen* Citizen)
{
    if (!Citizen)
    {
        return;
    }
    const TWeakObjectPtr<AGTCCitizen> Weak(Citizen);
    if (!Citizens.Contains(Weak))
    {
        Citizens.Add(Weak);
    }
    // Sync the newcomer to the weather that's already out (it missed the last
    // broadcast), so a citizen spawned mid-storm reacts immediately.
    if (WeatherSeverity > 0)
    {
        Citizen->ReactToWeather(WeatherSeverity);
    }
}

void UGTCCrowdSubsystem::SetWeatherSeverity(int32 Severity)
{
    const int32 Clamped = FMath::Clamp(Severity, 0, 4);
    if (Clamped == WeatherSeverity)
    {
        return; // no change — don't re-broadcast (controller calls this every tick)
    }
    WeatherSeverity = Clamped;

    // The sky turned: let the whole live crowd react together. Each citizen decides
    // its own response (shelter/hurry/grumble) from its bravery + the hour, and the
    // grumble is cadence-gated, so this reads as a city noticing — not a chorus.
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        if (AGTCCitizen* C = Weak.Get())
        {
            C->ReactToWeather(WeatherSeverity);
        }
    }
}

void UGTCCrowdSubsystem::UnregisterCitizen(AGTCCitizen* Citizen)
{
    Citizens.RemoveAll(
        [Citizen](const TWeakObjectPtr<AGTCCitizen>& C) { return !C.IsValid() || C.Get() == Citizen; });
}

void UGTCCrowdSubsystem::ReleaseCitizenRecord(
    int32 StableId, const FNpcNeeds& Needs, const FNpcIntent& Intent, const FVector& LastPos)
{
    // A body retired (or died): copy its lived state home (and where it stood) and
    // re-open the record so the off-screen sim picks the person back up.
    Population.ReleaseRecord(StableId, Needs, Intent, LastPos, /*bStoreLastPosition=*/true);
}

void UGTCCrowdSubsystem::StoreCitizenAcquaintance(int32 StableId, const FNpcAcquaintance& Acq)
{
    Population.StoreAcquaintance(StableId, Acq);
}

void UGTCCrowdSubsystem::AdvancePopulation(float DeltaTime)
{
    if (!bPersistentPopulation || Population.Num() == 0)
    {
        return;
    }

    PopulationAccum += DeltaTime;
    if (PopulationAccum < PopulationAdvanceIntervalSec)
    {
        return; // census drifts slowly — no need to re-simulate every frame.
    }

    const double ElapsedHours = PopulationAccum * GetHoursPerSecond();
    PopulationAccum = 0.0;
    Population.Advance(HourOfDay, ElapsedHours);
}

void UGTCCrowdSubsystem::GatherNeighbors(
    const FVector& From, double Radius, const AGTCCitizen* Exclude, TArray<FVector>& OutLocations) const
{
    OutLocations.Reset();
    const double RadiusSq = Radius * Radius;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        const AGTCCitizen* C = Weak.Get();
        if (!C || C == Exclude)
        {
            continue;
        }
        const FVector Loc = C->GetActorLocation();
        const double DistSq = FVector::DistSquaredXY(Loc, From);
        if (DistSq <= RadiusSq)
        {
            OutLocations.Add(Loc);
            if (OutLocations.Num() >= MaxNeighbors)
            {
                break; // cost bound — separation doesn't need the whole crowd.
            }
        }
    }
}

void UGTCCrowdSubsystem::GatherNearbyCars(
    const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const
{
    // Forward to the traffic layer: pedestrians dodge actual streamed cars.
    OutCars.Reset();
    if (const UWorld* World = GetWorld())
    {
        if (UGTCTrafficSubsystem* Traffic = World->GetSubsystem<UGTCTrafficSubsystem>())
        {
            Traffic->QueryCars(From, Radius, OutCars);
        }
    }
}

bool UGTCCrowdSubsystem::NearestPanicSource(
    const FVector& From, double Radius, const AGTCCitizen* Exclude, FVector& OutLocation) const
{
    const double RadiusSq = Radius * Radius;
    double BestSq = RadiusSq;
    bool bFound = false;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        const AGTCCitizen* C = Weak.Get();
        if (!C || C == Exclude)
        {
            continue;
        }
        if (C->GetBrainState() != static_cast<int32>(FNpcBrain::EState::Flee))
        {
            continue;
        }
        const FVector Loc = C->GetActorLocation();
        const double DistSq = FVector::DistSquaredXY(Loc, From);
        if (DistSq <= BestSq)
        {
            BestSq = DistSq;
            OutLocation = Loc;
            bFound = true;
        }
    }
    return bFound;
}

AGTCCitizen* UGTCCrowdSubsystem::FindChatPartner(
    const FVector& From, double Radius, AGTCCitizen* Self) const
{
    const double RadiusSq = Radius * Radius;
    AGTCCitizen* Best = nullptr;
    double BestSq = RadiusSq;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        AGTCCitizen* C = Weak.Get();
        if (!C || C == Self || !C->IsAvailableToChat())
        {
            continue;
        }
        const double DistSq = FVector::DistSquaredXY(C->GetActorLocation(), From);
        if (DistSq <= BestSq)
        {
            BestSq = DistSq;
            Best = C;
        }
    }
    return Best;
}

AGTCCitizen* UGTCCrowdSubsystem::FindNearestAcquaintance(
    const FVector& From, double Radius, AGTCCitizen* Self) const
{
    const double RadiusSq = Radius * Radius;
    AGTCCitizen* Best = nullptr;
    double BestSq = RadiusSq;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        AGTCCitizen* C = Weak.Get();
        if (!C || C == Self || !Self->Recognizes(C))
        {
            continue;
        }
        const double DistSq = FVector::DistSquaredXY(C->GetActorLocation(), From);
        if (DistSq <= BestSq)
        {
            BestSq = DistSq;
            Best = C;
        }
    }
    return Best;
}

void UGTCCrowdSubsystem::BroadcastMayhem(
    const FVector& AtLocation, double BaseIntensity, const AGTCCitizen* Victim)
{
    if (BaseIntensity <= 0.0)
    {
        return;
    }
    // Safety: this feeds ONLY WitnessPlayerMayhem (raises memory via Max), never
    // ApplyContact — so a witness can't emit its own broadcast and this cannot
    // recurse. The crowd-wide panic that can follow (an alarmed witness flees, and
    // the pre-existing NearestPanicSource/CatchesPanic contagion spreads it) is
    // intended and self-limiting: bravery gates contagion, memory decays in ~13s,
    // flee only triggers within FleeRadius of the player, and the live count is small.
    const double RadiusCm = FNpcWitness::DefaultRadiusM * 100.0; // m -> cm (must match FNpcMemory's metre range)
    const double RadiusSq = RadiusCm * RadiusCm;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        AGTCCitizen* C = Weak.Get();
        if (!C || C == Victim)
        {
            continue; // the victim already took their own memory hit on contact
        }
        const double DistSq = FVector::DistSquaredXY(C->GetActorLocation(), AtLocation);
        if (DistSq > RadiusSq)
        {
            continue;
        }
        // FNpcWitness speaks metres (like FNpcMemory); convert at the boundary.
        const double Share = FNpcWitness::Share(BaseIntensity, FMath::Sqrt(DistSq) / 100.0);
        if (Share > 0.0)
        {
            C->WitnessPlayerMayhem(Share);
        }
    }
}

bool UGTCCrowdSubsystem::PickSpawnPoint(const FVector& Around, FVector& OutPoint) const
{
    const double Angle = FMath::FRandRange(0.0, UE_DOUBLE_TWO_PI);
    const double R = FMath::FRandRange(SpawnRadiusMin, SpawnRadiusMax);
    FVector Point = Around + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, 0.0);

    // Drop onto walkable ground when a NavMesh exists; otherwise spawn on the ring
    // so the system is visibly alive even before the level's nav is built.
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Point, Projected, FVector(200.0, 200.0, 500.0)))
        {
            OutPoint = Projected.Location;
            OutPoint.Z += 90.0; // clear the capsule half-height above the floor.
            return true;
        }
    }

    OutPoint = Point;
    OutPoint.Z += 90.0;
    return true;
}

void UGTCCrowdSubsystem::StreamPopulation(float DeltaTime)
{
    UWorld* World = GetWorld();
    APawn* Player = GetPlayerPawn();
    if (!World || !Player)
    {
        return; // No anchor to stream around.
    }
    const FVector PlayerPos = Player->GetActorLocation();

    // Lay down the city's POIs around the player anchor the first time we have one,
    // so citizens in maps with no hand-authored markers (CityBeachStrip) still walk
    // to real, spread-out destinations.
    if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
    {
        Places->EnsureSeeded(PlayerPos);
    }

    // Retire citizens that have fallen too far behind the player. Collect first,
    // then destroy — Destroy() re-enters UnregisterCitizen and mutates Citizens.
    const double DespawnSq = DespawnRadius * DespawnRadius;
    TArray<AGTCCitizen*> ToRetire;
    int32 LiveInRange = 0;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        AGTCCitizen* C = Weak.Get();
        if (!C)
        {
            continue;
        }
        if (FVector::DistSquaredXY(C->GetActorLocation(), PlayerPos) > DespawnSq)
        {
            ToRetire.Add(C);
        }
        else
        {
            ++LiveInRange;
        }
    }
    for (AGTCCitizen* C : ToRetire)
    {
        C->Destroy();
    }

    // Top the crowd back up, amortised across passes.
    int32 Spawned = 0;
    TSubclassOf<AGTCCitizen> SpawnClass = CitizenClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCCitizen::StaticClass();
    }
    while (LiveInRange + Spawned < TargetPopulation && Spawned < MaxSpawnsPerPass)
    {
        // Embody a persistent census member when running the persistent population:
        // claim a free roster record FIRST so we can place its body where the person
        // actually is. The body restores its carried drives/intent (continuity) and
        // binds back for write-on-retire. If the whole cast is already on screen,
        // fall through to a fresh seed on the ring.
        int32 RecordId = INDEX_NONE;
        if (bPersistentPopulation)
        {
            RecordId = Population.AcquireFreeRecord();
        }

        // Release helper for the abort paths below (hands an un-bodied claim back).
        auto ReleaseClaimedRecord = [this, RecordId]()
        {
            if (RecordId != INDEX_NONE)
            {
                if (const FNpcCitizenRecord* Rec = Population.Find(RecordId))
                {
                    Population.ReleaseRecord(RecordId, Rec->Needs, Rec->CurrentIntent);
                }
            }
        };

        // Prefer to spawn the citizen AT its current activity's place when that spot
        // is in play near the player — so a "sleep@home" person appears at a nearby
        // home and a "socialize@bar" person at a bar, not a random ring point. The
        // place must be inside the despawn bubble (else it'd retire instantly) and
        // not right on top of the player; otherwise we fall back to the ring.
        const double MinSpawnSq = 500.0 * 500.0; // never spawn right on the player.
        FVector Point;
        bool bPlaced = false;
        if (RecordId != INDEX_NONE)
        {
            if (const FNpcCitizenRecord* Rec = Population.Find(RecordId))
            {
                // 1. Continuity: if we saw this person recently and they're still in
                //    range, put them back roughly where they were.
                if (Rec->bHasLastPosition)
                {
                    const double DistSq = FVector::DistSquaredXY(Rec->LastPosition, PlayerPos);
                    if (DistSq >= MinSpawnSq && DistSq <= DespawnSq)
                    {
                        Point = Rec->LastPosition;
                        bPlaced = true;
                    }
                }

                // 2. Else place them at their current activity's location when it's
                //    in play near the player.
                if (!bPlaced && !Rec->CurrentIntent.Place.IsEmpty())
                {
                    if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
                    {
                        const FGTCPlaceQueryResult Q =
                            Places->FindAvailable(FName(*Rec->CurrentIntent.Place.ToLower()), PlayerPos);
                        if (Q.bValid)
                        {
                            const double DistSq = FVector::DistSquaredXY(Q.Location, PlayerPos);
                            if (DistSq >= MinSpawnSq && DistSq <= DespawnSq)
                            {
                                Point = Q.Location;
                                Point.Z += 90.0; // clear the capsule half-height.
                                bPlaced = true;
                            }
                        }
                    }
                }
            }
        }
        // 3. Else the streaming ring.
        if (!bPlaced && !PickSpawnPoint(PlayerPos, Point))
        {
            ReleaseClaimedRecord();
            break;
        }

        const FRotator Rot(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
        const FTransform Xform(Rot, Point);

        AGTCCitizen* C = World->SpawnActorDeferred<AGTCCitizen>(
            SpawnClass, Xform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (!C)
        {
            ReleaseClaimedRecord();
            break;
        }

        // Stamp identity before BeginPlay so the citizen registers fully formed.
        if (const FNpcCitizenRecord* Rec = (RecordId != INDEX_NONE) ? Population.Find(RecordId) : nullptr)
        {
            C->InitializeFromRecord(*Rec);
        }
        else
        {
            C->InitializeFromSeed(SpawnCounter++);
        }
        C->FinishSpawning(Xform);
        ++Spawned;
    }

    ApplyTickLod(PlayerPos);

#if !UE_BUILD_SHIPPING
    // Heartbeat (throttled to ~every 5s): objective evidence that the crowd is
    // alive and MOVING — sample one citizen's position/speed/activity so a headless
    // boot log shows it walking, not just existing. Grep "[GTCCrowd]".
    static int32 HeartbeatTick = 0;
    if (((HeartbeatTick++) % 10) == 0)
    {
        AGTCCitizen* Sample = nullptr;
        for (const TWeakObjectPtr<AGTCCitizen>& W : Citizens)
        {
            if (W.IsValid()) { Sample = W.Get(); break; }
        }
        if (Sample)
        {
            const FVector L = Sample->GetActorLocation();
            UE_LOG(LogTemp, Display,
                TEXT("[GTCCrowd] hour=%.1f live=%d sample='%s' act=%s reason=%s state=%d pos=(%.0f,%.0f) spd=%.0f"),
                HourOfDay, Citizens.Num(), *Sample->GetArchetypeName(), *Sample->GetCurrentActivity(),
                *Sample->GetIntentReason(), Sample->GetBrainState(), L.X, L.Y, Sample->GetVelocity().Size2D());
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("[GTCCrowd] no live citizens yet (anchor=%s)"), *PlayerPos.ToString());
        }
    }
#endif
}

void UGTCCrowdSubsystem::ApplyTickLod(const FVector& PlayerPos)
{
    const double NearSq = NearTickRadius * NearTickRadius;
    const double FaceSq = FacePromoteRadius * FacePromoteRadius;
    for (const TWeakObjectPtr<AGTCCitizen>& Weak : Citizens)
    {
        AGTCCitizen* C = Weak.Get();
        if (!C)
        {
            continue;
        }
        const double DistSq = FVector::DistSquaredXY(C->GetActorLocation(), PlayerPos);
        C->SetActorTickInterval(DistSq <= NearSq ? 0.0f : static_cast<float>(FarTickInterval));
        // Promote the face to the rigged hero head only inside the tight face radius,
        // so the costly head rides only the handful of faces the player can read.
        C->SetFaceHighDetail(DistSq <= FaceSq);
    }
}
