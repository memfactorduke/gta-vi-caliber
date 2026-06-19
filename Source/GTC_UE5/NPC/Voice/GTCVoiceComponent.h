// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NpcVoice.h"
#include "GTCVoiceComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * The runtime mouth: the component that makes a citizen audible. It owns a
 * spatialised audio component fed by a procedural sound wave, and turns each
 * spoken line into a short stylised vocalisation (FVoiceSynth) in the citizen's
 * own voice. AGTCCitizen creates one per person, Configure()s it with the seeded
 * voice profile at identity time, and calls PlayLine() whenever it speaks — so
 * the city murmurs in distinct voices instead of staying silent.
 *
 * The DSP lives in pure-core FVoiceSynth (headless-tested); this adapter only
 * marshals the rendered PCM into the engine's audio path. A mute profile (the
 * mime) renders nothing, so PlayLine is a harmless no-op for them.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGTCVoiceComponent();

	/** Adopt this citizen's fixed vocal signature (pitch/rate/timbre). */
	void Configure(const FNpcVoiceProfile& InProfile);

	/**
	 * Speak `Line` aloud in this voice. `LineSeed` varies the pitch contour and
	 * breath noise per utterance so repeated lines don't sound identical. Renders
	 * via FVoiceSynth and queues the PCM onto the procedural wave; silent for a
	 * mute voice or a wordless line.
	 */
	UFUNCTION(BlueprintCallable, Category = "GTC|NPC|Voice")
	void PlayLine(const FString& Line, int32 LineSeed);

	/** The profile currently driving this voice (for HUD/debug). */
	const FNpcVoiceProfile& GetProfile() const { return Profile; }

protected:
	virtual void BeginPlay() override;

private:
	/** Lazily create the procedural wave + spatial audio component on the owner. */
	void EnsureAudioReady();

	FNpcVoiceProfile Profile;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AudioComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ProcWave = nullptr;

	/** Sample rate of the procedural stream (matches FVoiceSynth). */
	int32 SampleRate = 22050;
};
