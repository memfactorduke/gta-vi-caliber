// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorkBark.h"
#include "../NpcSeqMath.h"

namespace
{
	// Action token -> spoken lines for that beat of the job. Only the "talky"
	// actions appear; physical busywork (wipe_counter, frame_shot, scoop_after_dog,
	// stretch, the mime's whole silent repertoire...) is intentionally absent so it
	// reads as silent work. GTC-original, voice-agnostic, free to grow.
	const TMap<FName, TArray<FString>>& Banks()
	{
		static const TMap<FName, TArray<FString>> B = {
			// --- barista (Doomsday Barista) ---
			{ TEXT("call_order"), {
				TEXT("Oat-milk doom latte, for... whoever's left!"),
				TEXT("Order up. Drink it while there's still time."),
				TEXT("Large cold brew? Bold, on a day like this."),
			}},
			{ TEXT("foretell_doom"), {
				TEXT("The foam never lies, friend."),
				TEXT("Two sugars. The end is near. Anything else?"),
				TEXT("I read it in the crema. It's... not great."),
			}},
			// --- influencer (Pigeon Influencer) ---
			{ TEXT("narrate_to_camera"), {
				TEXT("Guys. GUYS. Look at this pigeon."),
				TEXT("Smash that follow for more bird content."),
				TEXT("We are SO back, chat."),
			}},
			{ TEXT("check_views"), {
				TEXT("Four views. Four loyal souls."),
				TEXT("It's trending. In my heart."),
			}},
			// --- crossing_guard (Method Crossing Guard) ---
			{ TEXT("blow_whistle"), {
				TEXT("HOLD! ...for your safety, and my craft."),
				TEXT("Cross now. Feel the moment."),
				TEXT("You. Yes, you. Wait for my cue."),
			}},
			{ TEXT("study_motivation"), {
				TEXT("What does this crosswalk... need from me?"),
				TEXT("Six years I've trained for this corner."),
			}},
			// --- vendor (Conspiracy Hot-Dog Vendor) ---
			{ TEXT("hawk_wares"), {
				TEXT("Dogs! Get your dogs! Cash only, they track cards."),
				TEXT("One dollar! The pigeons can't stop me!"),
				TEXT("Hot dog? Don't make eye contact with the birds."),
			}},
			{ TEXT("scan_for_drones"), {
				TEXT("...that pigeon's been here three days."),
				TEXT("Smile. They're always watching."),
			}},
			// --- yogi (Aggressively Calm Yoga Instructor) ---
			{ TEXT("enforce_serenity"), {
				TEXT("You WILL find your center. Now."),
				TEXT("Breathe. I said breathe."),
				TEXT("Relax. That's an order from the universe."),
			}},
			{ TEXT("correct_form"), {
				TEXT("Hips square. Soul squarer."),
				TEXT("No, no — feel it in your gratitude."),
			}},
			// --- stunt_double (Retired Stunt Double) ---
			{ TEXT("recall_glory_days"), {
				TEXT("I fell off a building for that scene. Twice."),
				TEXT("They don't roll like they used to."),
			}},
			// --- intern (Over-Caffeinated Intern) ---
			{ TEXT("sprint_an_errand"), {
				TEXT("On it on it on it!"),
				TEXT("Coffee's a food group, right? Right."),
			}},
			{ TEXT("vibrate_slightly"), {
				TEXT("I can hear colors."),
				TEXT("I'm fine. I'm SO fine."),
			}},
			// --- dog_walker (Philosophical Dog-Walker) ---
			{ TEXT("praise_good_boy"), {
				TEXT("Who's a good boy? Epistemologically?"),
				TEXT("Good dog. Better than most people, honestly."),
			}},
			{ TEXT("ponder_existence"), {
				TEXT("The leash, too, is a kind of freedom."),
				TEXT("Seven dogs. One unexamined life."),
			}},
			// --- food_critic (Feral Food Critic) ---
			{ TEXT("demand_the_menu"), {
				TEXT("Bring me everything. I will judge it."),
				TEXT("Is this... artisanal? It had better be."),
			}},
			{ TEXT("recoil_in_disgust"), {
				TEXT("One star. The sidewalk has more depth."),
				TEXT("I've TASTED better disappointment."),
			}},
			// --- life_coach (Unlicensed Life Coach) ---
			{ TEXT("affirm_loudly"), {
				TEXT("You are ENOUGH. That'll be forty dollars."),
				TEXT("I believe in you. Aggressively."),
				TEXT("Your only competition is yesterday-you. Crush him."),
			}},
			{ TEXT("hand_business_card"), {
				TEXT("Card's a little wet. So is destiny."),
				TEXT("Call me when you're ready to WIN."),
			}},
			// --- weather_anchor (Self-Appointed Weather Anchor) ---
			{ TEXT("deliver_forecast"), {
				TEXT("Live, from this bus bench: it's humid."),
				TEXT("Eighty percent chance of me being right."),
				TEXT("Back to you, also me."),
			}},
		};
		return B;
	}

	using GtcSeq::PosMod;
}

int32 FWorkBark::Count(FName Action)
{
	const TArray<FString>* Bank = Banks().Find(Action);
	return Bank ? Bank->Num() : 0;
}

FString FWorkBark::Line(FName Action, int32 Index)
{
	const TArray<FString>* Bank = Banks().Find(Action);
	if (!Bank || Bank->Num() == 0)
	{
		return FString();
	}
	return (*Bank)[PosMod(Index, Bank->Num())];
}
