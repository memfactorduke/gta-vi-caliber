// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcDialogue.h"

namespace
{
	// Godot posmod(a, n): result is always non-negative in [0, n). Uses 64-bit
	// intermediates to mirror GDScript int (64-bit) for the `seed*7+1` / `seed*13+2`
	// index arithmetic that would otherwise overflow int32 at large seeds.
	int32 DialoguePosmod(int64 A, int32 N)
	{
		const int64 N64 = static_cast<int64>(N);
		return static_cast<int32>(((A % N64) + N64) % N64);
	}

	// voice -> context -> lines (BANKS). Built once; preserves Godot author order.
	const TMap<FString, TMap<FString, TArray<FString>>>& DialogueBanks()
	{
		static const TMap<FString, TMap<FString, TArray<FString>>> B = []
		{
			TMap<FString, TMap<FString, TArray<FString>>> M;

			M.Add(TEXT("doomsday"), {
				{ TEXT("idle"), {
					TEXT("The foam spelled a warning this morning. It said 'low tip'."),
					TEXT("Enjoy the sunlight. I've read the espresso. I've seen things."),
					TEXT("We're all just beans, waiting to be ground. Anyway, oat milk?"),
				}},
				{ TEXT("work"), {
					TEXT("This is a medium. It is also an omen."),
					TEXT("Your order is ready. So, in a cosmic sense, is the end."),
				}},
				{ TEXT("see_player"), { TEXT("You. The grounds mentioned you. Wear a scarf.") }},
			});

			M.Add(TEXT("influencer"), {
				{ TEXT("idle"), {
					TEXT("Okay but the LIGHTING on {noun} right now? Unreal. Like and subscribe."),
					TEXT("Guys. GUYS. We just hit four followers. We're basically a movement."),
					TEXT("Don't mind me, just getting B-roll of {animal} for the brand."),
				}},
				{ TEXT("work"), { TEXT("Smash that follow for more content about {place}.") }},
				{ TEXT("see_player"), { TEXT("Wait, are you filming me? No? ...Could you?") }},
			});

			M.Add(TEXT("method_actor"), {
				{ TEXT("idle"), {
					TEXT("I am not 'a guy with a sign'. I AM the sign."),
					TEXT("To truly halt traffic, one must first halt oneself. Spiritually."),
				}},
				{ TEXT("work"), { TEXT("STOP. (I have waited my whole life to mean it this much.)") }},
				{ TEXT("see_player"), { TEXT("Don't break my concentration. I'm a crosswalk now.") }},
			});

			M.Add(TEXT("conspiracy"), {
				{ TEXT("idle"), {
					TEXT("Hot dogs? Sure. But have you ever seen one CHARGE its battery?"),
					TEXT("The pigeons aren't real. {animal} told me. {animal} is also not real."),
					TEXT("They put fluoride in the relish. Wake up. Mustard?"),
				}},
				{ TEXT("work"), { TEXT("That'll be six bucks. Cash. The card readers LISTEN.") }},
				{ TEXT("see_player"), { TEXT("You're either with me or you're {noun} in a trench coat.") }},
				{ TEXT("chat"), { TEXT("Don't look now, but that lamppost has been TAKING NOTES. Anyway, how are you?") }},
			});

			M.Add(TEXT("yogi"), {
				{ TEXT("idle"), {
					TEXT("Breathe in. Now hold it. Hold it. ...You may release it in 2027."),
					TEXT("Your aura is double-parked. We will be fixing that. Together."),
				}},
				{ TEXT("work"), { TEXT("Inhale calm. Exhale {noun}. Namaste, aggressively.") }},
				{ TEXT("see_player"), { TEXT("I can sense your tension from here. It owes me money.") }},
				{ TEXT("chat"), { TEXT("Mm. I hear your words, but your CHAKRAS are mumbling. Speak up, chakras.") }},
			});

			M.Add(TEXT("stunt_double"), {
				{ TEXT("idle"), {
					TEXT("Stairs? In THIS economy? I roll down those now."),
					TEXT("Sorry, reflex. I see a curb, I prepare to be set on fire."),
				}},
				{ TEXT("flee"), { TEXT("AND WE'RE RUNNING — this is the good part — protect the face!") }},
				{ TEXT("see_player"), { TEXT("Don't worry about me, I've fallen off {place}. On purpose.") }},
			});

			M.Add(TEXT("mime"), {
				{ TEXT("idle"), {
					TEXT("..."),
					TEXT("(gestures at an invisible wall, deeply moved)"),
					TEXT("(silent, but judging)"),
				}},
				{ TEXT("see_player"), { TEXT("(points at you, then at an invisible rope, urgently)") }},
				{ TEXT("chat"), { TEXT("(nods vigorously, mimes a tiny violin, points at the sky, weeps)") }},
			});

			M.Add(TEXT("intern"), {
				{ TEXT("idle"), {
					TEXT("I've been awake since Tuesday and I have NEVER felt more employable."),
					TEXT("Is this load-bearing? Is ME load-bearing? Anyway I reorganized {place}."),
				}},
				{ TEXT("work"), { TEXT("Synergy! I don't know what it means but I did SO much of it.") }},
				{ TEXT("see_player"), { TEXT("Do you need anything? A coffee? A merger? I'm so ready.") }},
			});

			M.Add(TEXT("philosopher"), {
				{ TEXT("idle"), {
					TEXT("If a dog walks a man, is anyone truly on a leash? ...Steve, heel."),
					TEXT("We are all just {animal}, briefly convinced we have a schedule."),
				}},
				{ TEXT("see_player"), { TEXT("You there — do you exist, or are you also avoiding emails?") }},
				{ TEXT("chat"), { TEXT("But IS a hot dog a sandwich, or is the sandwich us? ...Steve. STEVE. Heel.") }},
			});

			M.Add(TEXT("food_critic"), {
				{ TEXT("idle"), {
					TEXT("This bench: two stars. Ambitious silhouette, no follow-through."),
					TEXT("The air today has notes of bus and regret. I've had worse Tuesdays."),
				}},
				{ TEXT("eat"), { TEXT("I'll allow it. Three stars. The pickle showed real courage.") }},
				{ TEXT("see_player"), { TEXT("Your vibe? Unseasoned. But promising. I'll allow it.") }},
				{ TEXT("chat"), { TEXT("This conversation: a bold three stars. Slightly overcooked, but the texture? Fun.") }},
			});

			M.Add(TEXT("life_coach"), {
				{ TEXT("idle"), {
					TEXT("You didn't wake up today to be MEDIUM. That'll be $40."),
					TEXT("Your only competition is the {noun} you were yesterday. Believe it."),
				}},
				{ TEXT("see_player"), { TEXT("I see GREATNESS in you. Also an invoice. Mostly greatness.") }},
			});

			M.Add(TEXT("weather"), {
				{ TEXT("idle"), {
					TEXT("And it's a balmy sidewalk out there, folks, with a 70% chance of {animal}."),
					TEXT("Live, from this exact spot: skies are doing their absolute best."),
				}},
				{ TEXT("work"), { TEXT("Doppler says feelings later, clearing to mild smugness by five.") }},
				{ TEXT("see_player"), { TEXT("You heard it here first: YOU are the storm front, champ.") }},
			});

			return M;
		}();
		return B;
	}

	// The safety net (GENERIC) — any voice missing a context borrows these.
	const TMap<FString, TArray<FString>>& DialogueGeneric()
	{
		static const TMap<FString, TArray<FString>> G = {
			{ TEXT("idle"), { TEXT("Hm."), TEXT("Another day in {place}, I guess."), TEXT("I should really call {noun}.") }},
			{ TEXT("work"), { TEXT("Working, working, always working."), TEXT("This job and I have an understanding.") }},
			{ TEXT("eat"), { TEXT("Mmh. Food."), TEXT("I would die for this sandwich, conditionally.") }},
			{ TEXT("sleep"), { TEXT("Zzz..."), TEXT("Five more minutes. Or years.") }},
			{ TEXT("socialize"), { TEXT("So anyway, that's my whole thing about {noun}."), TEXT("Cheers, I guess!") }},
			{ TEXT("goof_off"), { TEXT("I'm not procrastinating, I'm marinating."), TEXT("Wheee, sort of.") }},
			{ TEXT("commute"), { TEXT("Move, MOVE — oh. Sorry. We're all in this."), TEXT("Another commute, another me.") }},
			{ TEXT("see_player"), { TEXT("Oh, hey."), TEXT("Do I know you? No? Cool, cool."), TEXT("Watch where you're vibing.") }},
			{ TEXT("flee"), { TEXT("NOPE."), TEXT("This is a problem for running-me!"), TEXT("I respect violence too much to stay!") }},
			{ TEXT("gawk"), { TEXT("Are you SEEING this?!"), TEXT("Somebody's having a day."), TEXT("I'm not staring, you're staring.") }},
			{ TEXT("bump"), { TEXT("Hey! Walkin' here!"), TEXT("Personal space is a {place}, my friend."), TEXT("Oof. Rude.") }},
			{ TEXT("greet"), {
				TEXT("Oh — hi! We're both just. Out here. Existing."),
				TEXT("Hey, you! Big fan of {noun}, by the way."),
				TEXT("Morning! Or — is it? Time is {place} to me now."),
			}},
			{ TEXT("chat"), {
				TEXT("So I told them, I said, '{noun} is not a personality.' They disagreed."),
				TEXT("Anyway, long story short, I no longer trust {animal}."),
				TEXT("You ever just think about {place}? No? ...Just me, then."),
				TEXT("Totally. Totally. ...Wait, what are we agreeing about?"),
				TEXT("And THAT'S why I don't do Tuesdays anymore."),
				TEXT("Mm-hm. Mm-hm. Big if true. What were you saying?"),
			}},
		};
		return G;
	}

	// Weather commentary by sky condition (everyone but the anchor).
	const TMap<FString, TArray<FString>>& DialogueWeather()
	{
		static const TMap<FString, TArray<FString>> W = {
			{ TEXT("clear"), { TEXT("Gorgeous out. Suspiciously gorgeous. What does it WANT."), TEXT("Not a cloud. Bliss.") }},
			{ TEXT("cloudy"), { TEXT("Bit grey. Like {place}, but as a sky."), TEXT("Clouds. Nature's passive aggression.") }},
			{ TEXT("overcast"), {
				TEXT("This sky owes me a refund."),
				TEXT("Real 'the universe forgot to pay its lighting bill' energy."),
			}},
			{ TEXT("rain"), {
				TEXT("Rain again. {animal} predicted this. I should've listened."),
				TEXT("It's not raining ON me, it's raining AT me. There's a difference."),
				TEXT("Great. Now I'm just {noun}, but damp."),
			}},
		};
		return W;
	}

	// The Self-Appointed Weather Anchor's unhinged-forecaster lines.
	const TMap<FString, TArray<FString>>& DialogueWeatherAnchor()
	{
		static const TMap<FString, TArray<FString>> WA = {
			{ TEXT("clear"), { TEXT("FORECAST: clear skies, high confidence, low followers.") }},
			{ TEXT("cloudy"), { TEXT("Partly cloudy with a 40% chance of me being right for once.") }},
			{ TEXT("overcast"), { TEXT("Doppler confirms: the sky is doing a Mood today, folks.") }},
			{ TEXT("rain"), { TEXT("LIVE: precipitation event in progress, I am IN it, send a towel.") }},
		};
		return WA;
	}

	// Map an NpcMind activity to a dialogue context (several activities share one).
	const TMap<FString, FString>& DialogueActivityContext()
	{
		static const TMap<FString, FString> AC = {
			{ TEXT("work"), TEXT("work") },
			{ TEXT("eat"), TEXT("eat") },
			{ TEXT("sleep"), TEXT("sleep") },
			{ TEXT("socialize"), TEXT("socialize") },
			{ TEXT("goof_off"), TEXT("goof_off") },
			{ TEXT("commute"), TEXT("commute") },
			{ TEXT("loiter"), TEXT("idle") },
			{ TEXT("freshen_up"), TEXT("idle") },
		};
		return AC;
	}
}

const TArray<FString>& FNpcDialogue::Nouns()
{
	static const TArray<FString> N = {
		TEXT("a sentient parking meter"),
		TEXT("the concept of Tuesday"),
		TEXT("my emotional support stapler"),
		TEXT("a pigeon with opinions"),
		TEXT("the void, but cozy"),
		TEXT("an unpaid invoice"),
		TEXT("a haunted vending machine"),
		TEXT("my third favourite cloud"),
	};
	return N;
}

const TArray<FString>& FNpcDialogue::Places()
{
	static const TArray<FString> P = {
		TEXT("the DMV of the soul"),
		TEXT("a parking garage at 3am"),
		TEXT("the good Wendy's"),
		TEXT("a Build-A-Bear in crisis"),
		TEXT("the suburbs of my mind"),
		TEXT("aisle seven"),
	};
	return P;
}

const TArray<FString>& FNpcDialogue::Animals()
{
	static const TArray<FString> A = {
		TEXT("a raccoon"),
		TEXT("seven confident geese"),
		TEXT("a tactical squirrel"),
		TEXT("a possum named Greg"),
		TEXT("a committee of crows"),
	};
	return A;
}

const TArray<FString>& FNpcDialogue::Witness()
{
	static const TArray<FString> W = {
		TEXT("Wait. WAIT. You're that one. From the {place} thing. I KNEW it."),
		TEXT("I saw what you did. I'm not saying anything. But I SAW. ({animal} also saw.)"),
		TEXT("Oh no. Oh no no. It's you. I'm just gonna— yep. Walking now."),
		TEXT("You've got a real 'recently committed crimes' aura. Respect. From over here."),
		TEXT("I'm a witness. A scared, well-dressed witness. Please don't subpoena me."),
		TEXT("Aren't you on the news? You're DEFINITELY on the news. Smile!"),
	};
	return W;
}

FString FNpcDialogue::Fill(const FString& Line, int32 SeedValue)
{
	FString Out = Line;
	if (Out.Contains(TEXT("{noun}")))
	{
		Out = Out.Replace(TEXT("{noun}"), *Nouns()[DialoguePosmod(SeedValue, Nouns().Num())]);
	}
	if (Out.Contains(TEXT("{place}")))
	{
		Out = Out.Replace(TEXT("{place}"), *Places()[DialoguePosmod(static_cast<int64>(SeedValue) * 7 + 1, Places().Num())]);
	}
	if (Out.Contains(TEXT("{animal}")))
	{
		Out = Out.Replace(TEXT("{animal}"), *Animals()[DialoguePosmod(static_cast<int64>(SeedValue) * 13 + 2, Animals().Num())]);
	}
	return Out;
}

FString FNpcDialogue::WitnessBark(int32 SeedValue)
{
	const TArray<FString>& W = Witness();
	return Fill(W[DialoguePosmod(SeedValue, W.Num())], SeedValue);
}

FString FNpcDialogue::WeatherBark(const FString& Voice, const FString& Condition, int32 SeedValue)
{
	const TMap<FString, TArray<FString>>& Bank = (Voice == TEXT("weather")) ? DialogueWeatherAnchor() : DialogueWeather();

	// bank.get(condition, WEATHER.get(condition, ["Weather, huh."]))
	const TArray<FString>* Lines = Bank.Find(Condition);
	TArray<FString> Fallback;
	if (!Lines)
	{
		const TArray<FString>* WeatherLines = DialogueWeather().Find(Condition);
		if (WeatherLines)
		{
			Lines = WeatherLines;
		}
		else
		{
			Fallback = { TEXT("Weather, huh.") };
			Lines = &Fallback;
		}
	}

	if (Lines->Num() == 0)
	{
		return TEXT("Weather, huh.");
	}
	return Fill((*Lines)[DialoguePosmod(SeedValue, Lines->Num())], SeedValue);
}

TArray<FString> FNpcDialogue::LinesFor(const FString& Voice, const FString& Context)
{
	const TMap<FString, TArray<FString>>* VoiceBank = DialogueBanks().Find(Voice);
	if (VoiceBank)
	{
		const TArray<FString>* CtxLines = VoiceBank->Find(Context);
		if (CtxLines && CtxLines->Num() > 0)
		{
			return *CtxLines;
		}
	}
	const TArray<FString>* GenericLines = DialogueGeneric().Find(Context);
	if (GenericLines)
	{
		return *GenericLines;
	}
	return TArray<FString>();
}

FString FNpcDialogue::Bark(const FString& Voice, const FString& Context, int32 SeedValue)
{
	const TArray<FString> Lines = LinesFor(Voice, Context);
	if (Lines.Num() == 0)
	{
		return TEXT("...");
	}
	const FString& Line = Lines[DialoguePosmod(SeedValue, Lines.Num())];
	return Fill(Line, SeedValue);
}

FString FNpcDialogue::BarkForActivity(const FString& Voice, const FString& Activity, int32 SeedValue)
{
	const FString* Mapped = DialogueActivityContext().Find(Activity);
	const FString Context = Mapped ? *Mapped : FString(TEXT("idle"));
	return Bark(Voice, Context, SeedValue);
}
