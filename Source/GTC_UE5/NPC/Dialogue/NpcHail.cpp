// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcHail.h"

namespace
{
	int32 PosMod(int32 Value, int32 Modulus)
	{
		if (Modulus <= 0)
		{
			return 0;
		}
		return ((Value % Modulus) + Modulus) % Modulus;
	}

	// Role -> player-directed hail lines. Keyed by FNpcOccupation::RoleFor tokens.
	// The mime has no bank (stays silent); a non-job citizen never reaches here.
	const TMap<FName, TArray<FString>>& Banks()
	{
		static const TMap<FName, TArray<FString>> B = {
			{ TEXT("barista"), {
				TEXT("Latte? Might be your last one, friend."),
				TEXT("Come in, beat the rush. And the apocalypse."),
				TEXT("You look like a cold-brew kind of doom."),
			}},
			{ TEXT("influencer"), {
				TEXT("You're on camera! Say something iconic!"),
				TEXT("Hey — wanna be in my pigeon vlog?"),
				TEXT("Don't mind me, just going viral."),
			}},
			{ TEXT("crossing_guard"), {
				TEXT("Use the crosswalk! I BELIEVE in you."),
				TEXT("Hold it right there, friend."),
				TEXT("Eyes up, feet steady. We cross as one."),
			}},
			{ TEXT("vendor"), {
				TEXT("Hot dog? Cash only, the cards aren't safe."),
				TEXT("Step right up — ignore the pigeons."),
				TEXT("One dollar! They can't silence flavor!"),
			}},
			{ TEXT("yogi"), {
				TEXT("Breathe with me. I insist."),
				TEXT("Your shoulders are up by your ears, friend."),
				TEXT("Find your center. It's right there. FIND it."),
			}},
			{ TEXT("stunt_double"), {
				TEXT("Watch the curb — I've broken worse."),
				TEXT("Stay loose. You never know."),
				TEXT("I'd roll if I were you."),
			}},
			{ TEXT("intern"), {
				TEXT("Need anything? Coffee? I'm SO caffeinated."),
				TEXT("Can't talk, deadline, hi, bye!"),
				TEXT("Is it Friday? Please be Friday."),
			}},
			{ TEXT("dog_walker"), {
				TEXT("Mind the leashes — all seven of them."),
				TEXT("Say hi to the pack."),
				TEXT("They're judging you. Lovingly."),
			}},
			{ TEXT("food_critic"), {
				TEXT("You look like a two-star palate. Prove me wrong."),
				TEXT("Don't eat there. Trust me."),
				TEXT("Everything here is... fine. Tragic."),
			}},
			{ TEXT("life_coach"), {
				TEXT("YOU. Yes, you. Ready to change your life?"),
				TEXT("I see potential. Forty bucks unlocks it."),
				TEXT("Crush today. Then crush tomorrow."),
			}},
			{ TEXT("weather_anchor"), {
				TEXT("Forecast for you: bright, chance of greatness."),
				TEXT("Humid out — stay hydrated. Back to you."),
				TEXT("Live from this corner: you're looking good!"),
			}},
		};
		return B;
	}
}

int32 FNpcHail::Count(FName Role)
{
	const TArray<FString>* Bank = Banks().Find(Role);
	return Bank ? Bank->Num() : 0;
}

FString FNpcHail::Line(FName Role, int32 Index)
{
	const TArray<FString>* Bank = Banks().Find(Role);
	if (!Bank || Bank->Num() == 0)
	{
		return FString();
	}
	return (*Bank)[PosMod(Index, Bank->Num())];
}
