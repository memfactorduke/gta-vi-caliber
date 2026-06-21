// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Shared deterministic integer / hash helpers for the NPC layer. `inline` => one ODR-safe
// definition across all translation units, so UE's unity-build concatenation can never redefine
// them. These were previously copy-pasted into each .cpp's anonymous namespace (PosMod x5,
// Wrap x2, Hash01 x2), which collided once a clean unity build merged those files into one blob.
namespace GtcSeq
{
	// Positive modulo: result always in [0, Modulus). Returns 0 for non-positive Modulus.
	inline int32 PosMod(int32 Value, int32 Modulus)
	{
		if (Modulus <= 0)
		{
			return 0;
		}
		return ((Value % Modulus) + Modulus) % Modulus;
	}

	// Wrap an index into [0, N) (positive modulo; caller guarantees N > 0).
	inline int32 Wrap(int32 Index, int32 N)
	{
		return ((Index % N) + N) % N;
	}

	// Deterministic FNV-1a-style hash of a seed into [0, 1).
	inline double Hash01(int32 Seed)
	{
		uint32 H = 2166136261u;
		const uint32 S = static_cast<uint32>(Seed);
		for (int32 Byte = 0; Byte < 4; ++Byte)
		{
			H ^= (S >> (Byte * 8)) & 0xFFu;
			H *= 16777619u;
		}
		H ^= H >> 15;
		H *= 2246822519u;
		H ^= H >> 13;
		return static_cast<double>(H & 0xFFFFFFu) / static_cast<double>(0x1000000u);
	}
}
