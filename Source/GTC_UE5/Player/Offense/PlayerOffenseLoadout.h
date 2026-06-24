// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * FPlayerOffenseLoadout — the player's limited offensive consumables and action pacing:
 * flashbang / grenade / molotov ammo plus a shared throw cooldown and a melee cooldown.
 *
 * Plain non-UObject pure-core with NO engine dependencies (only `int`/`double`/`enum`),
 * so it unit-tests headless and host-compiles standalone. The player pawn owns one and
 * its throw/melee execs consume it; this is what turns the dev-console throw/melee verbs
 * into real, rate-limited, finite-resource abilities (and closes the chain-stun /
 * takedown-spam exploits — you can no longer throw or strike faster than the cooldown,
 * and throwables run out). Times are seconds on whatever clock the caller passes in
 * (the pawn passes world time); state is caller-owned so it's deterministic to test.
 */
enum class EThrowableKind : unsigned char
{
    Flashbang = 0,
    Grenade = 1,
    Molotov = 2,
    Count = 3
};

struct FPlayerOffenseLoadout
{
    /** Current rounds of each throwable, indexed by EThrowableKind. */
    int Ammo[3] = {2, 3, 2};
    /** Per-kind carry cap (AddAmmo clamps to this). */
    int MaxAmmo[3] = {3, 5, 3};

    /** Shared minimum gap between throws — well above the 3s flashbang stun so one
     *  thrower can never keep a target perma-frozen. */
    double ThrowCooldownSec = 1.2;
    /** Minimum gap between melee strikes — stops walking down a stunned line back-stabbing. */
    double MeleeCooldownSec = 0.8;

    double LastThrowTime = -1000.0;
    double LastMeleeTime = -1000.0;

    static int Index(EThrowableKind K) { return static_cast<int>(K); }

    int GetAmmo(EThrowableKind K) const { return Ammo[Index(K)]; }
    int GetMaxAmmo(EThrowableKind K) const { return MaxAmmo[Index(K)]; }

    bool ThrowReady(double Now) const { return (Now - LastThrowTime) >= ThrowCooldownSec; }
    bool CanThrow(EThrowableKind K, double Now) const { return Ammo[Index(K)] > 0 && ThrowReady(Now); }

    /** Consume one round and stamp the throw cooldown if allowed. Returns true iff the
     *  throw should actually happen (caller then spawns the projectile). */
    bool TryThrow(EThrowableKind K, double Now)
    {
        if (!CanThrow(K, Now))
        {
            return false;
        }
        Ammo[Index(K)] -= 1;
        LastThrowTime = Now;
        return true;
    }

    /** Add ammo of a kind, clamped to its carry cap. Returns the new count. */
    int AddAmmo(EThrowableKind K, int N)
    {
        int& A = Ammo[Index(K)];
        A += (N > 0 ? N : 0);
        if (A > MaxAmmo[Index(K)])
        {
            A = MaxAmmo[Index(K)];
        }
        if (A < 0)
        {
            A = 0;
        }
        return A;
    }

    bool MeleeReady(double Now) const { return (Now - LastMeleeTime) >= MeleeCooldownSec; }

    /** Stamp the melee cooldown if ready. Returns true iff the strike should happen. */
    bool TryMelee(double Now)
    {
        if (!MeleeReady(Now))
        {
            return false;
        }
        LastMeleeTime = Now;
        return true;
    }

    // --- Save persistence ------------------------------------------------------
    // Only the AMMO counts persist; the cooldown timestamps are session-relative wall
    // time and are deliberately NOT saved (a fresh session starts ready to throw).

    void SerializeAmmo(int& OutFlashbang, int& OutGrenade, int& OutMolotov) const
    {
        OutFlashbang = Ammo[Index(EThrowableKind::Flashbang)];
        OutGrenade = Ammo[Index(EThrowableKind::Grenade)];
        OutMolotov = Ammo[Index(EThrowableKind::Molotov)];
    }

    /** Restore ammo from a snapshot, each clamped to [0, carry cap] so a corrupt or
     *  out-of-date save can never exceed the loadout's limits. */
    void RestoreAmmo(int Flashbang, int Grenade, int Molotov)
    {
        SetClamped(EThrowableKind::Flashbang, Flashbang);
        SetClamped(EThrowableKind::Grenade, Grenade);
        SetClamped(EThrowableKind::Molotov, Molotov);
    }

private:
    void SetClamped(EThrowableKind K, int Value)
    {
        const int Cap = MaxAmmo[Index(K)];
        Ammo[Index(K)] = Value < 0 ? 0 : (Value > Cap ? Cap : Value);
    }
};
