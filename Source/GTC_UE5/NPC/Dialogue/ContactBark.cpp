// Copyright Epic Games, Inc. All Rights Reserved.

#include "ContactBark.h"

namespace
{
    // Reaction -> lines, ordered (index selection is observable). Escalates from a
    // polite brush to a floored cry for help.
    const TMap<ENpcContactReaction, TArray<FString>>& ContactLines()
    {
        static const TMap<ENpcContactReaction, TArray<FString>> L = {
            { ENpcContactReaction::Excuse, {
                TEXT("Oh — excuse me."),
                TEXT("Sorry, my fault."),
                TEXT("Pardon me."),
                TEXT("Whoops — sorry!"),
                TEXT("My bad."),
            }},
            { ENpcContactReaction::Annoyed, {
                TEXT("Hey, watch where you're going!"),
                TEXT("Watch it, pal!"),
                TEXT("Do you mind?"),
                TEXT("Hey! Eyes up!"),
                TEXT("Walk much?"),
                TEXT("Seriously?"),
            }},
            { ENpcContactReaction::Flinch, {
                TEXT("Whoa!"),
                TEXT("Hey — easy!"),
                TEXT("Back off!"),
                TEXT("What the—?!"),
                TEXT("Careful!"),
            }},
            { ENpcContactReaction::Confront, {
                TEXT("You wanna go?!"),
                TEXT("What's your problem?!"),
                TEXT("Try that again!"),
                TEXT("You think that's funny?"),
                TEXT("Step back. Now."),
                TEXT("I'm not scared of you!"),
            }},
            { ENpcContactReaction::Flee, {
                TEXT("Get away from me!"),
                TEXT("Help! Somebody!"),
                TEXT("Leave me alone!"),
                TEXT("Stay back!"),
                TEXT("Please — no!"),
            }},
            { ENpcContactReaction::Knockdown, {
                TEXT("Aaagh!"),
                TEXT("He hit me!"),
                TEXT("Somebody help!"),
                TEXT("Call the police!"),
                TEXT("Why?!"),
            }},
        };
        return L;
    }

    // Ambient lines for the player brushing past close without contact.
    const TArray<FString>& PassingLines()
    {
        static const TArray<FString> P = {
            TEXT("Hey."),
            TEXT("How's it going?"),
            TEXT("'Scuse me."),
            TEXT("Watch yourself."),
            TEXT("Mornin'."),
            TEXT("..."),
            TEXT("Nice day, huh?"),
        };
        return P;
    }

    // Godot posmod(index, n): always non-negative in [0, n).
    int32 ContactWrap(int32 Index, int32 N)
    {
        return ((Index % N) + N) % N;
    }
}

namespace
{
    // voice -> reaction -> lines: personality where it pops (a shove, a hit, a fall).
    // Reactions a voice doesn't author fall back to the generic bank above, so this
    // only needs the lines that are worth saying in-character. A mime "speaks" in
    // stage directions — its speech bubble describes the silent outrage.
    const TMap<FString, TMap<ENpcContactReaction, TArray<FString>>>& VoiceContactLines()
    {
        static const TMap<FString, TMap<ENpcContactReaction, TArray<FString>>> V = {
            { TEXT("doomsday"), {
                { ENpcContactReaction::Annoyed, {
                    TEXT("The grounds warned me about you."),
                    TEXT("This was foretold. Watch it."),
                }},
                { ENpcContactReaction::Confront, {
                    TEXT("You'll regret this when the tide comes."),
                    TEXT("I've SEEN how this ends for you."),
                }},
                { ENpcContactReaction::Flee, {
                    TEXT("It's starting! It's all starting!"),
                    TEXT("The signs were RIGHT!"),
                }},
                { ENpcContactReaction::Knockdown, {
                    TEXT("So this is how it ends..."),
                    TEXT("Tell the baristas... I knew."),
                }},
            }},
            { TEXT("influencer"), {
                { ENpcContactReaction::Annoyed, {
                    TEXT("You ALMOST made me drop my phone!"),
                    TEXT("Do you know how many followers I have?"),
                }},
                { ENpcContactReaction::Confront, {
                    TEXT("I'm literally filming this. Smile."),
                    TEXT("This is SO going online."),
                }},
                { ENpcContactReaction::Flee, {
                    TEXT("Not the face! NOT the face!"),
                    TEXT("Someone clip this!"),
                }},
                { ENpcContactReaction::Knockdown, {
                    TEXT("I'm streaming this assault, by the way."),
                    TEXT("My followers will FIND you."),
                }},
            }},
            { TEXT("conspiracy"), {
                { ENpcContactReaction::Annoyed, {
                    TEXT("Was that... deliberate? Who sent you?"),
                    TEXT("The pigeons saw that."),
                }},
                { ENpcContactReaction::Confront, {
                    TEXT("You're one of THEM, aren't you?"),
                    TEXT("I knew they'd send someone."),
                }},
                { ENpcContactReaction::Flee, {
                    TEXT("They've activated him! Run!"),
                    TEXT("I KNEW the relish was a trap!"),
                }},
                { ENpcContactReaction::Knockdown, {
                    TEXT("Tell... the truth... about the buns..."),
                    TEXT("Wake up, sheeple!"),
                }},
            }},
            { TEXT("mime"), {
                { ENpcContactReaction::Excuse,    { TEXT("(an apologetic mime bow)") }},
                { ENpcContactReaction::Annoyed,   { TEXT("(furious silent gesturing)"), TEXT("(mimes an invisible wall between you)") }},
                { ENpcContactReaction::Flinch,    { TEXT("(exaggerated silent gasp)") }},
                { ENpcContactReaction::Confront,  { TEXT("(mimes rolling up sleeves)"), TEXT("(silent, menacing jazz hands)") }},
                { ENpcContactReaction::Flee,      { TEXT("(frantic invisible-rope climbing away)") }},
                { ENpcContactReaction::Knockdown, { TEXT("(the most dramatic silent collapse)") }},
            }},
            { TEXT("food_critic"), {
                { ENpcContactReaction::Annoyed,  { TEXT("Rude. Two stars."), TEXT("The audacity. Unsalted.") }},
                { ENpcContactReaction::Confront, { TEXT("I will END your Yelp presence."), TEXT("You call that a shove? Amateur.") }},
            }},
            { TEXT("intern"), {
                { ENpcContactReaction::Annoyed, { TEXT("I do NOT have time for this!"), TEXT("I'm already so behind!") }},
                { ENpcContactReaction::Flinch,  { TEXT("Is this in my job description?!") }},
                { ENpcContactReaction::Flee,    { TEXT("This is above my pay grade!"), TEXT("I'm just an intern!") }},
            }},
            { TEXT("method_actor"), {
                { ENpcContactReaction::Annoyed,  { TEXT("You've broken my concentration."), TEXT("I was MID-SCENE.") }},
                { ENpcContactReaction::Confront, { TEXT("Now THIS is my motivation."), TEXT("We do this in ONE take.") }},
                { ENpcContactReaction::Flee,     { TEXT("Exit, stage left!"), TEXT("This wasn't in the script!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("And... scene."), TEXT("What's my motivation NOW?!") }},
            }},
            { TEXT("yogi"), {
                { ENpcContactReaction::Annoyed,  { TEXT("I'm breathing through this. Barely."), TEXT("Namaste. That means back off.") }},
                { ENpcContactReaction::Confront, { TEXT("Let's find your center. Together."), TEXT("My chakras are ALIGNED for this.") }},
                { ENpcContactReaction::Flee,     { TEXT("Removing myself from this energy!"), TEXT("Toxic! Toxic vibes!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("I'll just... ground myself. Literally."), TEXT("Inhale... ow... exhale.") }},
            }},
            { TEXT("stunt_double"), {
                { ENpcContactReaction::Annoyed,  { TEXT("Kid, I've taken worse before lunch."), TEXT("That all you got?") }},
                { ENpcContactReaction::Confront, { TEXT("I do my own stunts. Come on."), TEXT("I've fallen off buildings, pal.") }},
                { ENpcContactReaction::Flee,     { TEXT("Tactical retreat! Done it a hundred times."), TEXT("Rolling out!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("Ten out of ten landing."), TEXT("Stick the landing... ow.") }},
            }},
            { TEXT("philosopher"), {
                { ENpcContactReaction::Annoyed,  { TEXT("Was that contact, or the illusion of it?"), TEXT("We all just bump into each other, cosmically.") }},
                { ENpcContactReaction::Confront, { TEXT("Define 'fight'. Then we'll talk."), TEXT("Violence is a failure of imagination. Yet here we are.") }},
                { ENpcContactReaction::Flee,     { TEXT("I think, therefore I RUN!"), TEXT("This is not the examined life I wanted!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("So this is what falling teaches us..."), TEXT("Gravity: undefeated.") }},
            }},
            { TEXT("life_coach"), {
                { ENpcContactReaction::Annoyed,  { TEXT("Hey! That's not very growth-mindset."), TEXT("Let's circle back on your behavior.") }},
                { ENpcContactReaction::Confront, { TEXT("I'm gonna need you to level UP."), TEXT("This is a teachable moment. For YOU.") }},
                { ENpcContactReaction::Flee,     { TEXT("Pivoting! Big pivot!"), TEXT("Reframing this as a cardio opportunity!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("Failure is just feedback... ow."), TEXT("I'm rebranding this as resting.") }},
            }},
            { TEXT("weather"), {
                { ENpcContactReaction::Annoyed,  { TEXT("Forecast: rude, chance of lawsuit."), TEXT("And in local news: YOU.") }},
                { ENpcContactReaction::Confront, { TEXT("High pressure system moving in. Me."), TEXT("Storm's coming, friend.") }},
                { ENpcContactReaction::Flee,     { TEXT("Doppler says RUN!"), TEXT("Taking cover! Live shot!") }},
                { ENpcContactReaction::Knockdown,{ TEXT("And we're down. Back to you in studio."), TEXT("Turbulence! Heavy turbulence!") }},
            }},
        };
        return V;
    }

    // Pick the bank for (voice, reaction): the voice's own if it authored one, else
    // the generic bank for that reaction.
    const TArray<FString>* ResolveBank(const FString& Voice, ENpcContactReaction Reaction)
    {
        if (const TMap<ENpcContactReaction, TArray<FString>>* ByReaction = VoiceContactLines().Find(Voice))
        {
            if (const TArray<FString>* Voiced = ByReaction->Find(Reaction))
            {
                if (Voiced->Num() > 0)
                {
                    return Voiced;
                }
            }
        }
        return ContactLines().Find(Reaction);
    }
}

FString FContactBark::Line(const FString& Voice, ENpcContactReaction Reaction, int32 Index)
{
    const TArray<FString>* Pool = ResolveBank(Voice, Reaction);
    if (!Pool || Pool->Num() == 0)
    {
        return FString(); // Ignore / unknown — nothing to say
    }
    return (*Pool)[ContactWrap(Index, Pool->Num())];
}

int32 FContactBark::Count(const FString& Voice, ENpcContactReaction Reaction)
{
    const TArray<FString>* Pool = ResolveBank(Voice, Reaction);
    return Pool ? Pool->Num() : 0;
}

FString FContactBark::PassingLine(int32 Index)
{
    const TArray<FString>& P = PassingLines();
    return P[ContactWrap(Index, P.Num())];
}

int32 FContactBark::PassingCount()
{
    return PassingLines().Num();
}
