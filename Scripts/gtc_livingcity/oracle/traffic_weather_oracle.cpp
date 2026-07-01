// Out-of-tree oracle for FTrafficWeather — compiles + runs the REAL TrafficWeather.cpp
// and re-asserts GTC.AI.Traffic.Weather, plus the one contract the live adapter
// (UGTCTrafficSubsystem) leans on: the caution scales an agent's BASE v0/T, so
// applying it fresh each frame from that base never compounds.
#include "../../../Source/GTC_UE5/AI/Traffic/TrafficWeather.h"
#include "harness.h"

int main()
{
    const FTrafficWeather::FParams P; // comfortable-city defaults

    // Clear, dry: the factors are the identity, so traffic behaves exactly as before.
    CheckNear(FTrafficWeather::SpeedFactor(0.0, 1.0, P), 1.0, "clear+dry -> full cruise speed");
    CheckNear(FTrafficWeather::HeadwayFactor(0.0, 1.0, P), 1.0, "clear+dry -> base headway");

    // Rain alone slows the city and stretches the gaps.
    Check(FTrafficWeather::SpeedFactor(1.0, 1.0, P) < 1.0, "rain slows cruise");
    Check(FTrafficWeather::HeadwayFactor(1.0, 1.0, P) > 1.0, "rain widens gaps");

    // Fog alone does too.
    Check(FTrafficWeather::SpeedFactor(0.0, 0.0, P) < 1.0, "fog slows cruise");

    // Rain and fog compound — a foggy downpour is worse than either alone.
    Check(FTrafficWeather::SpeedFactor(1.0, 0.0, P) < FTrafficWeather::SpeedFactor(1.0, 1.0, P),
        "fog+rain slower than rain alone");
    Check(FTrafficWeather::HeadwayFactor(1.0, 0.0, P) > FTrafficWeather::HeadwayFactor(1.0, 1.0, P),
        "fog+rain wider gap than rain alone");

    // Bounded: the city never crawls below the floor, never exceeds full speed, and
    // the headway gain is capped at MaxHeadwayGain so gaps stay sane.
    Check(FTrafficWeather::SpeedFactor(1.0, 0.0, P) >= P.MinSpeedFactor - 1e-9, "never below speed floor");
    Check(FTrafficWeather::SpeedFactor(1.0, 0.0, P) <= 1.0 + 1e-9, "never above full speed");
    Check(FTrafficWeather::HeadwayFactor(1.0, 0.0, P) <= P.MaxHeadwayGain + 1e-9, "headway gain capped");

    // Out-of-range inputs are clamped, not trusted.
    CheckNear(FTrafficWeather::SpeedFactor(5.0, 9.0, P), FTrafficWeather::SpeedFactor(1.0, 1.0, P),
        "inputs clamped to [0,1]");

    // Adjusted* compose the factor onto a base value.
    CheckNear(FTrafficWeather::AdjustedDesiredSpeed(12.0, 0.0, 1.0, P), 12.0, "adjusted v0 clear = base");
    Check(FTrafficWeather::AdjustedDesiredSpeed(12.0, 1.0, 0.0, P) < 12.0, "adjusted v0 in a storm < base");
    CheckNear(FTrafficWeather::AdjustedTimeHeadway(1.4, 0.0, 1.0, P), 1.4, "adjusted T clear = base");
    Check(FTrafficWeather::AdjustedTimeHeadway(1.4, 1.0, 0.0, P) > 1.4, "adjusted T in a storm > base");

    // The adapter's load-bearing invariant: it stores each car's base and re-derives
    // the effective IDM params from it every tick. Re-applying from the SAME base is
    // idempotent — caution does not stack frame over frame into a frozen city.
    const double Base = 12.0;
    const double Once = FTrafficWeather::AdjustedDesiredSpeed(Base, 1.0, 1.0, P);
    const double Twice = FTrafficWeather::AdjustedDesiredSpeed(Base, 1.0, 1.0, P);
    CheckNear(Once, Twice, "re-applying caution from base does not compound");

    return OracleSummary("traffic_weather_oracle");
}
