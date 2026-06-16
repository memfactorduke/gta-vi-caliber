// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Self-contained JSON value model for the save system.
 *
 * WHY NOT the engine `Json` module: the approved Wave 2 design permits "UE FJsonObject /
 * FJsonSerializer OR EQUIVALENT", and the Wave conventions forbid any *.Build.cs edit / new
 * module. In this installed UE 5.7, the `Json` module's out-of-line symbols are NOT on the
 * GTC_UE5 module's link line (Json is only a *transitive* public dep of Engine, headers
 * resolve but the archive does not link), so using FJsonObject would require adding "Json"
 * to PublicDependencyModuleNames — a forbidden Build.cs edit. This minimal, header-light
 * model is the sanctioned "equivalent": it lives entirely in-module, needs no new module
 * dependency, and mirrors Godot's Dictionary/Array/JSON semantics exactly.
 *
 * Semantics mirrored from Godot:
 *  - Objects are INSERTION-ORDERED (Godot Dictionary order is observable on save text).
 *  - Numbers are `double` (int and float are one numeric type, matching `is float or is int`).
 *  - Parse is silent on malformed input (returns an invalid/null value, never logs).
 *
 * --- ROUND-TRIP TOLERANCE CONTRACT ---
 * Round-trip parity is by *value within tolerance / structural equality*, NOT byte-identity.
 * Float text may differ in least-significant digits across the Godot<->UE boundary (W1
 * FRandomStream / double literals are deterministic-per-seed but not byte-identical to
 * Godot). Compare floats with GtcTest::Eps; never assert byte/string equality of serialized
 * text. Structure (keys present, order, array sizes, version header), integers, strings and
 * bools round-trip exactly.
 */
enum class EGtcJsonType : uint8
{
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
};

class FGtcJsonValue;
using FGtcJsonValuePtr = TSharedPtr<FGtcJsonValue>;

/** Insertion-ordered string->value map (mirrors Godot Dictionary order). */
class GTC_UE5_API FGtcJsonObject
{
public:
    /** Set (or replace, keeping position) a field. */
    void Set(const FString& Key, const FGtcJsonValuePtr& Value);
    void SetNumber(const FString& Key, double Value);
    void SetString(const FString& Key, const FString& Value);
    void SetBool(const FString& Key, bool Value);
    void SetObject(const FString& Key, const TSharedRef<FGtcJsonObject>& Value);
    void SetArray(const FString& Key, const TArray<FGtcJsonValuePtr>& Value);

    /** Field by key, or null TSharedPtr if absent. */
    FGtcJsonValuePtr Get(const FString& Key) const;
    bool Has(const FString& Key) const;

    /** True only when the key holds an Object. */
    bool HasObjectField(const FString& Key) const;

    /** Convenience typed getters with fallback. */
    double GetNumber(const FString& Key, double Fallback = 0.0) const;
    FString GetString(const FString& Key, const FString& Fallback = FString()) const;
    TSharedPtr<FGtcJsonObject> GetObject(const FString& Key) const;
    TArray<FGtcJsonValuePtr> GetArray(const FString& Key) const;

    int32 Num() const { return Keys.Num(); }
    const TArray<FString>& OrderedKeys() const { return Keys; }

    /** Deep clone (Godot duplicate(true)) — keeps Migrate pure. */
    TSharedRef<FGtcJsonObject> DeepClone() const;

private:
    TArray<FString> Keys;
    TMap<FString, FGtcJsonValuePtr> Map;
};

/** A JSON value: null / bool / number / string / array / object. */
class GTC_UE5_API FGtcJsonValue
{
public:
    FGtcJsonValue() : Type(EGtcJsonType::Null) {}

    static FGtcJsonValuePtr MakeNull();
    static FGtcJsonValuePtr MakeBool(bool In);
    static FGtcJsonValuePtr MakeNumber(double In);
    static FGtcJsonValuePtr MakeString(const FString& In);
    static FGtcJsonValuePtr MakeArray(const TArray<FGtcJsonValuePtr>& In);
    static FGtcJsonValuePtr MakeObject(const TSharedRef<FGtcJsonObject>& In);

    EGtcJsonType GetType() const { return Type; }
    bool IsNumber() const { return Type == EGtcJsonType::Number; }
    bool IsObject() const { return Type == EGtcJsonType::Object; }
    bool IsArray() const { return Type == EGtcJsonType::Array; }

    bool AsBool() const { return BoolValue; }
    double AsNumber() const { return NumberValue; }
    const FString& AsString() const { return StringValue; }
    const TArray<FGtcJsonValuePtr>& AsArray() const { return ArrayValue; }
    TSharedPtr<FGtcJsonObject> AsObject() const { return ObjectValue; }

    FGtcJsonValuePtr DeepClone() const;

private:
    EGtcJsonType Type;
    bool BoolValue = false;
    double NumberValue = 0.0;
    FString StringValue;
    TArray<FGtcJsonValuePtr> ArrayValue;
    TSharedPtr<FGtcJsonObject> ObjectValue;
};

/** Serialize / parse. Parse returns nullptr on malformed input (silent, like Godot). */
namespace GtcJson
{
    GTC_UE5_API FString Serialize(const TSharedRef<FGtcJsonObject>& Object);

    /** Parse text to a value. nullptr on any malformed input. */
    GTC_UE5_API FGtcJsonValuePtr Parse(const FString& Text);
}
