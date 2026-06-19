// Copyright Epic Games, Inc. All Rights Reserved.

#include "SGTCForgePanel.h"

#include "../Forge/GTCCharacterForge.h"
#include "../Wiring/CharacterWiring.h"
#include "../Wiring/RigAudit.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GTCForgePanel"

namespace
{
	// Render a wiring plan as a readable, multi-line checklist for the readout.
	FString DescribePlanText(const FCharacterWiringPlan& Plan)
	{
		FString S = Plan.Summary();
		for (const FCharacterAction& A : Plan.Actions)
		{
			S += FString::Printf(TEXT("\n%s %s"),
				A.bWired ? TEXT("[x]") : TEXT("[ ]"), GTCCharacterActionName(A.Action));
			if (!A.bWired && !A.Note.IsEmpty())
			{
				S += FString::Printf(TEXT("  (%s)"), *A.Note);
			}
		}
		return S;
	}
}

void SGTCForgePanel::Construct(const FArguments& InArgs)
{
	Services = InArgs._Services;

	auto MakeButton = [this](const FText& Label, const TFunction<FString(const FString&, const FString&)>& Service)
	{
		return SNew(SButton)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.ContentPadding(FMargin(18.f, 10.f))
			.OnClicked_Lambda([this, Service]() { return RunService(Service); })
			[
				SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 15)).Text(Label)
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
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
				.Text(LOCTEXT("Title", "Character Forge"))
			]
			// Skeleton path field.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SEditableTextBox)
				.MinDesiredWidth(420.f)
				.HintText(LOCTEXT("PathHint", "/Game/.../SK_MyCharacter.SK_MyCharacter"))
				.Text_Lambda([this]() { return FText::FromString(CurrentPath); })
				.OnTextChanged_Lambda([this](const FText& T) { CurrentPath = T.ToString(); })
			]
			// Role field.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SEditableTextBox)
				.HintText(LOCTEXT("RoleHint", "player | pedestrian | combatant | occupant"))
				.Text_Lambda([this]() { return FText::FromString(CurrentRole); })
				.OnTextChanged_Lambda([this](const FText& T) { CurrentRole = T.ToString(); })
			]
			// Action buttons.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f, 0.f) [ MakeButton(LOCTEXT("Inspect", "Inspect"), Services.Inspect) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f, 0.f) [ MakeButton(LOCTEXT("Apply", "Apply to Player"), Services.Apply) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(3.f, 0.f) [ MakeButton(LOCTEXT("Spawn", "Spawn"), Services.Spawn) ]
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

FReply SGTCForgePanel::RunService(const TFunction<FString(const FString&, const FString&)>& Service)
{
	if (Service)
	{
		LastResult = Service(CurrentPath, CurrentRole);
	}
	return FReply::Handled();
}

FReply SGTCForgePanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Enter runs Inspect for quick iteration.
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		return RunService(Services.Inspect);
	}
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE

// ---------------------------------------------------------------------------
// Console toggle — opens/closes the panel and wires it to the world's Forge.
// ---------------------------------------------------------------------------

namespace
{
	TSharedPtr<SGTCForgePanel> GForgePanel;

	UGTCCharacterForge* ForgeOf(UWorld* World)
	{
		return World ? World->GetSubsystem<UGTCCharacterForge>() : nullptr;
	}

	ACharacter* PlayerCharacterOf(UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GCharEditorCmd(
	TEXT("GTC.Character.Editor"),
	TEXT("GTC.Character.Editor — toggle the on-screen Character Forge side panel"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGameViewportClient* VPC = World ? World->GetGameViewport() : nullptr;
		if (!VPC)
		{
			UE_LOG(LogTemp, Warning, TEXT("GTC.Character.Editor: no game viewport."));
			return;
		}

		if (GForgePanel.IsValid())
		{
			VPC->RemoveViewportWidgetContent(GForgePanel.ToSharedRef());
			GForgePanel.Reset();
			return;
		}

		FGTCForgeServices Svc;
		Svc.Inspect = [World](const FString& Path, const FString& RoleStr) -> FString
		{
			UGTCCharacterForge* Forge = ForgeOf(World);
			if (!Forge) { return TEXT("No Forge subsystem."); }
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path);
			if (!Mesh) { return FString::Printf(TEXT("Could not load '%s'."), *Path); }
			const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Mesh->GetSkeleton());
			const FSkeletonProbe Probe = FSkeletonProbe::FromBoneNames(Bones);
			FString Text = DescribePlanText(FCharacterWiring::Plan(Probe, GTCParseCharacterRole(RoleStr)));
			Text += TEXT("\n\n") + GTCAuditRig(Probe, Bones).Summary();
			return Text;
		};
		Svc.Apply = [World](const FString& Path, const FString& RoleStr) -> FString
		{
			UGTCCharacterForge* Forge = ForgeOf(World);
			if (!Forge) { return TEXT("No Forge subsystem."); }
			ACharacter* Char = PlayerCharacterOf(World);
			if (!Char) { return TEXT("No player pawn to wire."); }
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path);
			if (!Mesh) { return FString::Printf(TEXT("Could not load '%s'."), *Path); }
			return DescribePlanText(Forge->ApplyToCharacter(Char, Mesh, GTCParseCharacterRole(RoleStr)));
		};
		Svc.Spawn = [World](const FString& Path, const FString& RoleStr) -> FString
		{
			UGTCCharacterForge* Forge = ForgeOf(World);
			if (!Forge) { return TEXT("No Forge subsystem."); }
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path);
			if (!Mesh) { return FString::Printf(TEXT("Could not load '%s'."), *Path); }

			FVector Loc = FVector::ZeroVector;
			FRotator Rot = FRotator::ZeroRotator;
			if (const ACharacter* Player = PlayerCharacterOf(World))
			{
				Loc = Player->GetActorLocation() + Player->GetActorForwardVector() * 200.0f;
				Rot = FRotator(0.0f, Player->GetActorRotation().Yaw + 180.0f, 0.0f);
			}
			FCharacterWiringPlan Plan;
			ACharacter* C = Forge->SpawnForged(Mesh, GTCParseCharacterRole(RoleStr), FTransform(Rot, Loc), 0, Plan);
			return C ? DescribePlanText(Plan) : FString(TEXT("Spawn failed."));
		};

		GForgePanel = SNew(SGTCForgePanel).Services(Svc);
		VPC->AddViewportWidgetContent(GForgePanel.ToSharedRef(), 6000);
	}));
