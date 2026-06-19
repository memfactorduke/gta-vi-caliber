// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcAcquaintance.h"

void FNpcAcquaintance::Meet(int32 OtherId, float Amount)
{
	if (OtherId == INDEX_NONE)
	{
		return; // anonymous encounter — nobody to remember
	}

	// Already known? Bump and cap.
	for (FNpcAcquaintanceEntry& E : Entries)
	{
		if (E.Id == OtherId)
		{
			E.Familiarity = FMath::Min(E.Familiarity + Amount, MaxFamiliarity);
			return;
		}
	}

	// New face. If the book is full, forget the least-familiar person to make room
	// — a passing stranger can't displace someone you actually know well.
	if (Entries.Num() >= Capacity)
	{
		int32 WeakestIdx = 0;
		for (int32 i = 1; i < Entries.Num(); ++i)
		{
			if (Entries[i].Familiarity < Entries[WeakestIdx].Familiarity)
			{
				WeakestIdx = i;
			}
		}
		Entries.RemoveAtSwap(WeakestIdx);
	}

	FNpcAcquaintanceEntry New;
	New.Id = OtherId;
	New.Familiarity = FMath::Min(Amount, MaxFamiliarity);
	Entries.Add(New);
}

float FNpcAcquaintance::FamiliarityWith(int32 OtherId) const
{
	if (OtherId == INDEX_NONE)
	{
		return 0.0f;
	}
	for (const FNpcAcquaintanceEntry& E : Entries)
	{
		if (E.Id == OtherId)
		{
			return E.Familiarity;
		}
	}
	return 0.0f;
}

ENpcFamiliarity FNpcAcquaintance::TierWith(int32 OtherId) const
{
	return TierFor(FamiliarityWith(OtherId));
}

void FNpcAcquaintance::Forget(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}
	for (int32 i = Entries.Num() - 1; i >= 0; --i)
	{
		Entries[i].Familiarity -= Amount;
		if (Entries[i].Familiarity <= 0.0f)
		{
			Entries.RemoveAtSwap(i);
		}
	}
}

ENpcFamiliarity FNpcAcquaintance::TierFor(float Familiarity)
{
	if (Familiarity >= FriendThreshold)
	{
		return ENpcFamiliarity::Friend;
	}
	if (Familiarity >= FamiliarThreshold)
	{
		return ENpcFamiliarity::FamiliarFace;
	}
	return ENpcFamiliarity::Stranger;
}
