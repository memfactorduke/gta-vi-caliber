// Copyright Epic Games, Inc. All Rights Reserved.

#include "TideModel.h"

namespace
{
    // Phase (radians) of the semidiurnal term at a given hour: 0 at a high tide.
    double SemidiurnalPhase(double Hours, double PeriodHours, double PhaseHours)
    {
        const double Omega = 2.0 * PI / PeriodHours;
        return Omega * (Hours - PhaseHours);
    }
}

double FTideModel::AmplitudeAt(double Hours) const
{
    if (Params.SpringNeapHours <= 0.0)
    {
        return FMath::Max(0.0, Params.MeanAmplitude);
    }
    const double Omega = 2.0 * PI / Params.SpringNeapHours;
    const double Env = Params.MeanAmplitude
        + Params.SpringNeapRange * FMath::Cos(Omega * (Hours - Params.SpringNeapPhaseHours));
    return FMath::Max(0.0, Env);
}

double FTideModel::LevelAt(double Hours) const
{
    if (Params.PeriodHours <= 0.0)
    {
        return Params.MeanLevel; // no semidiurnal swing -> flat at the datum
    }
    const double Theta = SemidiurnalPhase(Hours, Params.PeriodHours, Params.PhaseHours);
    return Params.MeanLevel + AmplitudeAt(Hours) * FMath::Cos(Theta);
}

double FTideModel::NormalizedAt(double Hours) const
{
    // A fully-cancelled spring-neap envelope cancels to a tiny positive float residual
    // (~1e-7, not exactly 0), so guard against a small epsilon rather than 0.0 — this
    // honours the documented "0.5 when the envelope is cancelled" contract.
    if (Params.PeriodHours <= 0.0 || AmplitudeAt(Hours) <= KINDA_SMALL_NUMBER)
    {
        return 0.5;
    }
    // (cos + 1) / 2 maps the swing into 0..1 independent of the envelope size.
    const double Theta = SemidiurnalPhase(Hours, Params.PeriodHours, Params.PhaseHours);
    return FMath::Clamp(0.5 * (FMath::Cos(Theta) + 1.0), 0.0, 1.0);
}

bool FTideModel::IsRisingAt(double Hours) const
{
    if (Params.PeriodHours <= 0.0)
    {
        return false;
    }
    // Central finite difference: provably correct by construction (no hand-derived
    // derivative to get wrong), and the envelope varies far too slowly to matter here.
    const double H = 1e-3; // hours
    return LevelAt(Hours + H) > LevelAt(Hours - H);
}

double FTideModel::TimeToPhase(double Hours, double TargetRadians) const
{
    if (Params.PeriodHours <= 0.0)
    {
        return 0.0;
    }
    const double Omega = 2.0 * PI / Params.PeriodHours;
    const double Theta = SemidiurnalPhase(Hours, Params.PeriodHours, Params.PhaseHours);

    // Smallest forward phase advance that lands on TargetRadians (mod 2pi), in (0, 2pi].
    const double TwoPi = 2.0 * PI;
    double Delta = FMath::Fmod(TargetRadians - Theta, TwoPi);
    if (Delta <= 0.0)
    {
        Delta += TwoPi; // being exactly on the target means the NEXT one, a full turn away
    }
    return Delta / Omega;
}

double FTideModel::TimeToNextHigh(double Hours) const
{
    // High tide is the semidiurnal peak: cos = 1, i.e. phase == 0 (mod 2pi).
    return TimeToPhase(Hours, 0.0);
}

double FTideModel::TimeToNextLow(double Hours) const
{
    // Low tide is the trough: cos = -1, i.e. phase == pi (mod 2pi).
    return TimeToPhase(Hours, PI);
}
