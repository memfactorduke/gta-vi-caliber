// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Insertion-ordered JSON value model for the save system.
 *
 * This is a thin ORDERED WRAPPER over the engine `Json` module. The serialize/parse layer
 * (see SaveJson.cpp) is implemented on UE's `TJsonWriter` / `TJsonReader` (the `Json` module,
 * a Private dependency added to GTC_UE5.Build.cs). The wrapper exists for one reason the
 * engine type cannot satisfy on its own: engine `FJsonObject` stores fields in an UNORDERED
 * `TMap`, but an insertion-ordered map key order is observable across the save round-trip, so this
 * model keeps an explicit ordered key list alongside the map. Serialization walks that list
 * (ordered `TJsonWriter` writes); parsing rebuilds it from the reader's in-document-order
 * token stream — so serialize->parse preserves key order exactly.
 *
 * Semantics mirrored from the reference implementation:
 *  - Objects are INSERTION-ORDERED (the reference Dictionary order is observable on save text).
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

/** Insertion-ordered string->value map (mirrors the reference Dictionary order). */
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

    /** Deep clone (the reference duplicate(true)) — keeps Migrate pure. */
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
