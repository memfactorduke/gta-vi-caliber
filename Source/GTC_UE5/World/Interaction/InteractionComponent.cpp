// Copyright Epic Games, Inc. All Rights Reserved.

#include "InteractionComponent.h"

#include "Interaction.h"   // merged FInteraction selection model (W1-tested) — reused, not re-ported
#include "Interactable.h"  // IGTCInteractable contract

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "WorldCollision.h"

UGTCInteractionComponent::UGTCInteractionComponent()
{
    // Default to ticking so the selection refreshes each frame; the gather is a
    // cheap forward overlap. Owners that prefer on-demand can disable tick and
    // call UpdateSelection() themselves.
    PrimaryComponentTick.bCanEverTick = true;

    // Placeholder reach gesture so press-E plays a visible motion today; it's an
    // existing Mannequin-skeleton clip, so it works on any character model. Swap
    // for a dedicated door-push clip later via this property — no code change.
    InteractAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Pistol/"
             "MM_Pistol_Equip.MM_Pistol_Equip")));
}

int32 UGTCInteractionComponent::SelectBest(const TArray<FGtcInteractionCandidate>& Candidates,
    const FVector& From, double Reach)
{
    // Build the geometry the merged FInteraction model scores, but ONLY from the
    // can-interact subset — gating excludes a candidate rather than re-scoring it.
    // We keep the original indices so the returned pick refers to the caller's
    // array, and so ties still resolve to the lower ORIGINAL index (the subset
    // preserves order, and FInteraction::Nearest already ties to the lower subset
    // index, which maps back to the lower original index).
    TArray<FVector> Points;
    TArray<int32> OriginalIndex;
    Points.Reserve(Candidates.Num());
    OriginalIndex.Reserve(Candidates.Num());
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        if (Candidates[Index].bCanInteract)
        {
            Points.Add(Candidates[Index].Location);
            OriginalIndex.Add(Index);
        }
    }

    // The pick is exactly the W1-tested FInteraction math over the gated subset.
    const int32 SubsetWinner = FInteraction::Nearest(Points, From, Reach);
    if (SubsetWinner == FInteraction::None)
    {
        return None;
    }
    return OriginalIndex[SubsetWinner];
}

FText UGTCInteractionComponent::GetSelectedPrompt() const
{
    if (SelectedTarget != nullptr && SelectedTarget->Implements<UGTCInteractable>())
    {
        return IGTCInteractable::Execute_GetInteractPrompt(SelectedTarget);
    }
    return FText::GetEmpty();
}

bool UGTCInteractionComponent::TriggerInteract()
{
    AActor* Target = SelectedTarget;
    if (Target == nullptr || !Target->Implements<UGTCInteractable>())
    {
        return false;
    }

    AActor* Instigator = GetOwner();
    // Re-check the gate at fire time: the world may have changed since selection.
    if (!IGTCInteractable::Execute_CanInteract(Target, Instigator))
    {
        return false;
    }

    IGTCInteractable::Execute_Interact(Target, Instigator);

    // The interact landed — play the owning character's reach/push gesture. This
    // is the only place animation enters: the interactable itself stays
    // anim-ignorant, and the gesture plays on whatever model the owner wears.
    PlayInteractGesture();
    return true;
}

void UGTCInteractionComponent::PlayInteractGesture()
{
    UAnimSequence* Seq = InteractAnim.LoadSynchronous();
    if (Seq == nullptr)
    {
        return; // no clip assigned — nothing to play (stays compilable/no-op)
    }

    // Prefer the character's canonical body mesh (GetMesh) so face/hair follower
    // meshes don't get picked; fall back to any skeletal mesh for non-Character
    // owners. Model-agnostic: the clip rides the shared "DefaultSlot".
    USkeletalMeshComponent* Body = nullptr;
    if (const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        Body = OwnerChar->GetMesh();
    }
    if (Body == nullptr && GetOwner() != nullptr)
    {
        Body = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    }
    if (Body == nullptr)
    {
        return;
    }

    if (UAnimInstance* AnimInstance = Body->GetAnimInstance())
    {
        AnimInstance->PlaySlotAnimationAsDynamicMontage(Seq, FName(TEXT("DefaultSlot")), 0.1f, 0.1f);
    }
}

void UGTCInteractionComponent::UpdateSelection()
{
    // DEFERRED-TO-PIE: GatherCandidates runs the live overlap/trace against the
    // owner's UWorld; it needs a real world so it is verified in PIE, not in the
    // headless unit tests. The pick it ends in (SelectBest) IS unit-tested.
    TArray<FGtcInteractionCandidate> Candidates;
    FVector From = FVector::ZeroVector;
    GatherCandidates(Candidates, From);

    const int32 Best = SelectBest(Candidates, From, static_cast<double>(InteractReach));
    SetSelectedTarget(Best == None ? nullptr : Candidates[Best].Actor.Get());
}

void UGTCInteractionComponent::GatherCandidates(
    TArray<FGtcInteractionCandidate>& OutCandidates, FVector& OutFrom) const
{
    OutCandidates.Reset();

    const AActor* Owner = GetOwner();
    const UWorld* World = GetWorld();
    if (Owner == nullptr || World == nullptr)
    {
        return;
    }

    OutFrom = Owner->GetActorLocation();
    AActor* MutableOwner = GetOwner();

    // Forward sphere overlap around the owner, radius = InteractReach. The
    // selection (SelectBest) does the in-reach + nearest scoring via FInteraction,
    // so the overlap radius is just the broad-phase cull.
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCInteractionGather), /*bTraceComplex=*/false, Owner);
    World->OverlapMultiByObjectType(
        Overlaps,
        OutFrom,
        FQuat::Identity,
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeSphere(InteractReach),
        Params);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (Candidate == nullptr || Candidate == MutableOwner)
        {
            continue;
        }
        if (!Candidate->Implements<UGTCInteractable>())
        {
            continue;
        }

        FGtcInteractionCandidate Desc;
        Desc.Actor = Candidate;
        Desc.Location = Candidate->GetActorLocation();
        Desc.bCanInteract = IGTCInteractable::Execute_CanInteract(Candidate, MutableOwner);
        OutCandidates.Add(MoveTemp(Desc));
    }
}

void UGTCInteractionComponent::SetSelectedTarget(AActor* NewTarget)
{
    if (NewTarget == SelectedTarget)
    {
        return;
    }
    SelectedTarget = NewTarget;
    OnSelectedTargetChanged.Broadcast(NewTarget);
}

void UGTCInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateSelection();
}
