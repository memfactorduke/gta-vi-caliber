// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcHail.h"
#include "../NpcSeqMath.h"

namespace
{
	using GtcSeq::PosMod;

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
			// Wave 2 trades.
			{ TEXT("lifeguard"), {
				TEXT("Stay where I can see you, swimmer."),
				TEXT("Tide's turning, keep it knee-deep, yeah?"),
				TEXT("Reapply that sunscreen. I'm watching."),
			}},
			{ TEXT("electrician"), {
				TEXT("Mind the wire, this sign bites back."),
				TEXT("Give it a sec, the 'O' always flickers."),
				TEXT("Don't touch that. Trust me, don't."),
			}},
			{ TEXT("courier"), {
				TEXT("Order up? No? Then mind the scooter."),
				TEXT("Heads up, coming through, food's getting cold."),
				TEXT("Know a faster way to Fourth? Asking for a meal."),
			}},
			{ TEXT("caricaturist"), {
				TEXT("Hold still, I'll make your nose famous."),
				TEXT("Ten bucks, two minutes, one unforgettable chin."),
				TEXT("That face? That's a portrait waiting to happen."),
			}},
			{ TEXT("tour_guide"), {
				TEXT("See that cornice? Pure 1937 pastel glory."),
				TEXT("Step this way, history's on the next block."),
				TEXT("Fun fact: everything here is older than it looks."),
			}},
			{ TEXT("fishmonger"), {
				TEXT("Fresh catch, half price, ignore the pelican."),
				TEXT("You want it on ice? Course you do."),
				TEXT("Best fish on the strip, and that bird agrees."),
			}},
			{ TEXT("paramedic"), {
				TEXT("Easy on the curb, I'd hate to circle back."),
				TEXT("You good? Breathing? Great, carry on."),
				TEXT("On wheels here, give me a lane, friend."),
			}},
			{ TEXT("bartender"), {
				TEXT("First one's strong, second one's a duet."),
				TEXT("Grab a stool, your song's already cued."),
				TEXT("Water's free, regrets are extra."),
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
