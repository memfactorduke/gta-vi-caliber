// Out-of-tree oracle for FReactionState — compiles + runs the REAL
// ReactionState.cpp and re-asserts the FSM cases of GTC.NPC.Decision.ReactionState
// (the enum FSM; the FNpcReaction verb bridge is exercised in the UE test only).
#include "../../../Source/GTC_UE5/NPC/Decision/ReactionState.h"
#include "harness.h"

using EReaction = FReactionState::EReaction;

int main()
{
    {
        FReactionState R;
        Check(!R.IsReacting(), "starts calm");
        const FReactionState::FTick T = R.Update(EReaction::Flee, 0.1);
        Check(T.State == EReaction::Flee, "escalates to flee");
        Check(T.bRisingEdge, "escalation is a rising edge");
        Check(R.IsReacting(), "now reacting");

        const FReactionState::FTick T2 = R.Update(EReaction::Flee, 0.1);
        Check(T2.State == EReaction::Flee, "stays fleeing");
        Check(!T2.bRisingEdge, "no repeat edge");

        const FReactionState::FTick T3 = R.Update(EReaction::Ignore, 0.1);
        Check(T3.State == EReaction::Flee, "flee holds through cooldown");

        const FReactionState::FTick T4 = R.Update(EReaction::Ignore, FReactionState::FleeHoldSeconds);
        Check(T4.State == EReaction::Gawk, "decays flee -> gawk");
        Check(!T4.bRisingEdge, "de-escalation is not an edge");

        const FReactionState::FTick T5 = R.Update(EReaction::Flee, 0.1);
        Check(T5.State == EReaction::Flee, "re-escalates to flee");
        Check(T5.bRisingEdge, "re-escalation is a rising edge");
    }

    {
        FReactionState R;
        Check(R.Update(EReaction::Gawk, 0.1).bRisingEdge, "gawk edge");
        Check(R.Update(EReaction::Ignore, 0.5).State == EReaction::Gawk, "gawk holds briefly");
        const FReactionState::FTick T = R.Update(EReaction::Ignore, 0.6);
        Check(T.State == EReaction::Ignore, "gawk decays to ignore");
        Check(!R.IsReacting(), "no longer reacting");
    }

    {
        FReactionState R;
        R.Update(EReaction::Flee, 0.1);
        const FReactionState::FTick T = R.Update(EReaction::Gawk, FReactionState::FleeHoldSeconds);
        Check(T.State == EReaction::Gawk, "flee eases to the still-desired gawk, not ignore");
    }

    return OracleSummary("FReactionState");
}
