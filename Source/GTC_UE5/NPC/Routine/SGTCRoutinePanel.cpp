// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGTCRoutinePanel.h"

#include "GTCRoutineSubsystem.h"
#include "../Decision/NpcRoutine.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"

#define LOCTEXT_NAMESPACE "GTCRoutinePanel"

void SGTCRoutinePanel::Construct(const FArguments& InArgs)
{
    Services = InArgs._Services;

    // A labelled text field bound to a member string.
    auto Field = [](const FText& Hint, TFunction<FString()> Get, TFunction<void(const FString&)> Set, float Width)
    {
        return SNew(SEditableTextBox)
            .MinDesiredWidth(Width)
            .HintText(Hint)
            .Text_Lambda([Get]() { return FText::FromString(Get()); })
            .OnTextChanged_Lambda([Set](const FText& T) { Set(T.ToString()); });
    };

    auto Button = [](const FText& Label, TFunction<FReply()> OnClick)
    {
        return SNew(SButton)
            .HAlign(HAlign_Center).VAlign(VAlign_Center)
            .ContentPadding(FMargin(12.f, 8.f))
            .OnClicked_Lambda([OnClick]() { return OnClick(); })
            [
                SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13)).Text(Label)
            ];
    };

    ChildSlot
    [
        SNew(SBorder)
        .HAlign(HAlign_Left).VAlign(VAlign_Top)
        .Padding(FMargin(16.f))
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.05f, 0.06f, 0.08f, 0.93f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                .Text(LOCTEXT("Title", "NPC Routine Editor"))
            ]
            // Id + display name.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Field(LOCTEXT("IdHint", "routine id e.g. early_bird_office"),
                        [this]() { return CurrentId; }, [this](const FString& S) { CurrentId = S; }, 260.f) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Field(LOCTEXT("NameHint", "display name"),
                        [this]() { return CurrentName; }, [this](const FString& S) { CurrentName = S; }, 200.f) ]
            ]
            // New / Show / Validate.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("New", "New"), [this]() { return Services.New ? (LastResult = Services.New(CurrentId, CurrentName), FReply::Handled()) : FReply::Handled(); }) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("Show", "Show"), [this]() { return RunId(Services.Show); }) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("Validate", "Validate"), [this]() { return RunId(Services.Validate); }) ]
            ]
            // Block fields: start / end / activity / place.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
                [ Field(LOCTEXT("StartHint", "start"), [this]() { return BStart; }, [this](const FString& S) { BStart = S; }, 64.f) ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
                [ Field(LOCTEXT("EndHint", "end"), [this]() { return BEnd; }, [this](const FString& S) { BEnd = S; }, 64.f) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Field(LOCTEXT("ActHint", "activity"), [this]() { return BActivity; }, [this](const FString& S) { BActivity = S; }, 120.f) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Field(LOCTEXT("PlaceHint", "place"), [this]() { return BPlace; }, [this](const FString& S) { BPlace = S; }, 120.f) ]
            ]
            // AddBlock / RemoveBlock(index).
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("AddBlock", "Add Block"), [this]()
                    { if (Services.AddBlock) { LastResult = Services.AddBlock(CurrentId, BStart, BEnd, BActivity, BPlace); } return FReply::Handled(); }) ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
                [ Field(LOCTEXT("IdxHint", "idx"), [this]() { return BIndex; }, [this](const FString& S) { BIndex = S; }, 52.f) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("RemoveBlock", "Remove Block"), [this]()
                    { if (Services.RemoveBlock) { LastResult = Services.RemoveBlock(CurrentId, BIndex); } return FReply::Handled(); }) ]
            ]
            // Assign to a citizen seed.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
                [ Field(LOCTEXT("SeedHint", "seed"), [this]() { return SeedStr; }, [this](const FString& S) { SeedStr = S; }, 90.f) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("Assign", "Assign id -> seed"), [this]()
                    { if (Services.Assign) { LastResult = Services.Assign(SeedStr, CurrentId); } return FReply::Handled(); }) ]
            ]
            // List / Save / SaveBank / Reload.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("List", "List"), [this]() { return Run(Services.List); }) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("Save", "Save"), [this]() { return RunId(Services.Save); }) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("SaveBank", "Save Bank"), [this]() { return Run(Services.SaveBank); }) ]
                + SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f)
                [ Button(LOCTEXT("Reload", "Reload"), [this]() { return Run(Services.Reload); }) ]
            ]
            // Readout.
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.92f, 1.f))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                .Text_Lambda([this]() { return FText::FromString(LastResult); })
            ]
        ]
    ];
}

FReply SGTCRoutinePanel::Run(const TFunction<FString()>& Service)
{
    if (Service) { LastResult = Service(); }
    return FReply::Handled();
}

FReply SGTCRoutinePanel::RunId(const TFunction<FString(const FString&)>& Service)
{
    if (Service) { LastResult = Service(CurrentId); }
    return FReply::Handled();
}

// --- Console toggle: GTC.Routine.Panel ---------------------------------------

namespace
{
    TSharedPtr<SGTCRoutinePanel> GRoutinePanel;

    UGTCRoutineSubsystem* GtcPanelSubOf(UWorld* World)
    {
        return World ? World->GetSubsystem<UGTCRoutineSubsystem>() : nullptr;
    }

    FString GtcPanelShowText(UGTCRoutineSubsystem* Sub, const FString& Id)
    {
        if (!Sub) { return TEXT("No routine subsystem."); }
        const FNpcRoutine* R = FNpcRoutineLibrary::Find(Sub->GetRoutines(), Id);
        if (!R) { return FString::Printf(TEXT("No routine '%s'."), *Id); }
        FString S = FString::Printf(TEXT("%s \"%s\":"), *R->Id, *R->DisplayName);
        for (int32 i = 0; i < R->Blocks.Num(); ++i)
        {
            const FNpcScheduleBlock& B = R->Blocks[i];
            S += FString::Printf(TEXT("\n  [%d] %5.2f-%5.2f  %s @ %s"), i, B.Start, B.End, *B.Activity, *B.Place);
        }
        if (!FNpcRoutineLibrary::CoversFullDay(*R)) { S += TEXT("\n  (warning: has a gap — not a full day)"); }
        return S;
    }

    FGTCRoutineServices GtcMakeRoutineServices(UWorld* World)
    {
        FGTCRoutineServices Svc;

        Svc.List = [World]() -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            const TArray<FNpcRoutine>& All = Sub->GetRoutines();
            FString S = FString::Printf(TEXT("%d routine(s):"), All.Num());
            for (const FNpcRoutine& R : All)
            {
                S += FString::Printf(TEXT("\n  %s  \"%s\"  (%d blocks%s)"),
                    *R.Id, *R.DisplayName, R.Blocks.Num(),
                    FNpcRoutineLibrary::CoversFullDay(R) ? TEXT("") : TEXT(", GAP"));
            }
            return S;
        };
        Svc.Show = [World](const FString& Id) -> FString { return GtcPanelShowText(GtcPanelSubOf(World), Id); };
        Svc.Validate = [World](const FString& Id) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            const FNpcRoutine* R = FNpcRoutineLibrary::Find(Sub->GetRoutines(), Id);
            if (!R) { return FString::Printf(TEXT("No routine '%s'."), *Id); }
            const int32 Overlap = FNpcRoutineLibrary::FirstOverlap(*R);
            return FString::Printf(TEXT("'%s': full-day=%s, overlap=%s"), *R->Id,
                FNpcRoutineLibrary::CoversFullDay(*R) ? TEXT("yes") : TEXT("NO (gap)"),
                Overlap == INDEX_NONE ? TEXT("none") : *FString::Printf(TEXT("block %d shadowed"), Overlap));
        };
        Svc.New = [World](const FString& Id, const FString& Name) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            return Sub->CreateRoutine(Id, Name)
                ? FString::Printf(TEXT("Created '%s'. Add blocks below."), *Id)
                : FString::Printf(TEXT("Could not create '%s' (empty id or already exists)."), *Id);
        };
        Svc.AddBlock = [World](const FString& Id, const FString& Start, const FString& End, const FString& Act, const FString& Place) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            if (!Sub->AddBlock(Id, FCString::Atof(*Start), FCString::Atof(*End), Act, Place))
            {
                return FString::Printf(TEXT("AddBlock failed for '%s'."), *Id);
            }
            return GtcPanelShowText(Sub, Id);
        };
        Svc.RemoveBlock = [World](const FString& Id, const FString& Index) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            if (!Sub->RemoveBlock(Id, FCString::Atoi(*Index))) { return TEXT("RemoveBlock failed (bad id/index)."); }
            return GtcPanelShowText(Sub, Id);
        };
        Svc.Assign = [World](const FString& Seed, const FString& Id) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            return Sub->AssignRoutineToSeed(FCString::Atoi(*Seed), Id)
                ? FString::Printf(TEXT("Seed %s now runs '%s'."), *Seed, *Id)
                : FString::Printf(TEXT("Assign failed (no routine '%s')."), *Id);
        };
        Svc.Save = [World](const FString& Id) -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            return Sub->SaveRoutine(Id, FString())
                ? FString::Printf(TEXT("Saved '%s' to Saved/Routines."), *Id)
                : FString::Printf(TEXT("Save failed for '%s'."), *Id);
        };
        Svc.SaveBank = [World]() -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            return Sub->SaveBank(FString()) ? TEXT("Saved the whole bank to Saved/Routines/routines.bank.json.") : TEXT("SaveBank failed.");
        };
        Svc.Reload = [World]() -> FString
        {
            UGTCRoutineSubsystem* Sub = GtcPanelSubOf(World);
            if (!Sub) { return TEXT("No routine subsystem."); }
            const bool bOk = Sub->ReloadDefaultBank();
            return FString::Printf(TEXT("Reload %s (%d routines now)."), bOk ? TEXT("ok") : TEXT("found no bank file"), Sub->GetRoutines().Num());
        };
        return Svc;
    }

    FAutoConsoleCommandWithWorldAndArgs GRoutinePanelCmd(
        TEXT("GTC.Routine.Panel"),
        TEXT("GTC.Routine.Panel — toggle the on-screen NPC Routine Editor panel"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>&, UWorld* World)
            {
                UGameViewportClient* VPC = World ? World->GetGameViewport() : nullptr;
                if (!VPC)
                {
                    UE_LOG(LogTemp, Warning, TEXT("GTC.Routine.Panel: no game viewport."));
                    return;
                }
                if (GRoutinePanel.IsValid())
                {
                    VPC->RemoveViewportWidgetContent(GRoutinePanel.ToSharedRef());
                    GRoutinePanel.Reset();
                    return;
                }
                GRoutinePanel = SNew(SGTCRoutinePanel).Services(GtcMakeRoutineServices(World));
                VPC->AddViewportWidgetContent(GRoutinePanel.ToSharedRef(), 6000);
            }));
}

#undef LOCTEXT_NAMESPACE
