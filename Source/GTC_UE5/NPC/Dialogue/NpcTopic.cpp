// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcTopic.h"
#include "../NpcSeqMath.h"

namespace
{
	using GtcSeq::PosMod;

	// The topics Shared() rotates between (SmallTalk is fallback-only, excluded).
	const ENpcChatTopic kRealTopics[] = {
		ENpcChatTopic::Weather, ENpcChatTopic::City, ENpcChatTopic::Food,
		ENpcChatTopic::Gossip, ENpcChatTopic::Pigeons, ENpcChatTopic::Work };

	const TArray<FString>& Bank(ENpcChatTopic Topic, bool bReply)
	{
		// Openers raise the topic; replies answer it. Voice-agnostic, characterful,
		// GTC-original. Kept small + lively; free to grow.
		static const TArray<FString> WeatherO = {
			TEXT("Some heat today, huh?"), TEXT("Think it's gonna rain?"),
			TEXT("This humidity is unreal.") };
		static const TArray<FString> WeatherR = {
			TEXT("Tell me about it."), TEXT("Saw clouds rolling in earlier."),
			TEXT("My hair gave up hours ago.") };

		static const TArray<FString> CityO = {
			TEXT("This town never slows down."), TEXT("Rent's gotten insane, right?"),
			TEXT("Whole block's changing.") };
		static const TArray<FString> CityR = {
			TEXT("You said it."), TEXT("I miss the old spot."),
			TEXT("Can't park anywhere anymore.") };

		static const TArray<FString> FoodO = {
			TEXT("Found this new taco place."), TEXT("You eat yet?"),
			TEXT("That diner's still the best.") };
		static const TArray<FString> FoodR = {
			TEXT("Ooh, where?"), TEXT("Starving, honestly."),
			TEXT("Their fries, though.") };

		static const TArray<FString> GossipO = {
			TEXT("You hear about Marco?"), TEXT("Did you SEE that earlier?"),
			TEXT("Don't tell anyone, but...") };
		static const TArray<FString> GossipR = {
			TEXT("No way. Spill."), TEXT("I KNEW it."),
			TEXT("My lips are sealed. ...Go on.") };

		static const TArray<FString> PigeonsO = {
			TEXT("Those pigeons are watching us."), TEXT("One of 'em winked at me."),
			TEXT("They're organizing, I'm telling you.") };
		static const TArray<FString> PigeonsR = {
			TEXT("...you're not wrong."), TEXT("Okay, that one IS staring."),
			TEXT("I don't trust 'em either.") };

		static const TArray<FString> WorkO = {
			TEXT("Long shift today."), TEXT("Boss is on one again."),
			TEXT("Almost the weekend.") };
		static const TArray<FString> WorkR = {
			TEXT("Same here."), TEXT("Hang in there."),
			TEXT("Counting the minutes.") };

		static const TArray<FString> SmallO = {
			TEXT("Hey, how's it going?"), TEXT("Crazy day, right?"),
			TEXT("Long time no see.") };
		static const TArray<FString> SmallR = {
			TEXT("Can't complain."), TEXT("You know how it is."),
			TEXT("Good to see ya.") };

		switch (Topic)
		{
		case ENpcChatTopic::Weather: return bReply ? WeatherR : WeatherO;
		case ENpcChatTopic::City:    return bReply ? CityR : CityO;
		case ENpcChatTopic::Food:    return bReply ? FoodR : FoodO;
		case ENpcChatTopic::Gossip:  return bReply ? GossipR : GossipO;
		case ENpcChatTopic::Pigeons: return bReply ? PigeonsR : PigeonsO;
		case ENpcChatTopic::Work:    return bReply ? WorkR : WorkO;
		case ENpcChatTopic::SmallTalk:
		default:                     return bReply ? SmallR : SmallO;
		}
	}
}

ENpcChatTopic FNpcTopic::Shared(int32 SeedA, int32 SeedB)
{
	// Order-independent combine so both sides agree: fold the pair's sum through a
	// finalizer, then index the real-topic list. (a+b) is symmetric; the mix breaks
	// up the obvious clustering of nearby sums.
	uint32 H = static_cast<uint32>(SeedA) + static_cast<uint32>(SeedB);
	H ^= H >> 15;
	H *= 2246822519u;
	H ^= H >> 13;
	const int32 N = UE_ARRAY_COUNT(kRealTopics);
	return kRealTopics[PosMod(static_cast<int32>(H & 0x7FFFFFFF), N)];
}

int32 FNpcTopic::Count(ENpcChatTopic Topic, bool bReply)
{
	return Bank(Topic, bReply).Num();
}

FString FNpcTopic::Line(ENpcChatTopic Topic, bool bReply, int32 Index)
{
	const TArray<FString>& B = Bank(Topic, bReply);
	if (B.Num() == 0)
	{
		return FString();
	}
	return B[PosMod(Index, B.Num())];
}

const TCHAR* FNpcTopic::Name(ENpcChatTopic Topic)
{
	switch (Topic)
	{
	case ENpcChatTopic::Weather: return TEXT("weather");
	case ENpcChatTopic::City:    return TEXT("city");
	case ENpcChatTopic::Food:    return TEXT("food");
	case ENpcChatTopic::Gossip:  return TEXT("gossip");
	case ENpcChatTopic::Pigeons: return TEXT("pigeons");
	case ENpcChatTopic::Work:    return TEXT("work");
	case ENpcChatTopic::SmallTalk:
	default:                     return TEXT("small_talk");
	}
}
