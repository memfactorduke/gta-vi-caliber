// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * What a pedestrian blurts when a car nearly takes them out. The curb-safety math
 * (FPedestrianTraffic) already makes citizens halt and dodge, but they did it in
 * dead silence — a near-miss should earn a yelp. This is the voice of that
 * startle: the universal "Hey! Watch it!", plus voice-flavoured variants so a
 * doomsday prepper reads it as the machine uprising and an influencer mourns the
 * content they almost made.
 *
 * Keyed by archetype voice with a generic fallback (the FContactBark pattern), so
 * any citizen has something to shout. The mime stays silent (returns "").
 *
 * PURE-CORE: scene-free, deterministic FString selection by voice + index
 * (Godot-style posmod wrap), no engine coupling. Unit-tested headless
 * (Tests/TrafficBarkTest.cpp, prefix GTC.NPC.Dialogue.TrafficBark). GTC-original.
 */
struct GTC_UE5_API FTrafficBark
{
	/**
	 * A startled-at-traffic line in `Voice`, posmod-wrapped by Index. A voice with
	 * no bank falls back to the generic bank; the mime returns "". Never empty
	 * except for the mime.
	 */
	static FString Line(const FString& Voice, int32 Index);

	/** Number of lines for a voice (its own bank, else generic; 0 for the mime). */
	static int32 Count(const FString& Voice);
};
