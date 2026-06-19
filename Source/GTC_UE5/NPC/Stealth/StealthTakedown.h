// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The stealth takedown — sneak up on a guard and drop him before he can react. FStealth
 * Detection already models BEING SEEN (the awareness meter UNAWARE->SUSPICIOUS->ALERTED);
 * this is the payoff on the other side: the geometry+state check for whether the player can
 * grab a target right now, and crucially whether it stays QUIET.
 *
 * The gameplay nuance it captures: you can take down anyone who isn't actively fighting you
 * (in reach, not yet alerted) from any angle — but only a takedown from BEHIND an UNAWARE
 * target is SILENT. Grab someone from the side and you'll get the kill, but it's a loud
 * scuffle that alerts everyone nearby (the adapter routes that through SoundPropagation /
 * raises FCrimeWitness). So stealth rewards patience and positioning.
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in, no engine coupling, no
 * RNG. The takedown adapter supplies the attacker + target positions and the target's facing
 * (ground-plane) plus its alerted flag (from FStealthDetection::IsAlerted), then on
 * CanExecute plays the montage + kills the target, and uses IsSilent to decide whether to
 * stay hidden or blow the alarm. Distances are the adapter's own units (UE cm); angles
 * radians. Unit-tested headless (NPC/Stealth/Tests/StealthTakedownTest.cpp, prefix
 * GTC.NPC.Stealth.Takedown).
 */
struct GTC_UE5_API FStealthTakedown
{
    /** How close (cm) the attacker must be to grab the target. */
    static constexpr double DefaultReach = 150.0;
    /** Half-angle (radians) of the target's rear arc that counts as "behind" (~60deg). */
    static constexpr double RearArcHalfAngleRad = 1.0472;

    /**
     * Angle (radians, 0..pi) between the target's BACK direction (-Facing) and the direction
     * from the target to the attacker. ~0 means the attacker is squarely behind; ~pi means
     * dead in front. Returns pi (treated as "in front") for a degenerate facing/coincident
     * positions, so a takedown is never silently granted on bad input.
     */
    static double RearAngleRad(const FVector& TargetFacing, const FVector& TargetPos, const FVector& AttackerPos);

    /** Is the attacker within the target's rear arc (behind it)? */
    static bool IsBehind(const FVector& TargetFacing, const FVector& TargetPos, const FVector& AttackerPos,
        double RearArcHalfAngle = RearArcHalfAngleRad);

    /** Is the attacker close enough to grab the target? */
    static bool InReach(const FVector& AttackerPos, const FVector& TargetPos, double Reach = DefaultReach);

    /**
     * Can a takedown be performed at all: the target isn't already alerted (that's a fight,
     * not a takedown) and the attacker is in reach. Angle does NOT gate execution — only
     * silence (see IsSilent).
     */
    static bool CanExecute(const FVector& AttackerPos, const FVector& TargetPos, const FVector& TargetFacing,
        bool bTargetAlerted, double Reach = DefaultReach);

    /**
     * Is the takedown silent (no alarm raised): only when the target is unaware AND the
     * attacker struck from behind. A side/front grab on a distracted target still works but
     * is loud.
     */
    static bool IsSilent(const FVector& AttackerPos, const FVector& TargetPos, const FVector& TargetFacing,
        bool bTargetAlerted, double RearArcHalfAngle = RearArcHalfAngleRad);
};
