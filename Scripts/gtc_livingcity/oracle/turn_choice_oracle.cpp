// Out-of-tree oracle for FTurnChoice — compiles + runs the REAL TurnChoice.cpp
// and re-asserts GTC.AI.Traffic.Turn.
#include "../../../Source/GTC_UE5/AI/Traffic/TurnChoice.h"
#include "harness.h"

int main()
{
    using ETurn = FTurnChoice::ETurn;
    const FVector X(1, 0, 0), Z(0, 0, 1), NX(-1, 0, 0), NZ(0, 0, -1);
    const double HalfPi = 1.5707963267948966;

    Check(FTurnChoice::Classify(X, X) == ETurn::Straight, "same heading is straight");
    Check(FTurnChoice::Classify(X, Z) == ETurn::Left, "+X -> +Z is a left turn");
    Check(FTurnChoice::Classify(X, NZ) == ETurn::Right, "+X -> -Z is a right turn");
    Check(FTurnChoice::Classify(X, NX) == ETurn::UTurn, "reversal is a U-turn");

    CheckNear(FTurnChoice::SignedTurn(X, Z), HalfPi, "left turn is +pi/2");
    CheckNear(FTurnChoice::SignedTurn(X, NZ), -HalfPi, "right turn is -pi/2");

    Check(FTurnChoice::ChooseByRoute(TArray<int32>({5, 7, 9}), 7) == 1, "route picks the lane to the next node");
    Check(FTurnChoice::ChooseByRoute(TArray<int32>({5, 7, 9}), 3) == -1, "route absent -> -1");

    {
        const TArray<FVector> Outs({Z, X, NX});
        Check(FTurnChoice::ChooseStraightest(X, Outs, false) == 1, "straightest continuation is +X");
    }
    Check(FTurnChoice::ChooseStraightest(X, TArray<FVector>({NX}), false) == -1, "only a U-turn available -> none");
    Check(FTurnChoice::ChooseStraightest(X, TArray<FVector>({NX}), true) == 0, "U-turn allowed when permitted");
    Check(FTurnChoice::ChooseStraightest(FVector::ZeroVector, TArray<FVector>({X}), true) == -1, "degenerate incoming -> none");

    return OracleSummary("FTurnChoice");
}
