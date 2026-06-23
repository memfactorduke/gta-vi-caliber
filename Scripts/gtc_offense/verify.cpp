// Host-clang runtime verification of FPlayerOffenseLoadout (no engine deps).
// Mirrors the GTC.Player.Offense.Loadout automation test so the ammo/cooldown logic
// is RUN-verified headlessly (the UE automation suite needs the editor we must not launch).
//
//   clang++ -std=c++17 -I../../Source/GTC_UE5/Player/Offense verify.cpp -o /tmp/offense_verify && /tmp/offense_verify

#include "PlayerOffenseLoadout.h"
#include <cstdio>
#include <cstdlib>

static int g_failures = 0;
static void Check(bool cond, const char* what)
{
    if (!cond)
    {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

int main()
{
    // Starting ammo.
    {
        FPlayerOffenseLoadout L;
        Check(L.GetAmmo(EThrowableKind::Flashbang) == 2, "flashbang start == 2");
        Check(L.GetAmmo(EThrowableKind::Grenade) == 3, "grenade start == 3");
        Check(L.GetAmmo(EThrowableKind::Molotov) == 2, "molotov start == 2");
    }
    // Throw consumes + respects cooldown + empties.
    {
        FPlayerOffenseLoadout L;
        Check(L.TryThrow(EThrowableKind::Flashbang, 0.0), "throw at 0 ok");
        Check(L.GetAmmo(EThrowableKind::Flashbang) == 1, "flashbang now 1");
        Check(!L.TryThrow(EThrowableKind::Flashbang, 0.5), "throw at 0.5 blocked (cooldown)");
        Check(L.GetAmmo(EThrowableKind::Flashbang) == 1, "ammo unchanged after blocked throw");
        Check(L.TryThrow(EThrowableKind::Flashbang, 1.3), "throw at 1.3 ok");
        Check(L.GetAmmo(EThrowableKind::Flashbang) == 0, "flashbang now 0");
        Check(!L.TryThrow(EThrowableKind::Flashbang, 10.0), "throw blocked when empty");
    }
    // AddAmmo clamps to max + ignores negatives.
    {
        FPlayerOffenseLoadout L;
        Check(L.AddAmmo(EThrowableKind::Grenade, 10) == 5, "grenade clamps to 5");
        Check(L.AddAmmo(EThrowableKind::Grenade, -3) == 5, "negative add ignored");
    }
    // Melee cooldown.
    {
        FPlayerOffenseLoadout L;
        Check(L.TryMelee(0.0), "melee at 0 ok");
        Check(!L.TryMelee(0.5), "melee at 0.5 blocked");
        Check(L.TryMelee(0.9), "melee at 0.9 ok");
    }
    // Serialize / restore ammo round-trip + clamp.
    {
        FPlayerOffenseLoadout L;
        L.TryThrow(EThrowableKind::Grenade, 0.0); // grenade 3 -> 2
        int f = 0, g = 0, m = 0;
        L.SerializeAmmo(f, g, m);
        Check(f == 2 && g == 2 && m == 2, "serialize captures current ammo");
        FPlayerOffenseLoadout L2;
        L2.RestoreAmmo(f, g, m);
        Check(L2.GetAmmo(EThrowableKind::Grenade) == 2, "restore round-trips grenade");
        // Out-of-range restore clamps to [0, cap]: grenade cap 5, molotov cap 3.
        L2.RestoreAmmo(-5, 99, 3);
        Check(L2.GetAmmo(EThrowableKind::Flashbang) == 0, "restore clamps negative to 0");
        Check(L2.GetAmmo(EThrowableKind::Grenade) == 5, "restore clamps over-cap to 5");
        Check(L2.GetAmmo(EThrowableKind::Molotov) == 3, "restore keeps at-cap");
    }

    if (g_failures == 0)
    {
        std::printf("=== offense_loadout_verify PASSED ===\n");
        return 0;
    }
    std::printf("=== offense_loadout_verify FAILED (%d) ===\n", g_failures);
    return 1;
}
