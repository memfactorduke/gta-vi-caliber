// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerSkills.h"
#include "SkillsCoordinator.generated.h"

/**
 * Live bridge for PlayerSkills — the UE 5.7 port of Godot's self-wiring
 * SkillsCoordinator Node. Owns the pure FPlayerSkills model and trains it from
 * player activity: on-foot distance trains stamina, driven-vehicle distance trains
 * driving, confirmed weapon hits train shooting. Exposes level()/tier()/bonus()/
 * overall_mastery() and Serialize/Restore for the save system.
 *
 * Lifecycle: a UGameInstanceSubsystem (skills are player-global, like the Godot group
 * "player_skills"). It owns a single FPlayerSkills for its whole lifetime.
 *
 * OWNERSHIP MODEL: SkillsCoordinator OWNS the skills model and exposes it via getters.
 * The Godot scene-graph wiring — resolving the player/vehicle Node3D, planar-distance
 * accumulation from global_position deltas, weapon "hit_confirmed" signal binding — is
 * Wave 3 engine wiring. Here the pure motion math is preserved as explicit driver
 * methods (TrainFromOnFootMove / TrainFromVehicleMove / OnHitConfirmed) that a future
 * scene adapter feeds positions into, so behavior stays headless-testable.
 */
UCLASS()
class GTC_UE5_API USkillsCoordinator : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Stamina effort per planar metre walked (Godot @export). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    double StaminaEffortPerMeter = 0.04;

    /** Driving effort per planar metre driven (Godot @export). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    double DrivingEffortPerMeter = 0.02;

    /** Effort granted per confirmed weapon hit (Godot @export). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    double ShootingHitEffort = 2.5;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Pass-through queries (delegate to the owned model) ----------------

    double TrainActivity(const FString& Id, double Effort);
    double Level(const FString& Id) const;
    FString Tier(const FString& Id) const;
    double Bonus(const FString& Id) const;
    double OverallMastery() const;

    /** Const access to the owned pure model (ownership stays with the subsystem). */
    const FPlayerSkills& GetSkills() const { EnsureModel(); return *_Skills; }

    // --- Drivers (motion math ported from the Godot Node) ------------------

    /**
     * On-foot movement from `From` to `To` (planar X/Z). Trains stamina by the
     * planar distance * StaminaEffortPerMeter when the player is visible and moved
     * more than 0.01m. Mirrors _train_stamina. Returns gain applied.
     */
    double TrainFromOnFootMove(const FVector& From, const FVector& To, bool bVisible = true);

    /**
     * Driven-vehicle movement from `From` to `To` (planar X/Z). Trains driving by the
     * planar distance * DrivingEffortPerMeter when moved more than 0.01m. Mirrors
     * _train_driving. Returns gain applied.
     */
    double TrainFromVehicleMove(const FVector& From, const FVector& To);

    /** A confirmed weapon hit: trains shooting by ShootingHitEffort. Mirrors _on_hit_confirmed. */
    double OnHitConfirmed();

    /** Planar (X/Z) distance between two points, mirroring Godot _planar_distance. */
    static double PlanarDistance(const FVector& A, const FVector& B);

    // --- Persistence (SaveManager) -----------------------------------------

    /**
     * Flatten the owned skills to {id: value} in insertion order. Named SerializeState
     * (not Serialize) to avoid hiding UObject::Serialize.
     */
    TArray<TPair<FString, double>> SerializeState() const;

    /** Restore skill values from a {id: value} snapshot (unknown ids ignored). */
    void Restore(const TArray<TPair<FString, double>>& Data);

private:
    /** Lazily allocate the owned model (with default skills) if not present. */
    void EnsureModel() const;

    mutable TUniquePtr<FPlayerSkills> _Skills;
};
