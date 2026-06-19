// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ReactionState.h"
#include "../NpcReaction.h"

/**
 * Tests for FReactionState — the rising-edge + hysteresis FSM over FNpcReaction.
 * GTC-original. Pins instant escalation (with a rising edge), held de-escalation,
 * and the FNpcReaction verb -> reaction bridge the adapter uses.
 */

namespace
{
    using EReaction = FReactionState::EReaction;

    // The mapping the live adapter applies at the boundary.
    EReaction FromVerb(const FString& Verb)
    {
        if (Verb == TEXT("flee")) { return EReaction::Flee; }
        if (Verb == TEXT("gawk")) { return EReaction::Gawk; }
        return EReaction::Ignore;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FReactionStateTest,
    "GTC.NPC.Decision.ReactionState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReactionStateTest::RunTest(const FString& Parameters)
{
    // Escalation is instant and fires a rising edge.
    {
        FReactionState R;
        TestFalse(TEXT("starts calm"), R.IsReacting());
        const FReactionState::FTick T = R.Update(EReaction::Flee, 0.1);
        TestTrue(TEXT("escalates to flee"), T.State == EReaction::Flee);
        TestTrue(TEXT("escalation is a rising edge"), T.bRisingEdge);
        TestTrue(TEXT("now reacting"), R.IsReacting());

        // Sustained threat holds without re-firing the edge.
        const FReactionState::FTick T2 = R.Update(EReaction::Flee, 0.1);
        TestTrue(TEXT("stays fleeing"), T2.State == EReaction::Flee);
        TestFalse(TEXT("no repeat edge"), T2.bRisingEdge);

        // Threat clears, but the flee holds out its cooldown.
        const FReactionState::FTick T3 = R.Update(EReaction::Ignore, 0.1);
        TestTrue(TEXT("flee holds through cooldown"), T3.State == EReaction::Flee);

        // After the flee hold elapses it steps down one level, no edge.
        const FReactionState::FTick T4 = R.Update(EReaction::Ignore, FReactionState::FleeHoldSeconds);
        TestTrue(TEXT("decays flee -> gawk"), T4.State == EReaction::Gawk);
        TestFalse(TEXT("de-escalation is not an edge"), T4.bRisingEdge);

        // Re-escalation during the wary phase fires a fresh edge.
        const FReactionState::FTick T5 = R.Update(EReaction::Flee, 0.1);
        TestTrue(TEXT("re-escalates to flee"), T5.State == EReaction::Flee);
        TestTrue(TEXT("re-escalation is a rising edge"), T5.bRisingEdge);
    }

    // Full decay gawk -> ignore once interest lapses.
    {
        FReactionState R;
        TestTrue(TEXT("gawk edge"), R.Update(EReaction::Gawk, 0.1).bRisingEdge);
        TestTrue(TEXT("gawk holds briefly"), R.Update(EReaction::Ignore, 0.5).State == EReaction::Gawk);
        const FReactionState::FTick T = R.Update(EReaction::Ignore, 0.6); // total 1.1 >= GawkHold
        TestTrue(TEXT("gawk decays to ignore"), T.State == EReaction::Ignore);
        TestFalse(TEXT("no longer reacting"), R.IsReacting());
    }

    // De-escalation never overshoots below what's still desired.
    {
        FReactionState R;
        R.Update(EReaction::Flee, 0.1);
        const FReactionState::FTick T = R.Update(EReaction::Gawk, FReactionState::FleeHoldSeconds);
        TestTrue(TEXT("flee eases to the still-desired gawk, not ignore"), T.State == EReaction::Gawk);
    }

    // The FNpcReaction bridge: a scared citizen's verb drives a flee with an edge.
    {
        FReactionState R;
        const FString Verb = FNpcReaction::Decide(8.0, 0.9, 0.1, 0.0); // "flee"
        const FReactionState::FTick T = R.Update(FromVerb(Verb), 0.1);
        TestTrue(TEXT("FNpcReaction flee verb escalates"), T.State == EReaction::Flee);
        TestTrue(TEXT("FNpcReaction flee fires an edge"), T.bRisingEdge);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
