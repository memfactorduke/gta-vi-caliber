// Out-of-tree oracle for FTrafficLane — compiles + runs the REAL TrafficLane.cpp
// (and FTrafficModel.cpp for the queue pairing) and re-asserts GTC.AI.Traffic.Lane.
#include "../../../Source/GTC_UE5/AI/Traffic/TrafficLane.h"
#include "../../../Source/GTC_UE5/Worldcore/TrafficModel.h"
#include "harness.h"

int main()
{
    using FAgent = FTrafficLane::FAgent;

    TArray<FAgent> Cars;
    Cars.Add(FAgent{0.0, 10.0, 4.5});
    Cars.Add(FAgent{8.0, 10.0, 4.5});
    Cars.Add(FAgent{30.0, 10.0, 4.5});

    Check(FTrafficLane::LeaderIndex(Cars, 0) == 1, "car0 leader is car1");
    Check(FTrafficLane::LeaderIndex(Cars, 1) == 2, "car1 leader is car2");
    Check(FTrafficLane::LeaderIndex(Cars, 2) == -1, "front car has no leader");

    CheckNear(FTrafficLane::BumperGap(Cars[0], Cars[1]), 3.5, "gap car0->car1");
    CheckNear(FTrafficLane::BumperGap(Cars[1], Cars[2]), 17.5, "gap car1->car2");

    Check(!FTrafficLane::Overlaps(Cars[0], Cars[1]), "queued cars do not overlap");
    Check(FTrafficLane::Overlaps(FAgent{0.0, 0.0, 4.5}, FAgent{4.0, 0.0, 4.5}), "cars 4m apart overlap");

    Check(FTrafficLane::LeaderIndex(Cars, 9) == -1, "bad index has no leader");

    {
        TArray<FAgent> Tie;
        Tie.Add(FAgent{0.0, 0.0, 4.5});
        Tie.Add(FAgent{5.0, 0.0, 4.5});
        Tie.Add(FAgent{5.0, 0.0, 4.5});
        Check(FTrafficLane::LeaderIndex(Tie, 0) == 1, "tie picks lower index");
    }

    {
        const int32 Lead0 = FTrafficLane::LeaderIndex(Cars, 0);
        const double Gap0 = FTrafficLane::BumperGap(Cars[0], Cars[Lead0]);
        const double FollowAccel = FTrafficModel::CarFollowingAccel(
            Cars[0].Speed, Gap0, Cars[Lead0].Speed, 14.0, 1.5, 2.0, 2.0, 1.5);
        Check(FollowAccel < 0.0, "tight follower brakes");

        const double FreeAccel = FTrafficModel::CarFollowingAccel(
            Cars[2].Speed, 1.0e6, 14.0, 14.0, 1.5, 2.0, 2.0, 1.5);
        Check(FreeAccel > 0.0, "front car accelerates on open road");
    }

    return OracleSummary("FTrafficLane");
}
