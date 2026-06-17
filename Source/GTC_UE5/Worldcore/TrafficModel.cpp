// Copyright (c) 2026 GTC contributors

#include "TrafficModel.h"

#include <algorithm>
#include <cmath>

double FTrafficModel::CarFollowingAccel(double Speed, double Gap, double LeaderSpeed,
    double DesiredSpeed, double MaxAccel, double ComfortDecel, double MinGap,
    double TimeHeadway)
{
    const double V0 = DesiredSpeed > 1e-6 ? DesiredSpeed : 1e-6;
    // Free-road term: ease toward desired speed (quartic -> soft approach).
    const double FreeTerm = 1.0 - std::pow(Speed / V0, 4.0);

    // Interaction term: desired dynamic gap s* versus the actual gap.
    double Interaction = 0.0;
    if (Gap > 1e-6)
    {
        // Guard the product before sqrt: a negative param (reachable only via a
        // raw core call — the class setters reject negatives) would otherwise be
        // sqrt(negative) = NaN (Codex review).
        const double BrakeProduct = MaxAccel * ComfortDecel;
        const double BrakeDenom = BrakeProduct > 1e-12 ? 2.0 * std::sqrt(BrakeProduct) : 0.0;
        const double Approach = BrakeDenom > 1e-9 ? Speed * (Speed - LeaderSpeed) / BrakeDenom : 0.0;
        const double SStar = MinGap + std::max(0.0, Speed * TimeHeadway + Approach);
        const double Ratio = SStar / Gap;
        Interaction = Ratio * Ratio;
    }
    else
    {
        Interaction = 1.0e6; // touching/overlapping the leader -> brake hard
    }

    return MaxAccel * (FreeTerm - Interaction);
}
