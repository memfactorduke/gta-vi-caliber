// Copyright (c) 2026 GTC contributors

#include "NpcConversation.h"
#include "NpcDialogue.h"

TArray<FNpcConversationTurn> FNpcConversation::Exchange(const FString& VoiceA, const FString& VoiceB, int32 SeedValue, int32 Turns)
{
	TArray<FNpcConversationTurn> Out;
	const int32 N = FMath::Max(Turns, 1);
	for (int32 i = 0; i < N; ++i)
	{
		const bool bEven = (i % 2 == 0);
		const FString& Voice = bEven ? VoiceA : VoiceB;

		FNpcConversationTurn Turn;
		Turn.Speaker = bEven ? 0 : 1;
		// Spread the seed per turn so consecutive lines differ.
		Turn.Text = FNpcDialogue::Bark(Voice, TEXT("chat"), SeedValue + i * 101);
		Out.Add(Turn);
	}
	return Out;
}

FString FNpcConversation::Greeting(const FString& Voice, int32 SeedValue)
{
	return FNpcDialogue::Bark(Voice, TEXT("greet"), SeedValue);
}
