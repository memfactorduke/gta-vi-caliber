// Copyright (c) 2026 GTC contributors

#include "MissionCampaign.h"

void UMissionCampaign::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMissionCampaign::Deinitialize()
{
    UnbindController();
    Super::Deinitialize();
}

UMissionCampaign::FCampaignObjective UMissionCampaign::MakeReach(
    const FString& Id, const FString& Text, const FVector& Pos, double Radius)
{
    FCampaignObjective O;
    O.Id = Id;
    O.Text = Text;
    O.Pos = Pos;
    O.Kind = TEXT("reach");
    O.Radius = Radius;
    O.Duration = 3.0; // Godot _reach leaves duration at its objective default.
    return O;
}

UMissionCampaign::FCampaignObjective UMissionCampaign::MakeHold(
    const FString& Id, const FString& Text, const FVector& Pos, double Duration, double Radius)
{
    FCampaignObjective O;
    O.Id = Id;
    O.Text = Text;
    O.Pos = Pos;
    O.Kind = TEXT("hold");
    O.Radius = Radius;
    O.Duration = Duration;
    return O;
}

TArray<UMissionCampaign::FCampaignMission> UMissionCampaign::OpeningArc()
{
    // Five-mission opening arc — verbatim from Godot mission_campaign.gd _missions().
    // Absolute world coordinates near the downtown spawn (FloatingOrigin conversion
    // is Wave 3); kinds/radii/durations match the Godot table exactly.
    TArray<FCampaignMission> Missions;

    {
        FCampaignMission M;
        M.Id = TEXT("intro");
        M.Title = TEXT("WELCOME TO THE COAST");
        M.Objectives = {
            MakeReach(TEXT("reach_car"), TEXT("Get in your car"), FVector(7, 1, 5)),
            MakeReach(TEXT("drive_strip"), TEXT("Drive down to the strip"), FVector(72, 1, -48), 7.0),
            MakeReach(TEXT("return_home"), TEXT("Head back to the start"), FVector(0, 1, 0)),
        };
        Missions.Add(MoveTemp(M));
    }
    {
        FCampaignMission M;
        M.Id = TEXT("pickup");
        M.Title = TEXT("THE PICKUP");
        M.Objectives = {
            MakeReach(TEXT("m2_stash"), TEXT("Collect the stash from the alley"), FVector(-58, 1, 34)),
            MakeHold(TEXT("m2_contact"), TEXT("Wait for the contact at the docks"), FVector(-24, 1, -86), 2.5),
            MakeReach(TEXT("m2_dropoff"), TEXT("Run the package to the drop"), FVector(46, 1, 62)),
        };
        Missions.Add(MoveTemp(M));
    }
    {
        FCampaignMission M;
        M.Id = TEXT("heat");
        M.Title = TEXT("HEAT");
        M.TimeLimit = 150.0;
        M.Objectives = {
            MakeReach(TEXT("m3_wheels"), TEXT("Grab fresh wheels — move!"), FVector(16, 1, -28)),
            MakeReach(TEXT("m3_strip"), TEXT("Tear down the strip, they're tailing you"), FVector(96, 1, -14), 8.0),
            MakeReach(TEXT("m3_safehouse"), TEXT("Make the safehouse before the clock dies"), FVector(-44, 1, -60)),
        };
        Missions.Add(MoveTemp(M));
    }
    {
        FCampaignMission M;
        M.Id = TEXT("deal");
        M.Title = TEXT("THE DEAL");
        M.Objectives = {
            MakeHold(TEXT("m4_meet"), TEXT("Hold the meet — show them you're alone"), FVector(62, 1, 38), 3.0),
            MakeReach(TEXT("m4_exchange"), TEXT("Make the exchange under the overpass"), FVector(-70, 1, -18)),
            MakeHold(TEXT("m4_bank"), TEXT("Sit on the cash till it cools"), FVector(10, 1, 70), 3.0),
        };
        Missions.Add(MoveTemp(M));
    }
    {
        FCampaignMission M;
        M.Id = TEXT("kingpin");
        M.Title = TEXT("KINGPIN");
        M.TimeLimit = 180.0;
        M.Objectives = {
            MakeReach(TEXT("m5_collect"), TEXT("Collect tribute across town"), FVector(-88, 1, 52), 7.0),
            MakeHold(TEXT("m5_rivals"), TEXT("Stare down the rivals' corner"), FVector(82, 1, -66), 2.0),
            MakeReach(TEXT("m5_strip"), TEXT("Own the strip one last time"), FVector(72, 1, -48), 8.0),
            MakeReach(TEXT("m5_throne"), TEXT("Take the throne — return home the king"), FVector(0, 1, 0)),
        };
        Missions.Add(MoveTemp(M));
    }

    return Missions;
}

void UMissionCampaign::BindController(UMissionController* Controller)
{
    UnbindController();
    if (Controller == nullptr)
    {
        return;
    }
    BoundController = Controller;

    // Build the merged MissionChain purely for sequencing (index/progress). The
    // full authored mission data (kinds/radii/durations) is read from OpeningArc()
    // by ActiveIndex(); FMissionDef only carries id/title.
    TArray<MissionChain::FMissionDef> Defs;
    for (const FCampaignMission& M : OpeningArc())
    {
        Defs.Add(MissionChain::FMissionDef(M.Id, M.Title));
    }
    Chain = MissionChain(Defs);
    bRetryPending = false;

    PassedHandle = Controller->OnMissionCompleted.AddUObject(this, &UMissionCampaign::HandleMissionPassed);
    FailedHandle = Controller->OnMissionFailed.AddUObject(this, &UMissionCampaign::HandleMissionFailed);

    LoadCurrent();
}

void UMissionCampaign::UnbindController()
{
    if (UMissionController* Controller = BoundController.Get())
    {
        if (PassedHandle.IsValid())
        {
            Controller->OnMissionCompleted.Remove(PassedHandle);
        }
        if (FailedHandle.IsValid())
        {
            Controller->OnMissionFailed.Remove(FailedHandle);
        }
    }
    PassedHandle.Reset();
    FailedHandle.Reset();
    BoundController.Reset();
}

void UMissionCampaign::LoadCurrent()
{
    UMissionController* Controller = BoundController.Get();
    if (Controller == nullptr)
    {
        return;
    }
    if (Chain.IsCampaignComplete())
    {
        return; // Godot: mission.is_empty() early-out.
    }

    const TArray<FCampaignMission> Arc = OpeningArc();
    const int32 ActiveIndex = Chain.ActiveIndex();
    if (!Arc.IsValidIndex(ActiveIndex))
    {
        return;
    }
    const FCampaignMission& Mission = Arc[ActiveIndex];

    // Split the authored objectives into controller defs + waypoints + driver defs.
    TArray<UMissionController::FObjectiveDef> ObjectiveDefs;
    TMap<FString, FVector> Waypoints;
    TMap<FString, FMissionObjectiveDriver::FObjectiveDef> DriverDefs;
    ObjectiveDefs.Reserve(Mission.Objectives.Num());
    for (const FCampaignObjective& O : Mission.Objectives)
    {
        ObjectiveDefs.Add({ O.Id, O.Text });
        Waypoints.Add(O.Id, O.Pos);
        DriverDefs.Add(O.Id, FMissionObjectiveDriver::FObjectiveDef(O.Kind, O.Radius, O.Duration));
    }

    Controller->Title = Mission.Title;
    Controller->ObjectiveDefs = MoveTemp(ObjectiveDefs);
    Controller->Waypoints = MoveTemp(Waypoints);
    Controller->TimeLimit = Mission.TimeLimit;
    Controller->Reset(); // Godot has_method("reset") -> reset(): rebuild + begin.

    Driver.Defs = MoveTemp(DriverDefs);
    Driver.Bind();
}

void UMissionCampaign::HandleMissionPassed()
{
    Chain.CompleteCurrent();
    if (Chain.IsCampaignComplete())
    {
        return;
    }
    // Godot defers _load_current to let the controller finish emitting; the UE
    // delegate broadcast has already returned to the controller by the time this
    // runs, so a direct load is safe (no re-entrant rebuild mid-broadcast).
    LoadCurrent();
}

void UMissionCampaign::HandleMissionFailed()
{
    // Failure (timeout / death) replays the same mission (a checkpoint retry), gated on
    // the player being back on their feet (AdvanceRetryIfPending).
    bRetryPending = true;
}

bool UMissionCampaign::AdvanceRetryIfPending(bool bPlayerDead)
{
    if (bRetryPending && !bPlayerDead)
    {
        bRetryPending = false;
        LoadCurrent();
        return true;
    }
    return false;
}
