// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Live bridge from the routine panel to the world's UGTCRoutineSubsystem. The
 * console toggle fills these with lambdas that re-resolve the subsystem each call,
 * so the panel survives a world change (mirrors FGTCForgeServices). Each returns a
 * human-readable result string the panel shows in its readout.
 */
struct FGTCRoutineServices
{
    TFunction<FString()> List;                                                  // every routine
    TFunction<FString(const FString& /*Id*/)> Show;                             // one routine's blocks
    TFunction<FString(const FString& /*Id*/)> Validate;                         // coverage + overlap
    TFunction<FString(const FString& /*Id*/, const FString& /*Name*/)> New;     // create empty
    TFunction<FString(const FString& /*Id*/, const FString& /*Start*/, const FString& /*End*/,
                      const FString& /*Activity*/, const FString& /*Place*/)> AddBlock;
    TFunction<FString(const FString& /*Id*/, const FString& /*Index*/)> RemoveBlock;
    TFunction<FString(const FString& /*Seed*/, const FString& /*Id*/)> Assign;
    TFunction<FString(const FString& /*Id*/)> Save;                             // one .routine.json
    TFunction<FString()> SaveBank;                                             // whole bank
    TFunction<FString()> Reload;                                              // re-read bank file
};

/**
 * SGTCRoutinePanel — the on-screen "editor on the side" for NPC daily routines.
 * Type a routine id (and a block's start/end/activity/place), then New / AddBlock /
 * RemoveBlock / Show / Validate / Save / Reload, or assign a routine to a citizen
 * seed. The readout echoes each result. The SAME bank the console + JSON edit, so a
 * human gets a GUI without losing the text/AI/MCP paths. Minimal Slate, matching
 * SGTCForgePanel / the C++-driven front-end.
 */
class GTC_UE5_API SGTCRoutinePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGTCRoutinePanel) {}
        SLATE_ARGUMENT(FGTCRoutineServices, Services)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual bool SupportsKeyboardFocus() const override { return true; }

private:
    FGTCRoutineServices Services;

    // Editable fields.
    FString CurrentId;
    FString CurrentName;
    FString BStart = TEXT("9.0");
    FString BEnd = TEXT("17.0");
    FString BActivity = TEXT("work");
    FString BPlace = TEXT("office");
    FString BIndex = TEXT("0");
    FString SeedStr = TEXT("0");
    FString LastResult = TEXT("Type a routine id, then New / AddBlock / Show. Edit a block's start/end/activity/place above AddBlock.");

    /** Run a no-arg service and capture its text. */
    FReply Run(const TFunction<FString()>& Service);
    /** Run an id-only service. */
    FReply RunId(const TFunction<FString(const FString&)>& Service);
};
