// Copyright (c) 2026 GTC contributors

#include "SoundPropagation.h"

double FSoundPropagation::BaseLoudness(ESoundKind Kind)
{
    switch (Kind)
    {
    case ESoundKind::Explosion:
        return 1.0;
    case ESoundKind::Gunshot:
        return 0.85;
    case ESoundKind::Alarm:
        return 0.7;
    case ESoundKind::CarHorn:
        return 0.55;
    case ESoundKind::GlassBreak:
        return 0.5;
    case ESoundKind::Shout:
        return 0.45;
    case ESoundKind::Engine:
        return 0.35;
    case ESoundKind::SilencedShot:
        return 0.2;
    case ESoundKind::Footstep:
        return 0.12;
    default:
        return 0.0;
    }
}

double FSoundPropagation::BaseLoudnessForInt(int32 KindRaw)
{
    // Mirror Godot's match-on-int: only the defined ordinals map; all else -> 0.0.
    if (KindRaw < static_cast<int32>(ESoundKind::Gunshot)
        || KindRaw > static_cast<int32>(ESoundKind::GlassBreak))
    {
        return 0.0;
    }
    return BaseLoudness(static_cast<ESoundKind>(KindRaw));
}

bool FSoundPropagation::IsAlarming(ESoundKind Kind)
{
    switch (Kind)
    {
    case ESoundKind::Gunshot:
    case ESoundKind::Explosion:
    case ESoundKind::Alarm:
    case ESoundKind::GlassBreak:
    case ESoundKind::Shout:
        return true;
    default:
        return false;
    }
}

double FSoundPropagation::PerceivedIntensity(
    const FVector& SourcePos, const FVector& ListenerPos, double Loudness, double Ambient)
{
    if (Loudness <= 0.0)
    {
        return 0.0;
    }
    const double Dx = SourcePos.X - ListenerPos.X;
    const double Dz = SourcePos.Z - ListenerPos.Z;
    const double DistSq = Dx * Dx + Dz * Dz;
    const double Attenuated = Loudness * FalloffReferenceSq / (FalloffReferenceSq + DistSq);
    return FMath::Clamp(Attenuated - FMath::Max(Ambient, 0.0), 0.0, 1.0);
}

bool FSoundPropagation::IsAudible(
    const FVector& SourcePos,
    const FVector& ListenerPos,
    double Loudness,
    double Ambient,
    double Threshold)
{
    return PerceivedIntensity(SourcePos, ListenerPos, Loudness, Ambient) >= Threshold;
}

double FSoundPropagation::AudibleRadius(double Loudness, double HearingFloor)
{
    if (Loudness <= 0.0 || HearingFloor <= 0.0 || Loudness <= HearingFloor)
    {
        return 0.0;
    }
    return FMath::Sqrt(FalloffReferenceSq * (Loudness / HearingFloor - 1.0));
}

ESoundReaction FSoundPropagation::ReactionFor(double Intensity, bool bAlarming)
{
    if (Intensity < NoticedAt)
    {
        return ESoundReaction::Unheard;
    }
    const double AlarmedAt = bAlarming ? AlarmedAtAlarming : AlarmedAtAmbient;
    if (Intensity >= AlarmedAt)
    {
        return ESoundReaction::Alarmed;
    }
    return ESoundReaction::Noticed;
}

TArray<FSoundPropagation::FListenerResult> FSoundPropagation::Emit(
    const FVector& SourcePos, ESoundKind Kind, const TArray<FListener>& Listeners, double Ambient)
{
    const double Loudness = BaseLoudness(Kind);
    const bool bAlarming = IsAlarming(Kind);

    TArray<FListenerResult> Out;
    Out.Reserve(Listeners.Num());
    for (const FListener& Listener : Listeners)
    {
        const double Intensity = PerceivedIntensity(SourcePos, Listener.Pos, Loudness, Ambient);
        const ESoundReaction Reaction = ReactionFor(Intensity, bAlarming);

        FListenerResult Result;
        Result.Pos = Listener.Pos;
        Result.Id = Listener.Id;
        Result.Intensity = Intensity;
        Result.Reaction = Reaction;
        Result.bHeard = Reaction != ESoundReaction::Unheard;
        Out.Add(Result);
    }
    return Out;
}

FSoundPropagation::FHeardEvent FSoundPropagation::LoudestHeard(
    const FVector& ListenerPos, const TArray<FSoundEvent>& Events, double Ambient)
{
    FHeardEvent Best;
    double BestIntensity = 0.0;
    for (const FSoundEvent& Event : Events)
    {
        const double Intensity =
            PerceivedIntensity(Event.Pos, ListenerPos, BaseLoudness(Event.Kind), Ambient);
        if (Intensity >= DefaultAudible && Intensity > BestIntensity)
        {
            BestIntensity = Intensity;
            Best.bHeard = true;
            Best.Pos = Event.Pos;
            Best.Kind = Event.Kind;
            Best.Intensity = Intensity;
            Best.Reaction = ReactionFor(Intensity, IsAlarming(Event.Kind));
        }
    }
    return Best;
}

double FSoundPropagation::AmbientFor(double BaseAmbient, bool bIsNight, double Rain01)
{
    double FloorLevel = FMath::Max(BaseAmbient, 0.0);
    if (bIsNight)
    {
        FloorLevel *= NightQuietFactor;
    }
    FloorLevel += FMath::Clamp(Rain01, 0.0, 1.0) * RainMaskGain;
    return FMath::Clamp(FloorLevel, 0.0, 1.0);
}
