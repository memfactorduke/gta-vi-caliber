// Copyright Epic Games, Inc. All Rights Reserved.

#include "ResidencyBudget.h"

int64 FResidencyBudget::TotalBytes(const TArray<FResidentTile>& Resident)
{
	int64 Total = 0;
	for (const FResidentTile& Tile : Resident)
	{
		Total += Tile.Bytes;
	}
	return Total;
}

bool FResidencyBudget::IsOverBudget(const TArray<FResidentTile>& Resident, int64 BudgetBytes)
{
	return TotalBytes(Resident) > BudgetBytes;
}

TArray<int32> FResidencyBudget::SelectEvictions(const TArray<FResidentTile>& Resident, int64 BytesToFree)
{
	TArray<int32> Evict;
	if (BytesToFree <= 0)
	{
		return Evict;
	}

	// Order candidates by eviction preference: highest key first, then larger
	// byte size (free more per eviction), then lower index for determinism.
	TArray<int32> Order;
	Order.Reserve(Resident.Num());
	for (int32 I = 0; I < Resident.Num(); ++I)
	{
		Order.Add(I);
	}
	Order.Sort([&Resident](int32 A, int32 B)
	{
		const FResidentTile& Ta = Resident[A];
		const FResidentTile& Tb = Resident[B];
		if (Ta.EvictionKey != Tb.EvictionKey)
		{
			return Ta.EvictionKey > Tb.EvictionKey;
		}
		if (Ta.Bytes != Tb.Bytes)
		{
			return Ta.Bytes > Tb.Bytes;
		}
		return A < B;
	});

	int64 Freed = 0;
	for (int32 Idx : Order)
	{
		if (Freed >= BytesToFree)
		{
			break;
		}
		Evict.Add(Idx);
		Freed += Resident[Idx].Bytes;
	}
	return Evict;
}

TArray<int32> FResidencyBudget::EvictionsToAdmit(const TArray<FResidentTile>& Resident, int64 IncomingBytes, int64 BudgetBytes)
{
	const int64 BytesToFree = TotalBytes(Resident) + IncomingBytes - BudgetBytes;
	return SelectEvictions(Resident, BytesToFree);
}
