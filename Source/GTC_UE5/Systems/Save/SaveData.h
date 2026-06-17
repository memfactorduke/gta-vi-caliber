// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "SaveJson.h"

/**
 * Pure (de)serialization for a save snapshot — the UE port of the reference `save_data.gd`
 * (class SaveData, RefCounted). No file or scene access: the SaveSubsystem gathers a
 * plain JSON object of game state, this wraps it with a version header and turns it
 * to/from JSON text, and the round-trip + malformed-input handling is unit-tested
 * (Tests/SaveDataTest.cpp, reference behavior game/tests/unit/test_save_data.gd, 21 funcs).
 * Decode never throws / never crashes: bad input yields an empty snapshot.
 *
 * JSON backing: a small in-module JSON model (SaveJson.h) — NOT the engine `Json` module.
 * The approved design permits "FJsonObject/FJsonSerializer OR EQUIVALENT"; using the engine
 * Json type would require adding "Json" to PublicDependencyModuleNames (a forbidden
 * Build.cs edit — Json is only a transitive public dep and does not link into this module),
 * so the in-module model is the sanctioned equivalent. Object member order is insertion
 * order, matching the reference observable Dictionary order.
 *
 * Schema versioning (matches the the reference model exactly):
 *   v2 added stats (money/armor), progression XP, property ownership and boat/bike
 *      vehicle entries on top of v1's position/health/wanted/cars.
 *   v3 added lifetime/100%-completion stats as a separate section.
 *   v4 added activity-based player skills.
 *
 * --- ROUND-TRIP TOLERANCE CONTRACT (read before relying on parity) ---
 * Round-trip parity for this model is by *value within tolerance / structural equality*,
 * NOT byte-identity. The Wave 1 ports feed values produced by FRandomStream and `double`
 * literals that are deterministic-per-seed but NOT byte-identical to the reference RNG / float
 * text, so JSON number text may differ in the least-significant digits across the
 * Godot<->UE boundary. Callers and tests MUST compare floats with GtcTest::Eps (shared
 * Tests/GtcTestTolerances.h), never via byte/string equality of the serialized text.
 * Structural shape (keys present, order, array sizes, version header) IS preserved exactly;
 * integers, strings and bools round-trip exactly; only floating-point text is tolerance-bound.
 *
 * Plain C++ value namespace (no UObject): static-style free functions, exactly like the
 * the reference static SaveData API. The owning USaveSubsystem is the UObject layer.
 */
namespace GtcSaveData
{
    /** Current save format version. */
    inline constexpr int32 Version = 4;

    /** Wrap a state snapshot with a version header and serialise to JSON text. */
    GTC_UE5_API FString Encode(const TSharedRef<FGtcJsonObject>& Snapshot);

    /**
     * Parse save text back to the inner snapshot. Returns an empty object for anything
     * that isn't a versioned object with an object payload, so callers can trust the
     * shape without exception handling. Silent on malformed input (a normal path).
     */
    GTC_UE5_API TSharedRef<FGtcJsonObject> Decode(const FString& Text);

    /** The format version embedded in save text, or 0 if absent/unparseable. */
    GTC_UE5_API int32 VersionOf(const FString& Text);

    /**
     * Bring an older snapshot up to the current shape. v1 saves predate the
     * stats/progression/properties keys — normalised to empty objects. An empty decode
     * stays empty (so SaveSubsystem's "empty == no saved data" guard still fires and a
     * corrupt/old save can't silently wipe trackers via restore({})). Unknown future
     * versions pass through untouched. Pure: returns a new object.
     */
    GTC_UE5_API TSharedRef<FGtcJsonObject> Migrate(const TSharedRef<FGtcJsonObject>& Snapshot, int32 FromVersion);

    /** FVector -> JSON-safe [x, y, z]. double precision preserved. */
    GTC_UE5_API TArray<FGtcJsonValuePtr> Vec3ToArray(const FVector& V);

    /**
     * [x, y, z] -> FVector, or Fallback when the value is malformed (not an array of
     * exactly three numbers). Takes an untrusted value so a corrupt save can never crash.
     */
    GTC_UE5_API FVector ArrayToVec3(const FGtcJsonValuePtr& Value, const FVector& Fallback);

    /** FTransform -> JSON-safe object (origin + the three rotation basis columns). */
    GTC_UE5_API TSharedRef<FGtcJsonObject> TransformToDict(const FTransform& T);

    /** Object -> FTransform, or Fallback when the value is malformed. */
    GTC_UE5_API FTransform DictToTransform(const FGtcJsonValuePtr& Value, const FTransform& Fallback);

    /** Numeric value -> double, or Fallback for anything non-numeric. */
    GTC_UE5_API double NumberOr(const FGtcJsonValuePtr& Value, double Fallback);
}
