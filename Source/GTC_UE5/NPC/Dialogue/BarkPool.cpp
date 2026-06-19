// Copyright (c) 2026 GTC contributors

#include "BarkPool.h"

namespace
{
	// LINES dictionary, keyed by situation, preserving the reference author order.
	const TMap<EBarkSituation, TArray<FString>>& BarkPoolLines()
	{
		static const TMap<EBarkSituation, TArray<FString>> L = {
			{ EBarkSituation::IDLE, {
				TEXT("Lovely day for it."),
				TEXT("Where did I park?"),
				TEXT("Running late again."),
				TEXT("I need a coffee."),
				TEXT("Nice weather, huh?"),
				TEXT("...so then I told him no."),
			}},
			{ EBarkSituation::ALARMED, {
				TEXT("What was that?!"),
				TEXT("Did you hear that?"),
				TEXT("Hey — watch it!"),
				TEXT("Everything okay?"),
			}},
			{ EBarkSituation::FLEE, {
				TEXT("He's got a gun!"),
				TEXT("Run!"),
				TEXT("Somebody help!"),
				TEXT("Get down!"),
				TEXT("Call the cops!"),
			}},
		};
		return L;
	}
}

FString FBarkPool::Line(EBarkSituation Situation, int32 Index)
{
	const TArray<FString>* Pool = BarkPoolLines().Find(Situation);
	if (!Pool || Pool->Num() == 0)
	{
		return FString();
	}
	// the reference posmod(index, pool.size()): always non-negative in [0, size).
	const int32 N = Pool->Num();
	const int32 WrappedIndex = ((Index % N) + N) % N;
	return (*Pool)[WrappedIndex];
}

int32 FBarkPool::Count(EBarkSituation Situation)
{
	const TArray<FString>* Pool = BarkPoolLines().Find(Situation);
	return Pool ? Pool->Num() : 0;
}

bool FBarkPool::ShouldBark(double TimeSinceLast, double Cooldown)
{
	return TimeSinceLast >= Cooldown;
}
