// Copyright (c) 2026 GTC contributors

#include "EmergencyServices.h"

#include "Math/UnrealMathUtility.h"

#include <limits>

EEmergencyService FEmergencyServices::ServiceForInt(int32 IncidentRaw)
{
    switch (static_cast<EEmergencyIncident>(IncidentRaw))
    {
    case EEmergencyIncident::Fire:
        return EEmergencyService::FireTruck;
    case EEmergencyIncident::Wreck:
        return EEmergencyService::Ambulance;
    case EEmergencyIncident::Injury:
        return EEmergencyService::Ambulance;
    case EEmergencyIncident::Shooting:
        return EEmergencyService::PoliceBackup;
    default:
        // Unknown/out-of-range incident -> help-first default (someone is hurt).
        return EEmergencyService::Ambulance;
    }
}

EEmergencyService FEmergencyServices::ServiceFor(EEmergencyIncident Incident)
{
    return ServiceForInt(static_cast<int32>(Incident));
}

double FEmergencyServices::EtaSeconds(const FVector& Dispatch, const FVector& Incident, double Speed)
{
    if (Speed <= 0.0)
    {
        // Can't get there — caller treats +infinity as "no responder available".
        return std::numeric_limits<double>::infinity();
    }
    const double Dx = Incident.X - Dispatch.X;
    const double Dz = Incident.Z - Dispatch.Z;
    return FMath::Sqrt(Dx * Dx + Dz * Dz) / Speed;
}

int32 FEmergencyServices::NearestResponderIndex(const FVector& Incident, const TArray<FResponder>& Responders)
{
    int32 BestIndex = INDEX_NONE;
    double BestDistSq = std::numeric_limits<double>::infinity();
    for (int32 Index = 0; Index < Responders.Num(); ++Index)
    {
        const FResponder& Responder = Responders[Index];
        if (!Responder.bHasPos)
        {
            continue;
        }
        const double Dx = Responder.Pos.X - Incident.X;
        const double Dz = Responder.Pos.Z - Incident.Z;
        const double DistSq = Dx * Dx + Dz * Dz;
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestIndex = Index;
        }
    }
    return BestIndex;
}

bool FEmergencyServices::ShouldDispatch(EEmergencyIncident Incident, bool bPlayerCaused, int32 WantedStars)
{
    if (ServiceFor(Incident) == EEmergencyService::PoliceBackup)
    {
        return true;
    }
    if (bPlayerCaused && WantedStars >= HotSceneStars)
    {
        return false;
    }
    return true;
}

double FEmergencyServices::ReviveChance(double Severity)
{
    const double S = FMath::Clamp(Severity, 0.0, 1.0);
    if (S >= UnrevivableSeverity)
    {
        return 0.0;
    }
    return 1.0 - S;
}

// --- Stateful response timer ------------------------------------------------

FEmergencyServices::FEmergencyServices(double InResponseDelay)
    : ResponseDelay(FMath::Max(InResponseDelay, 0.0))
{
}

void FEmergencyServices::Begin()
{
    if (Phase != EEmergencyPhase::Idle)
    {
        return;
    }
    Phase = EEmergencyPhase::EnRoute;
    Elapsed = 0.0;
}

void FEmergencyServices::Tick(double Delta)
{
    if (Phase != EEmergencyPhase::EnRoute)
    {
        return;
    }
    Elapsed += FMath::Max(Delta, 0.0);
    if (Elapsed >= ResponseDelay)
    {
        Elapsed = ResponseDelay;
        Phase = EEmergencyPhase::Treating;
    }
}

bool FEmergencyServices::HasArrived() const
{
    return Phase == EEmergencyPhase::Treating;
}

bool FEmergencyServices::Treating() const
{
    // Identical to HasArrived() by design; the oracle tests both separately.
    return Phase == EEmergencyPhase::Treating;
}

double FEmergencyServices::Progress() const
{
    if (Phase == EEmergencyPhase::Idle)
    {
        return 0.0;
    }
    if (Phase == EEmergencyPhase::Treating)
    {
        return 1.0;
    }
    if (ResponseDelay <= 0.0)
    {
        return 1.0;
    }
    return FMath::Clamp(Elapsed / ResponseDelay, 0.0, 1.0);
}

void FEmergencyServices::Cancel()
{
    Phase = EEmergencyPhase::Idle;
    Elapsed = 0.0;
}

void FEmergencyServices::Reset()
{
    Cancel();
}

EEmergencyPhase FEmergencyServices::GetPhase() const
{
    return Phase;
}
