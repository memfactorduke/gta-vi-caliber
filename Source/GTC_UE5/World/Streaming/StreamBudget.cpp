// Copyright Epic Games, Inc. All Rights Reserved.

#include "StreamBudget.h"

bool FStreamBudget::CanAdmit(int32 AdmittedSoFar, double SpentCost, double NextCost, int32 MaxOps, double MaxCostPerTick)
{
	if (MaxOps <= 0 || AdmittedSoFar >= MaxOps)
	{
		return false;
	}
	if (AdmittedSoFar == 0)
	{
		return true; // forward progress: first item always admitted this tick
	}
	return SpentCost + NextCost <= MaxCostPerTick;
}

int32 FStreamBudget::AdmitCount(const TArray<double>& CostsInPriorityOrder, int32 MaxOps, double MaxCostPerTick)
{
	double Spent = 0.0;
	int32 Admitted = 0;
	for (double Cost : CostsInPriorityOrder)
	{
		if (!CanAdmit(Admitted, Spent, Cost, MaxOps, MaxCostPerTick))
		{
			break;
		}
		Spent += Cost;
		++Admitted;
	}
	return Admitted;
}
