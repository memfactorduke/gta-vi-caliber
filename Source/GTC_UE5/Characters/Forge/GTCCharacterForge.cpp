// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCCharacterForge.h"

#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"

#include "../Wiring/RigProfile.h"
#include "../Wiring/RigAudit.h"
#include "CharacterRecipe.h"
#include "../../Weapons/Component/GTCWeaponComponent.h"
#include "../../Player/Health/PlayerHealthComponent.h"
#include "../../NPC/Agent/GTCCitizen.h"
#include "../../NPC/Agent/GTCCrowdSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGtcCharacterForge, Log, All);

bool UGTCCharacterForge::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Present in the running game, PIE, and the open editor world (side editor).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

// ---------------------------------------------------------------------------
// Bridge: rig → wiring plan
// ---------------------------------------------------------------------------

TArray<FString> UGTCCharacterForge::BoneNamesOf(const USkeleton* Skeleton)
{
	TArray<FString> Names;
	if (!Skeleton)
	{
		return Names;
	}

	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const int32 Num = Ref.GetNum();
	Names.Reserve(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		Names.Add(Ref.GetBoneName(i).ToString());
	}
	return Names;
}

FSkeletonProbe UGTCCharacterForge::ProbeSkeleton(const USkeleton* Skeleton)
{
	return FSkeletonProbe::FromBoneNames(BoneNamesOf(Skeleton));
}

FSkeletonProbe UGTCCharacterForge::ProbeMesh(const USkeletalMesh* Mesh)
{
	// GetSkeleton() is non-const on USkeletalMesh; the probe is read-only.
	USkeletalMesh* Mutable = const_cast<USkeletalMesh*>(Mesh);
	const USkeleton* Skeleton = Mutable ? Mutable->GetSkeleton() : nullptr;
	return ProbeSkeleton(Skeleton);
}

FCharacterWiringPlan UGTCCharacterForge::PlanForMesh(const USkeletalMesh* Mesh, ECharacterRole Role) const
{
	return FCharacterWiring::Plan(ProbeMesh(Mesh), Role);
}

// ---------------------------------------------------------------------------
// Apply: wire a live pawn
// ---------------------------------------------------------------------------

FCharacterWiringPlan UGTCCharacterForge::ApplyToCharacter(ACharacter* Target, USkeletalMesh* Body, ECharacterRole Role) const
{
	FCharacterWiringPlan Plan;
	Plan.Role = Role;
	if (!Target || !Body)
	{
		return Plan;
	}

	// Probe once from the rig's bone names — reused for the plan and for resolving
	// the hand bone the weapon binds to.
	const TArray<FString> Bones = BoneNamesOf(Body->GetSkeleton());
	Plan = FCharacterWiring::Plan(FSkeletonProbe::FromBoneNames(Bones), Role);

	// Assign the body mesh (keeps whatever locomotion anim class the pawn's own
	// appearance pass already set on the component).
	if (USkeletalMeshComponent* MeshComp = Target->GetMesh())
	{
		MeshComp->SetSkeletalMeshAsset(Body);
	}

	// Attach the gameplay components the plan calls for, driven by the action
	// binding table: every wired action whose binding names a component tag gets
	// that component ensured on the pawn. Idempotent — FindComponentByClass means
	// a pawn that already owns one (the player's health/weapon) is never doubled.
	bool bNeedWeapon = false;
	bool bNeedHealth = false;
	for (const FCharacterAction& A : Plan.Actions)
	{
		if (!A.bWired)
		{
			continue;
		}
		const FActionBinding Bind = GTCActionBinding(A.Action);
		if (FCString::Strcmp(Bind.ComponentTag, TEXT("Weapon")) == 0) { bNeedWeapon = true; }
		else if (FCString::Strcmp(Bind.ComponentTag, TEXT("Health")) == 0) { bNeedHealth = true; }
	}

	if (bNeedWeapon)
	{
		UGTCWeaponComponent* Weapon = Target->FindComponentByClass<UGTCWeaponComponent>();
		const bool bFresh = (Weapon == nullptr);
		if (bFresh)
		{
			Weapon = NewObject<UGTCWeaponComponent>(Target);
		}
		if (Weapon)
		{
			// Bind the weapon to THIS rig's actual hand bone (the component defaults
			// to UE's "hand_r", which is wrong for Mixamo/MetaHuman/etc.). Prefer the
			// right hand, fall back to the left. Set before RegisterComponent so the
			// component's first attach uses the correct socket.
			FString Hand = GTCResolveCanonicalBone(ECanonicalBone::RightHand, Bones);
			if (Hand.IsEmpty())
			{
				Hand = GTCResolveCanonicalBone(ECanonicalBone::LeftHand, Bones);
			}
			if (!Hand.IsEmpty())
			{
				Weapon->AttachSocketName = FName(*Hand);
			}
			if (bFresh)
			{
				Weapon->RegisterComponent();
			}
		}
	}
	if (bNeedHealth && !Target->FindComponentByClass<UPlayerHealthComponent>())
	{
		if (UPlayerHealthComponent* Health = NewObject<UPlayerHealthComponent>(Target))
		{
			Health->RegisterComponent();
		}
	}

	return Plan;
}

ACharacter* UGTCCharacterForge::SpawnForged(USkeletalMesh* Body, ECharacterRole Role,
	const FTransform& Where, int32 Seed, FCharacterWiringPlan& OutPlan) const
{
	OutPlan = FCharacterWiringPlan();
	OutPlan.Role = Role;

	UWorld* W = GetWorld();
	if (!W || !Body)
	{
		return nullptr;
	}

	// Prefer the crowd's configured citizen class (a BP with the shared appearance
	// set + AI controller); fall back to the raw C++ class.
	TSubclassOf<AGTCCitizen> SpawnClass = AGTCCitizen::StaticClass();
	if (const UGTCCrowdSubsystem* Crowd = W->GetSubsystem<UGTCCrowdSubsystem>())
	{
		if (Crowd->CitizenClass.Get())
		{
			SpawnClass = Crowd->CitizenClass;
		}
	}

	AGTCCitizen* C = W->SpawnActorDeferred<AGTCCitizen>(
		SpawnClass, Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!C)
	{
		return nullptr;
	}

	// Stamp identity before BeginPlay so the citizen comes up fully formed, then
	// override the body from the dropped-in skeleton and wire the role's actions.
	C->InitializeFromSeed(Seed);
	C->FinishSpawning(Where);
	OutPlan = ApplyToCharacter(C, Body, Role);
	return C;
}

// ---------------------------------------------------------------------------
// Console "side editor"
// ---------------------------------------------------------------------------

namespace
{
	UGTCCharacterForge* ResolveForge(UWorld* World)
	{
		return World ? World->GetSubsystem<UGTCCharacterForge>() : nullptr;
	}

	// Print a plan: the headline, then every relevant action with its state, then
	// the skip warnings. This is what a creator sees after dropping a skeleton in.
	void LogPlan(const FCharacterWiringPlan& Plan, const FString& Label)
	{
		UE_LOG(LogGtcCharacterForge, Display, TEXT("%s — %s"), *Label, *Plan.Summary());
		for (const FCharacterAction& A : Plan.Actions)
		{
			if (A.bWired)
			{
				UE_LOG(LogGtcCharacterForge, Display, TEXT("    [x] %s"), GTCCharacterActionName(A.Action));
			}
			else
			{
				UE_LOG(LogGtcCharacterForge, Display, TEXT("    [ ] %s — %s"),
					GTCCharacterActionName(A.Action), *A.Note);
			}
		}
		if (!Plan.bUsable)
		{
			UE_LOG(LogGtcCharacterForge, Warning,
				TEXT("    rig is UNUSABLE for this role (cannot meet the minimum)."));
		}
	}
}

static FAutoConsoleCommandWithWorldAndArgs GCharInspectCmd(
	TEXT("GTC.Character.Inspect"),
	TEXT("GTC.Character.Inspect <SkeletalMeshPath> [player|ped|combatant|occupant] — probe a rig and print what would auto-wire"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Inspect <SkeletalMeshPath> [role]"));
			return;
		}
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}

		// No role given → let the backend auto-pick from the rig.
		const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Mesh->GetSkeleton());
		const FSkeletonProbe Probe = FSkeletonProbe::FromBoneNames(Bones);
		const ECharacterRole Role = Args.IsValidIndex(1)
			? GTCParseCharacterRole(Args[1])
			: GTCSuggestRole(Probe);
		LogPlan(FCharacterWiring::Plan(Probe, Role), Mesh->GetName());
		// Quality grade alongside the wiring checklist.
		UE_LOG(LogGtcCharacterForge, Display, TEXT("    %s"), *GTCAuditRig(Probe, Bones).Summary());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharApplyCmd(
	TEXT("GTC.Character.Apply"),
	TEXT("GTC.Character.Apply <SkeletalMeshPath> [role] — wire the player pawn from this skeleton right now"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Apply <SkeletalMeshPath> [role]"));
			return;
		}
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ACharacter* Char = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (!Char)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("No player character to wire."));
			return;
		}

		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}

		const ECharacterRole Role = Args.IsValidIndex(1)
			? GTCParseCharacterRole(Args[1]) : ECharacterRole::Player;
		const FCharacterWiringPlan Plan = Forge->ApplyToCharacter(Char, Mesh, Role);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Wired player pawn from '%s':"), *Mesh->GetName());
		LogPlan(Plan, Char->GetName());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharSpawnCmd(
	TEXT("GTC.Character.Spawn"),
	TEXT("GTC.Character.Spawn <SkeletalMeshPath> [role] [seed] — spawn a fresh wired pawn from this skeleton in front of the player"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Spawn <SkeletalMeshPath> [role] [seed]"));
			return;
		}
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}

		// No role given → let the backend auto-pick from the rig.
		const ECharacterRole Role = Args.IsValidIndex(1)
			? GTCParseCharacterRole(Args[1])
			: GTCSuggestRole(UGTCCharacterForge::ProbeMesh(Mesh));
		const int32 Seed = Args.IsValidIndex(2) ? FCString::Atoi(*Args[2]) : 0;

		// Place it a couple of metres in front of the player (origin if headless).
		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		if (const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
		{
			if (const APawn* P = PC->GetPawn())
			{
				Loc = P->GetActorLocation() + P->GetActorForwardVector() * 200.0f;
				Rot = FRotator(0.0f, P->GetActorRotation().Yaw + 180.0f, 0.0f);
			}
		}

		FCharacterWiringPlan Plan;
		ACharacter* Spawned = Forge->SpawnForged(Mesh, Role, FTransform(Rot, Loc), Seed, Plan);
		if (!Spawned)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Spawn failed (no world?)."));
			return;
		}
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Spawned '%s' from '%s':"), *Spawned->GetName(), *Mesh->GetName());
		LogPlan(Plan, Spawned->GetName());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharRigCmd(
	TEXT("GTC.Character.Rig"),
	TEXT("GTC.Character.Rig <SkeletalMeshPath> — identify the rig convention and resolve its key bones for retargeting"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Rig <SkeletalMeshPath>"));
			return;
		}
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}

		const USkeleton* Skel = Mesh->GetSkeleton();
		const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Skel);
		const ERigConvention Convention = GTCDetectRigConvention(Bones);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("'%s' — %d bones, convention: %s"),
			*Mesh->GetName(), Bones.Num(), GTCRigConventionName(Convention));

		// Resolve the joints the engine needs to find for retargeting / sockets.
		const ECanonicalBone Key[] = {
			ECanonicalBone::Pelvis, ECanonicalBone::Spine, ECanonicalBone::Head,
			ECanonicalBone::RightHand, ECanonicalBone::LeftHand,
			ECanonicalBone::RightFoot, ECanonicalBone::LeftFoot, ECanonicalBone::Jaw,
		};
		for (ECanonicalBone B : Key)
		{
			const FString Actual = GTCResolveCanonicalBone(B, Bones);
			UE_LOG(LogGtcCharacterForge, Display, TEXT("    %-10s -> %s"),
				GTCCanonicalBoneName(B), Actual.IsEmpty() ? TEXT("(absent)") : *Actual);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharAuditCmd(
	TEXT("GTC.Character.Audit"),
	TEXT("GTC.Character.Audit <SkeletalMeshPath> — grade a rig's quality (0-100) with actionable issues"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Audit <SkeletalMeshPath>"));
			return;
		}
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}
		const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Mesh->GetSkeleton());
		const FRigAudit Audit = GTCAuditRig(FSkeletonProbe::FromBoneNames(Bones), Bones);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("'%s' — %s"), *Mesh->GetName(), *Audit.Summary());
		for (const FString& Issue : Audit.Issues)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("    ! %s"), *Issue);
		}
		for (const FString& Note : Audit.Notes)
		{
			UE_LOG(LogGtcCharacterForge, Display, TEXT("    - %s"), *Note);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharReadyCmd(
	TEXT("GTC.Character.Ready"),
	TEXT("GTC.Character.Ready <SkeletalMeshPath> [role] — can this rig ship in that role? (plan + audit + role rules)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Ready <SkeletalMeshPath> [role]"));
			return;
		}
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Args[0]);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load skeletal mesh '%s'."), *Args[0]);
			return;
		}
		const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Mesh->GetSkeleton());
		const FSkeletonProbe Probe = FSkeletonProbe::FromBoneNames(Bones);
		const ECharacterRole Role = Args.IsValidIndex(1) ? GTCParseCharacterRole(Args[1]) : GTCSuggestRole(Probe);
		const FCharacterReadiness Ready = GTCAssessReadiness(Probe, Bones, Role);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("'%s' — %s"), *Mesh->GetName(), *Ready.Summary());
		for (const FString& B : Ready.Blockers)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("    BLOCKER: %s"), *B);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharSaveRecipeCmd(
	TEXT("GTC.Character.SaveRecipe"),
	TEXT("GTC.Character.SaveRecipe <FilePath> <SkeletonPath> [role] [seed] — save an authored character to a .character.json file"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* /*World*/)
	{
		if (!Args.IsValidIndex(0) || !Args.IsValidIndex(1))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.SaveRecipe <FilePath> <SkeletonPath> [role] [seed]"));
			return;
		}
		FCharacterRecipe Recipe;
		Recipe.SkeletonPath = Args[1];
		Recipe.Role = Args.IsValidIndex(2) ? GTCParseCharacterRole(Args[2]) : ECharacterRole::Pedestrian;
		Recipe.Seed = Args.IsValidIndex(3) ? FCString::Atoi(*Args[3]) : 0;
		Recipe.Name = FPaths::GetBaseFilename(Args[0]);

		if (GtcCharacterRecipe::SaveToFile(Recipe, Args[0]))
		{
			UE_LOG(LogGtcCharacterForge, Display, TEXT("Saved character recipe to '%s'."), *Args[0]);
		}
		else
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Failed to write '%s'."), *Args[0]);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharLoadRecipeCmd(
	TEXT("GTC.Character.LoadRecipe"),
	TEXT("GTC.Character.LoadRecipe <FilePath> — load a .character.json and spawn the wired character in front of the player"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.LoadRecipe <FilePath>"));
			return;
		}
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		FCharacterRecipe Recipe;
		if (!GtcCharacterRecipe::LoadFromFile(Args[0], Recipe))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load a valid recipe from '%s'."), *Args[0]);
			return;
		}
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Recipe.SkeletonPath);
		if (!Mesh)
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Recipe '%s' names a skeleton that won't load: '%s'."),
				*Recipe.Name, *Recipe.SkeletonPath);
			return;
		}

		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		if (const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
		{
			if (const APawn* P = PC->GetPawn())
			{
				Loc = P->GetActorLocation() + P->GetActorForwardVector() * 200.0f;
				Rot = FRotator(0.0f, P->GetActorRotation().Yaw + 180.0f, 0.0f);
			}
		}
		FCharacterWiringPlan Plan;
		ACharacter* Spawned = Forge->SpawnForged(Mesh, Recipe.Role, FTransform(Rot, Loc), Recipe.Seed, Plan);
		if (Spawned)
		{
			UE_LOG(LogGtcCharacterForge, Display, TEXT("Loaded recipe '%s' -> spawned '%s':"), *Recipe.Name, *Spawned->GetName());
			LogPlan(Plan, Spawned->GetName());
		}
		else
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Recipe '%s' loaded but spawn failed."), *Recipe.Name);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharAddCmd(
	TEXT("GTC.Character.Add"),
	TEXT("GTC.Character.Add <SkeletonPath> [role] [seed] [name] — append a character to the authored roster"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0))
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.Add <SkeletonPath> [role] [seed] [name]"));
			return;
		}
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		FCharacterRecipe R;
		R.SkeletonPath = Args[0];
		R.Role = Args.IsValidIndex(1) ? GTCParseCharacterRole(Args[1]) : ECharacterRole::Pedestrian;
		R.Seed = Args.IsValidIndex(2) ? FCString::Atoi(*Args[2]) : Forge->AuthoredRoster.Num();
		R.Name = Args.IsValidIndex(3) ? Args[3] : FString::Printf(TEXT("cast_%d"), Forge->AuthoredRoster.Num());
		Forge->AuthoredRoster.Add(R);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Added '%s' (%s) — roster now %d."),
			*R.Name, GTCCharacterRoleName(R.Role), Forge->AuthoredRoster.Num());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharRosterCmd(
	TEXT("GTC.Character.Roster"),
	TEXT("GTC.Character.Roster — list the authored roster"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Roster: %d character(s)"), Forge->AuthoredRoster.Num());
		for (int32 i = 0; i < Forge->AuthoredRoster.Num(); ++i)
		{
			const FCharacterRecipe& R = Forge->AuthoredRoster[i];
			UE_LOG(LogGtcCharacterForge, Display, TEXT("  #%d  \"%s\"  [%s]  seed=%d  %s"),
				i, *R.Name, GTCCharacterRoleName(R.Role), R.Seed, *R.SkeletonPath);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharDedupeCmd(
	TEXT("GTC.Character.Dedupe"),
	TEXT("GTC.Character.Dedupe — remove duplicate (skeleton, role) entries from the authored roster"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }
		const int32 Removed = GtcCharacterRecipe::Dedupe(Forge->AuthoredRoster);
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Dedupe removed %d duplicate(s); roster now %d."),
			Removed, Forge->AuthoredRoster.Num());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharSaveBankCmd(
	TEXT("GTC.Character.SaveBank"),
	TEXT("GTC.Character.SaveBank <FilePath> — save the authored roster to a .charbank.json file"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0)) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.SaveBank <FilePath>")); return; }
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }
		if (GtcCharacterRecipe::SaveBankToFile(Forge->AuthoredRoster, Args[0]))
		{
			UE_LOG(LogGtcCharacterForge, Display, TEXT("Saved %d character(s) to '%s'."), Forge->AuthoredRoster.Num(), *Args[0]);
		}
		else
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Failed to write '%s'."), *Args[0]);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharLoadBankCmd(
	TEXT("GTC.Character.LoadBank"),
	TEXT("GTC.Character.LoadBank <FilePath> — replace the roster with a saved .charbank.json"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!Args.IsValidIndex(0)) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("usage: GTC.Character.LoadBank <FilePath>")); return; }
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }
		if (GtcCharacterRecipe::LoadBankFromFile(Args[0], Forge->AuthoredRoster))
		{
			UE_LOG(LogGtcCharacterForge, Display, TEXT("Loaded %d character(s) from '%s'. Use GTC.Character.SpawnAll."),
				Forge->AuthoredRoster.Num(), *Args[0]);
		}
		else
		{
			UE_LOG(LogGtcCharacterForge, Warning, TEXT("Could not load a bank from '%s'."), *Args[0]);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharSpawnAllCmd(
	TEXT("GTC.Character.SpawnAll"),
	TEXT("GTC.Character.SpawnAll — spawn every character in the authored roster around the player"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		FVector Origin = FVector::ZeroVector;
		FVector Right = FVector(0, 1, 0);
		FVector Forward = FVector(1, 0, 0);
		if (const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
		{
			if (const APawn* P = PC->GetPawn())
			{
				Origin = P->GetActorLocation() + P->GetActorForwardVector() * 250.0f;
				Right = P->GetActorRightVector();
				Forward = P->GetActorForwardVector();
			}
		}

		int32 Spawned = 0;
		for (int32 i = 0; i < Forge->AuthoredRoster.Num(); ++i)
		{
			const FCharacterRecipe& R = Forge->AuthoredRoster[i];
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *R.SkeletonPath);
			if (!Mesh)
			{
				UE_LOG(LogGtcCharacterForge, Warning, TEXT("  '%s': skeleton '%s' won't load — skipped."), *R.Name, *R.SkeletonPath);
				continue;
			}
			// Lay the cast out in a row, centred, facing the player.
			const float Offset = (static_cast<float>(i) - 0.5f * (Forge->AuthoredRoster.Num() - 1)) * 120.0f;
			const FVector Loc = Origin + Right * Offset;
			const FRotator Rot = (-Forward).Rotation();
			FCharacterWiringPlan Plan;
			if (Forge->SpawnForged(Mesh, R.Role, FTransform(Rot, Loc), R.Seed, Plan))
			{
				++Spawned;
			}
		}
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Spawned %d / %d roster character(s)."), Spawned, Forge->AuthoredRoster.Num());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCharAuditAllCmd(
	TEXT("GTC.Character.AuditAll"),
	TEXT("GTC.Character.AuditAll — ship-readiness check every character in the authored roster"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
	{
		UGTCCharacterForge* Forge = ResolveForge(World);
		if (!Forge) { UE_LOG(LogGtcCharacterForge, Warning, TEXT("No Character Forge subsystem.")); return; }

		int32 Shippable = 0;
		int32 Missing = 0;
		for (int32 i = 0; i < Forge->AuthoredRoster.Num(); ++i)
		{
			const FCharacterRecipe& R = Forge->AuthoredRoster[i];
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *R.SkeletonPath);
			if (!Mesh)
			{
				++Missing;
				UE_LOG(LogGtcCharacterForge, Warning, TEXT("  #%d \"%s\": MISSING skeleton '%s'"),
					i, *R.Name, *R.SkeletonPath);
				continue;
			}
			const TArray<FString> Bones = UGTCCharacterForge::BoneNamesOf(Mesh->GetSkeleton());
			const FSkeletonProbe Probe = FSkeletonProbe::FromBoneNames(Bones);
			const FCharacterReadiness Ready = GTCAssessReadiness(Probe, Bones, R.Role);
			if (Ready.bShippable) { ++Shippable; }
			UE_LOG(LogGtcCharacterForge, Display, TEXT("  #%d \"%s\": %s"), i, *R.Name, *Ready.Summary());
		}
		UE_LOG(LogGtcCharacterForge, Display, TEXT("Roster readiness: %d/%d shippable%s."),
			Shippable, Forge->AuthoredRoster.Num(),
			Missing > 0 ? *FString::Printf(TEXT(" (%d missing skeleton)"), Missing) : TEXT(""));
	}));
