// Copyright (c) 2026 GTC contributors

#include "AmbientEvents.h"

#include <limits>

namespace
{
	// the reference -INF sentinel for "never fired". Negative infinity so any finite Now - LastFired
	// comparison passes the cooldown/global-gap check after a Reset() or on a fresh roster.
	constexpr double NegInf = -std::numeric_limits<double>::infinity();
}

FAmbientEvents::FAmbientEvents(const TArray<FEventDef>& Events)
	: LastAnyFired(NegInf)
{
	const TArray<FEventDef>& Source = Events.Num() > 0 ? Events : DefaultEvents();
	for (const FEventDef& Entry : Source)
	{
		Register(Entry);
	}
}

TArray<FAmbientEvents::FEventDef> FAmbientEvents::DefaultEvents()
{
	TArray<FEventDef> Out;
	Out.Add({TEXT("mugging"), TEXT("crime"), 3.0, 0, 2, FString(), 60.0});
	Out.Add({TEXT("stranded_motorist"), TEXT("help"), 2.0, 0, 0, FString(), 90.0});
	Out.Add({TEXT("street_race"), TEXT("race"), 2.0, 0, 1, FString(), 120.0});
	Out.Add({TEXT("getaway_driver"), TEXT("crime"), 1.5, 1, 3, FString(), 120.0});
	Out.Add({TEXT("gang_shootout"), TEXT("combat"), 1.5, 2, 5, TEXT("docks"), 150.0});
	Out.Add({TEXT("security_van"), TEXT("heist"), 1.0, 0, 2, FString(), 180.0});
	return Out;
}

int32 FAmbientEvents::EventCount() const
{
	return Entries.Num();
}

bool FAmbientEvents::HasEvent(const FString& Id) const
{
	return Index.Contains(Id);
}

TArray<FString> FAmbientEvents::Ids() const
{
	TArray<FString> Out;
	Out.Reserve(Entries.Num());
	for (const FEntry& E : Entries)
	{
		Out.Add(E.Id);
	}
	return Out;
}

FString FAmbientEvents::KindOf(const FString& Id) const
{
	const FEntry* E = Find(Id);
	if (E == nullptr)
	{
		return FString();
	}
	return E->Kind;
}

bool FAmbientEvents::CanFire(const FString& Id, double Now, const FAmbientContext& Context) const
{
	const FEntry* E = Find(Id);
	if (E == nullptr)
	{
		return false;
	}
	// the reference int(context.get("stars", 0)) truncates toward zero; (int32) cast does the same.
	const int32 Stars = static_cast<int32>(Context.Stars);
	if (Stars < E->MinStars || Stars > E->MaxStars)
	{
		return false;
	}
	if (!E->District.IsEmpty() && E->District != Context.District)
	{
		return false;
	}
	return Now - E->LastFired >= E->Cooldown;
}

TArray<FString> FAmbientEvents::EligibleIds(double Now, const FAmbientContext& Context) const
{
	TArray<FString> Out;
	for (const FEntry& E : Entries)
	{
		if (CanFire(E.Id, Now, Context))
		{
			Out.Add(E.Id);
		}
	}
	return Out;
}

FString FAmbientEvents::TriggerNext(FRandomStream& Rng, double Now, const FAmbientContext& Context)
{
	if (Now - LastAnyFired < GlobalGap)
	{
		return FString();
	}
	const TArray<FString> Eligible = EligibleIds(Now, Context);
	if (Eligible.Num() == 0)
	{
		return FString();
	}
	const FString Id = WeightedPick(Rng, Eligible);
	if (FEntry* E = Find(Id))
	{
		E->LastFired = Now;
	}
	LastAnyFired = Now;
	return Id;
}

FString FAmbientEvents::TriggerNextNoRng()
{
	// Godot: `if rng == null ...: return ""` — no state change, fires nothing.
	return FString();
}

void FAmbientEvents::Trigger(const FString& Id, double Now)
{
	if (FEntry* E = Find(Id))
	{
		E->LastFired = Now;
		LastAnyFired = Now;
	}
}

double FAmbientEvents::LastFiredOf(const FString& Id) const
{
	const FEntry* E = Find(Id);
	if (E == nullptr)
	{
		return NegInf;
	}
	return E->LastFired;
}

void FAmbientEvents::Reset()
{
	LastAnyFired = NegInf;
	for (FEntry& E : Entries)
	{
		E.LastFired = NegInf;
	}
}

FString FAmbientEvents::WeightedPick(FRandomStream& Rng, const TArray<FString>& Eligible) const
{
	double Total = 0.0;
	for (const FString& Id : Eligible)
	{
		if (const FEntry* E = Find(Id))
		{
			Total += E->Weight;
		}
	}
	double Roll = Rng.GetFraction() * Total;
	for (const FString& Id : Eligible)
	{
		if (const FEntry* E = Find(Id))
		{
			Roll -= E->Weight;
		}
		if (Roll <= 0.0)
		{
			return Id;
		}
	}
	return Eligible[Eligible.Num() - 1];
}

void FAmbientEvents::Register(const FEventDef& Def)
{
	if (Def.Id.IsEmpty() || Index.Contains(Def.Id))
	{
		return;
	}
	if (Def.Weight <= 0.0)
	{
		return;
	}
	FEntry Entry;
	Entry.Id = Def.Id;
	Entry.Kind = Def.Kind;
	Entry.Weight = Def.Weight;
	Entry.MinStars = Def.MinStars;
	Entry.MaxStars = Def.MaxStars;
	Entry.District = Def.District;
	Entry.Cooldown = FMath::Max(0.0, Def.Cooldown);
	Entry.LastFired = NegInf;

	const int32 NewIndex = Entries.Add(Entry);
	Index.Add(Def.Id, NewIndex);
}

const FAmbientEvents::FEntry* FAmbientEvents::Find(const FString& Id) const
{
	const int32* Idx = Index.Find(Id);
	return Idx != nullptr ? &Entries[*Idx] : nullptr;
}

FAmbientEvents::FEntry* FAmbientEvents::Find(const FString& Id)
{
	const int32* Idx = Index.Find(Id);
	return Idx != nullptr ? &Entries[*Idx] : nullptr;
}
