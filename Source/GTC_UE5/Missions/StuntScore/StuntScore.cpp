// Copyright (c) 2026 GTC contributors

#include "StuntScore.h"

const TArray<FStuntScore::FTrickPoints>& FStuntScore::TrickPoints()
{
    // Insertion-ordered mirror of the Godot TRICK_POINTS Dictionary.
    static const TArray<FTrickPoints> Table = {
        { TEXT("jump"), 50.0 },
        { TEXT("flip"), 250.0 },
        { TEXT("spin"), 150.0 },
        { TEXT("near_miss"), 100.0 },
        { TEXT("wheelie"), 40.0 },
    };
    return Table;
}

TArray<FString> FStuntScore::TrickKinds() const
{
    TArray<FString> Kinds;
    Kinds.Reserve(TrickPoints().Num());
    for (const FTrickPoints& Row : TrickPoints())
    {
        Kinds.Add(Row.Kind);
    }
    return Kinds;
}

int32 FStuntScore::AddTrick(const FString& Kind, double Magnitude)
{
    const FTrickPoints* Row = TrickPoints().FindByPredicate(
        [&Kind](const FTrickPoints& R) { return R.Kind == Kind; });
    if (Row == nullptr || Magnitude <= 0.0)
    {
        return 0;
    }
    const int32 Points = FMath::RoundToInt32(Row->Points * Magnitude);
    _ComboPoints += static_cast<double>(Points);
    _ComboCount += 1;
    return Points;
}

double FStuntScore::Multiplier() const
{
    if (_ComboCount <= 0)
    {
        return 1.0;
    }
    return FMath::Min(1.0 + static_cast<double>(_ComboCount - 1) * MultStep, MaxMult);
}

int32 FStuntScore::PendingScore() const
{
    return FMath::RoundToInt32(_ComboPoints * Multiplier());
}

int32 FStuntScore::Bank()
{
    const int32 Banked = PendingScore();
    _Total += Banked;
    _Best = FMath::Max(_Best, Banked);
    ResetCombo();
    return Banked;
}

int32 FStuntScore::Wipe()
{
    const int32 Lost = PendingScore();
    ResetCombo();
    return Lost;
}

int32 FStuntScore::CashFor(int32 Score)
{
    return FMath::Max(0, Score);
}

int32 FStuntScore::RespectFor(int32 Score)
{
    return FMath::RoundToInt32(FMath::Max(0.0, static_cast<double>(Score)) * 0.1);
}

void FStuntScore::ResetCombo()
{
    _ComboPoints = 0.0;
    _ComboCount = 0;
}
