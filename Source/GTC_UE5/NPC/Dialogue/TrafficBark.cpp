// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficBark.h"

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

	const TArray<FString>& GenericBank()
	{
		static const TArray<FString> Bank = {
			TEXT("Hey! Watch it!"),
			TEXT("WHOA—!"),
			TEXT("I'm walking here!"),
			TEXT("Easy! EASY!"),
			TEXT("You trying to kill me?!"),
		};
		return Bank;
	}

	// Voice-flavoured startle banks (FContactBark pattern). Absent voice -> generic.
	const TArray<FString>* VoiceBank(const FString& Voice)
	{
		static const TArray<FString> Doomsday = {
			TEXT("The machines! It's BEGUN!"),
			TEXT("Death on four wheels — I KNEW it!"),
			TEXT("This is how it starts!") };
		static const TArray<FString> Influencer = {
			TEXT("You almost hit my CONTENT!"),
			TEXT("That's going on the story!"),
			TEXT("Did anyone get that?!") };
		static const TArray<FString> Conspiracy = {
			TEXT("They sent that one for ME!"),
			TEXT("Government plates. Has to be."),
			TEXT("See?! SEE?!") };
		static const TArray<FString> Yogi = {
			TEXT("Breathe. We... we breathe through this."),
			TEXT("The universe TESTS me."),
			TEXT("Centered. I am centered. Barely.") };

		if (Voice == TEXT("doomsday"))   return &Doomsday;
		if (Voice == TEXT("influencer")) return &Influencer;
		if (Voice == TEXT("conspiracy")) return &Conspiracy;
		if (Voice == TEXT("yogi"))       return &Yogi;
		return nullptr;
	}
}

int32 FTrafficBark::Count(const FString& Voice)
{
	if (Voice == TEXT("mime"))
	{
		return 0; // the mime mimes their terror, silently
	}
	const TArray<FString>* Bank = VoiceBank(Voice);
	return Bank ? Bank->Num() : GenericBank().Num();
}

FString FTrafficBark::Line(const FString& Voice, int32 Index)
{
	if (Voice == TEXT("mime"))
	{
		return FString();
	}
	const TArray<FString>* Bank = VoiceBank(Voice);
	const TArray<FString>& Use = Bank ? *Bank : GenericBank();
	if (Use.Num() == 0)
	{
		return FString();
	}
	return Use[PosMod(Index, Use.Num())];
}
