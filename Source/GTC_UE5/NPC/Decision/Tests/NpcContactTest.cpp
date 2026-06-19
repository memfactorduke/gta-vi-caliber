// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcContact.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FNpcContact — the close-range "the player just touched me"
 * decision model. Severity bands and the fight/flight branching are asserted
 * exactly so a tuning change that would, say, let a walk-by floor a pedestrian
 * trips the suite. Prefix GTC.NPC.Decision.NpcContact.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcContactTest,
    "GTC.NPC.Decision.NpcContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcContactTest::RunTest(const FString& Parameters)
{
    using R = ENpcContactReaction;

    // --- Severity ---------------------------------------------------------------

    // A strike is serious even from a standstill, and never below its floor.
    TestTrue(TEXT("standstill strike is serious"),
        FNpcContact::Severity(0.0, true) >= FNpcContact::StrikeBase - GtcTest::Eps);
    // A fast strike reaches a flooring blow.
    TestTrue(TEXT("fast strike floors"),
        FNpcContact::Severity(FNpcContact::StrikeSwingSpeed, true) >= FNpcContact::FloorMin);

    // A bare body bump can stagger but NEVER floor — capped below the knockdown band.
    TestTrue(TEXT("body contact capped below floor"),
        FNpcContact::Severity(1000.0, false) <= FNpcContact::BodyMax + GtcTest::Eps);
    TestTrue(TEXT("body contact never reaches knockdown"),
        FNpcContact::Severity(1000.0, false) < FNpcContact::FloorMin);

    // A gentle brush is a graze; a brisk barge is a hard contact.
    TestTrue(TEXT("slow touch is a graze"),
        FNpcContact::Severity(1.0, false) <= FNpcContact::GrazeMax);
    TestTrue(TEXT("brisk barge is hard"),
        FNpcContact::Severity(10.0, false) > FNpcContact::BumpMax);

    // Approach speed is clamped non-negative (a receding player can't "hit").
    TestEqual(TEXT("negative approach is zero severity"),
        FNpcContact::Severity(-5.0, false), 0.0);

    // --- Decide: light contact --------------------------------------------------

    // A calm citizen brushed = polite excuse; bumped = annoyed.
    TestEqual(TEXT("calm graze -> excuse"), FNpcContact::Decide(0.05, 0.5, 0.0), R::Excuse);
    TestEqual(TEXT("calm bump -> annoyed"), FNpcContact::Decide(0.25, 0.5, 0.0), R::Annoyed);

    // A rattled (already-scared) citizen flinches at the same light contact.
    TestEqual(TEXT("rattled graze -> flinch"), FNpcContact::Decide(0.05, 0.5, 0.9), R::Flinch);
    TestEqual(TEXT("rattled bump -> flinch"), FNpcContact::Decide(0.25, 0.5, 0.9), R::Flinch);

    // --- Decide: serious contact, nerve decides fight or flight -----------------

    // Brave squares up to a shove or a hit.
    TestEqual(TEXT("brave + hard shove -> confront"), FNpcContact::Decide(0.5, 0.9, 0.0), R::Confront);
    TestEqual(TEXT("brave + hard hit -> confront"), FNpcContact::Decide(0.8, 0.9, 0.0), R::Confront);

    // Timid recoils from a moderate shove, but breaks and runs from a hard hit.
    TestEqual(TEXT("timid + moderate shove -> flinch"), FNpcContact::Decide(0.5, 0.2, 0.0), R::Flinch);
    TestEqual(TEXT("timid + hard hit -> flee"), FNpcContact::Decide(0.8, 0.2, 0.0), R::Flee);

    // --- Decide: a flooring blow takes anyone down ------------------------------

    TestEqual(TEXT("floor force knocks down the brave"), FNpcContact::Decide(0.95, 0.95, 0.0), R::Knockdown);
    TestEqual(TEXT("floor force knocks down the timid"), FNpcContact::Decide(0.95, 0.05, 0.0), R::Knockdown);

    // Decide never returns the resting Ignore value.
    TestNotEqual(TEXT("decide is never ignore (low)"), FNpcContact::Decide(0.0, 0.5, 0.0), R::Ignore);
    TestNotEqual(TEXT("decide is never ignore (high)"), FNpcContact::Decide(1.0, 0.5, 1.0), R::Ignore);

    // --- Classification helpers -------------------------------------------------

    TestTrue(TEXT("confront is hostile"), FNpcContact::IsHostile(R::Confront));
    TestFalse(TEXT("flee is not hostile"), FNpcContact::IsHostile(R::Flee));
    TestTrue(TEXT("flee is panic"), FNpcContact::IsPanic(R::Flee));
    TestTrue(TEXT("knockdown is panic"), FNpcContact::IsPanic(R::Knockdown));
    TestFalse(TEXT("annoyed is not panic"), FNpcContact::IsPanic(R::Annoyed));
    TestTrue(TEXT("knockdown is knockdown"), FNpcContact::IsKnockdown(R::Knockdown));
    TestFalse(TEXT("ignore does not react"), FNpcContact::Reacts(R::Ignore));
    TestTrue(TEXT("excuse reacts"), FNpcContact::Reacts(R::Excuse));

    // --- Physical consequences --------------------------------------------------

    // Only a knockdown (big) and a flinch (small recoil) impart knockback.
    TestTrue(TEXT("knockdown launches hard"), FNpcContact::Knockback(R::Knockdown, 1.0) >= 700.0);
    TestTrue(TEXT("flinch recoils gently"),
        FNpcContact::Knockback(R::Flinch, 1.0) > 0.0 && FNpcContact::Knockback(R::Flinch, 1.0) < 200.0);
    TestEqual(TEXT("confront has no knockback"), FNpcContact::Knockback(R::Confront, 1.0), 0.0);
    TestEqual(TEXT("excuse has no knockback"), FNpcContact::Knockback(R::Excuse, 1.0), 0.0);

    // Memory: a flooring blow is unforgettable; a polite brush leaves no mark.
    TestTrue(TEXT("knockdown is memorable"), FNpcContact::MemoryBump(R::Knockdown, 1.0) >= 0.9);
    TestEqual(TEXT("excuse leaves no memory"), FNpcContact::MemoryBump(R::Excuse, 1.0), 0.0);
    TestTrue(TEXT("flee is memorable enough to recognise"),
        FNpcContact::MemoryBump(R::Flee, 0.5) > 0.25); // above NpcMemory "uneasy"

    // Durations: every real reaction lingers a positive beat; only the resting
    // state is zero. (A dropped switch case would return 0 and expire instantly.)
    const R RealReactions[] = { R::Excuse, R::Annoyed, R::Flinch, R::Confront, R::Flee, R::Knockdown };
    for (const R Rr : RealReactions)
    {
        TestTrue(TEXT("real reaction lingers"), FNpcContact::Duration(Rr) > 0.0);
    }
    TestEqual(TEXT("ignore has no duration"), FNpcContact::Duration(R::Ignore), 0.0);

    // --- Exact band boundaries: lock the determinism contract ------------------
    // The header promises a tuning change "that would let a walk-by floor a
    // pedestrian trips the suite" — so pin each comparator at its edge (an
    // inclusive<->exclusive flip changes the verdict only AT the boundary, which
    // the interior-point tests above would miss). Uses named constants so a retune
    // moves the test with the intent.

    // FloorMin (>=): the knockdown floor is inclusive; one epsilon below is not.
    TestEqual(TEXT("at FloorMin -> knockdown"),
        FNpcContact::Decide(FNpcContact::FloorMin, 0.05, 0.0), R::Knockdown);
    TestNotEqual(TEXT("just below FloorMin -> not knockdown"),
        FNpcContact::Decide(FNpcContact::FloorMin - GtcTest::Eps, 0.9, 0.0), R::Knockdown);

    // GrazeMax / BumpMax (<=): calm light-contact edges.
    TestEqual(TEXT("at GrazeMax -> excuse"),
        FNpcContact::Decide(FNpcContact::GrazeMax, 0.5, 0.0), R::Excuse);
    TestEqual(TEXT("just past GrazeMax -> annoyed"),
        FNpcContact::Decide(FNpcContact::GrazeMax + GtcTest::Eps, 0.5, 0.0), R::Annoyed);
    TestEqual(TEXT("at BumpMax -> annoyed"),
        FNpcContact::Decide(FNpcContact::BumpMax, 0.5, 0.0), R::Annoyed);
    TestEqual(TEXT("just past BumpMax (timid) -> flinch"),
        FNpcContact::Decide(FNpcContact::BumpMax + GtcTest::Eps, 0.2, 0.0), R::Flinch);

    // HardMax (<=): the timid flinch/flee crossover — the model's headline edge.
    TestEqual(TEXT("timid at HardMax -> flinch"),
        FNpcContact::Decide(FNpcContact::HardMax, 0.2, 0.0), R::Flinch);
    TestEqual(TEXT("timid just past HardMax -> flee"),
        FNpcContact::Decide(FNpcContact::HardMax + GtcTest::Eps, 0.2, 0.0), R::Flee);

    // ConfrontNerve (>=): the fight/flight pivot on a serious-but-not-floor shove.
    TestEqual(TEXT("nerve at ConfrontNerve -> confront"),
        FNpcContact::Decide(0.5, FNpcContact::ConfrontNerve, 0.0), R::Confront);
    TestEqual(TEXT("nerve just below ConfrontNerve -> flinch"),
        FNpcContact::Decide(0.5, FNpcContact::ConfrontNerve - GtcTest::Eps, 0.0), R::Flinch);

    // RattledThreshold (>=): a jumpy citizen flinches at a brush; a calmer one excuses.
    TestEqual(TEXT("alarm at RattledThreshold -> flinch on graze"),
        FNpcContact::Decide(0.05, 0.5, FNpcContact::RattledThreshold), R::Flinch);
    TestEqual(TEXT("alarm just below RattledThreshold -> excuse on graze"),
        FNpcContact::Decide(0.05, 0.5, FNpcContact::RattledThreshold - GtcTest::Eps), R::Excuse);

    // --- Coupled Severity -> Decide: the body-bump fight/flight split is reachable
    // end to end (not just at hand-fed severities). A full-force barge on a timid
    // pedestrian flees; a mid barge merely flinches.
    TestEqual(TEXT("timid hard body barge -> flee (coupled)"),
        FNpcContact::Decide(FNpcContact::Severity(1000.0, false), 0.2, 0.0), R::Flee);
    TestEqual(TEXT("timid mid body bump -> flinch (coupled)"),
        FNpcContact::Decide(FNpcContact::Severity(8.0, false), 0.2, 0.0), R::Flinch);

    // Over-speed strike stays clamped to [.., 1.0] and still floors.
    TestTrue(TEXT("overspeed strike capped at 1.0"),
        FNpcContact::Severity(100.0, true) <= 1.0 + GtcTest::Eps);
    TestTrue(TEXT("overspeed strike still floors"),
        FNpcContact::Severity(100.0, true) >= FNpcContact::FloorMin);

    // --- Knockback / memory scale with severity (the * S terms) -----------------
    TestEqual(TEXT("flinch knockback zero at zero severity"),
        FNpcContact::Knockback(R::Flinch, 0.0), 0.0);
    TestTrue(TEXT("knockdown knockback scales with severity"),
        FNpcContact::Knockback(R::Knockdown, 1.0) > FNpcContact::Knockback(R::Knockdown, 0.0));
    TestTrue(TEXT("knockdown memory scales with severity"),
        FNpcContact::MemoryBump(R::Knockdown, 1.0) > FNpcContact::MemoryBump(R::Knockdown, 0.0));
    // Cover the otherwise-unasserted Confront / Flinch / Annoyed memory arms by ordering.
    TestTrue(TEXT("confront memory above annoyed"),
        FNpcContact::MemoryBump(R::Confront, 0.5) > FNpcContact::MemoryBump(R::Annoyed, 0.5));
    TestTrue(TEXT("flinch memory above annoyed"),
        FNpcContact::MemoryBump(R::Flinch, 0.5) > FNpcContact::MemoryBump(R::Annoyed, 0.5));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
