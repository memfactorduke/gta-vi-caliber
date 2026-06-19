// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVoiceComponent.h"
#include "VoiceSynth.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

// Global kill-switch for the placeholder NPC voice synth. Defaults to muted: the
// procedural blip voices read as a droning hum across a crowd, so they stay off
// until a real phoneme bank / TTS replaces them. Flip live from the console with
// `gtc.NpcVoice.Mute 0` to let the city speak again.
static TAutoConsoleVariable<int32> CVarNpcVoiceMute(
	TEXT("gtc.NpcVoice.Mute"),
	1,
	TEXT("Silence all NPC voice playback. 1 = muted (default), 0 = audible."),
	ECVF_Default);

UGTCVoiceComponent::UGTCVoiceComponent()
{
	// No tick: this component only acts when the owner speaks.
	PrimaryComponentTick.bCanEverTick = false;
	SampleRate = FVoiceSynth::DefaultSampleRate();
}

void UGTCVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	// Defer audio creation until the first line so a never-speaking crowd member
	// (off-screen, mute) costs nothing.
}

void UGTCVoiceComponent::Configure(const FNpcVoiceProfile& InProfile)
{
	Profile = InProfile;
}

void UGTCVoiceComponent::EnsureAudioReady()
{
	if (ProcWave && AudioComp)
	{
		return;
	}

	if (!ProcWave)
	{
		// A procedural wave already declares itself streaming (its ctor sets the
		// indefinite duration); we only supply format + group. It plays whatever we
		// queue and falls idle when the queue drains.
		ProcWave = NewObject<USoundWaveProcedural>(this);
		ProcWave->SetSampleRate(SampleRate);
		ProcWave->NumChannels = 1;
		ProcWave->SoundGroup = SOUNDGROUP_Voice;
	}

	if (!AudioComp)
	{
		AActor* Owner = GetOwner();
		AudioComp = NewObject<UAudioComponent>(Owner ? Owner : (UObject*)this);
		AudioComp->bAutoActivate = false;
		AudioComp->bAllowSpatialization = true;
		AudioComp->bIsUISound = false;
		if (Owner && Owner->GetRootComponent())
		{
			AudioComp->SetupAttachment(Owner->GetRootComponent());
		}
		AudioComp->RegisterComponent();
		AudioComp->SetSound(ProcWave);
	}
}

void UGTCVoiceComponent::PlayLine(const FString& Line, int32 LineSeed)
{
	if (CVarNpcVoiceMute.GetValueOnGameThread() != 0)
	{
		return; // city-wide voice mute (gtc.NpcVoice.Mute) — placeholder synth is off
	}

	if (Profile.bMute)
	{
		return; // the mime stays silent; lip/gesture timing is handled by the citizen
	}

	const TArray<int16> Pcm = FVoiceSynth::RenderPcm(Profile, Line, LineSeed, SampleRate);
	if (Pcm.Num() == 0)
	{
		return; // wordless line — nothing to say
	}

	EnsureAudioReady();
	if (!ProcWave || !AudioComp)
	{
		return;
	}

	// Queue the freshly-synthesised utterance and make sure the stream is playing.
	ProcWave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	if (!AudioComp->IsPlaying())
	{
		AudioComp->Play();
	}
}
