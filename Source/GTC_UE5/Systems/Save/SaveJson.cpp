// Copyright (c) 2026 GTC contributors

#include "SaveJson.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

// ---- FGtcJsonObject ---------------------------------------------------------

void FGtcJsonObject::Set(const FString& Key, const FGtcJsonValuePtr& Value)
{
    if (!Map.Contains(Key))
    {
        Keys.Add(Key);
    }
    Map.Add(Key, Value);
}

void FGtcJsonObject::SetNumber(const FString& Key, double Value) { Set(Key, FGtcJsonValue::MakeNumber(Value)); }
void FGtcJsonObject::SetString(const FString& Key, const FString& Value) { Set(Key, FGtcJsonValue::MakeString(Value)); }
void FGtcJsonObject::SetBool(const FString& Key, bool Value) { Set(Key, FGtcJsonValue::MakeBool(Value)); }
void FGtcJsonObject::SetObject(const FString& Key, const TSharedRef<FGtcJsonObject>& Value) { Set(Key, FGtcJsonValue::MakeObject(Value)); }
void FGtcJsonObject::SetArray(const FString& Key, const TArray<FGtcJsonValuePtr>& Value) { Set(Key, FGtcJsonValue::MakeArray(Value)); }

FGtcJsonValuePtr FGtcJsonObject::Get(const FString& Key) const
{
    const FGtcJsonValuePtr* Found = Map.Find(Key);
    return Found ? *Found : FGtcJsonValuePtr();
}

bool FGtcJsonObject::Has(const FString& Key) const { return Map.Contains(Key); }

bool FGtcJsonObject::HasObjectField(const FString& Key) const
{
    const FGtcJsonValuePtr V = Get(Key);
    return V.IsValid() && V->IsObject();
}

double FGtcJsonObject::GetNumber(const FString& Key, double Fallback) const
{
    const FGtcJsonValuePtr V = Get(Key);
    return (V.IsValid() && V->IsNumber()) ? V->AsNumber() : Fallback;
}

FString FGtcJsonObject::GetString(const FString& Key, const FString& Fallback) const
{
    const FGtcJsonValuePtr V = Get(Key);
    return (V.IsValid() && V->GetType() == EGtcJsonType::String) ? V->AsString() : Fallback;
}

TSharedPtr<FGtcJsonObject> FGtcJsonObject::GetObject(const FString& Key) const
{
    const FGtcJsonValuePtr V = Get(Key);
    return (V.IsValid() && V->IsObject()) ? V->AsObject() : TSharedPtr<FGtcJsonObject>();
}

TArray<FGtcJsonValuePtr> FGtcJsonObject::GetArray(const FString& Key) const
{
    const FGtcJsonValuePtr V = Get(Key);
    return (V.IsValid() && V->IsArray()) ? V->AsArray() : TArray<FGtcJsonValuePtr>();
}

TSharedRef<FGtcJsonObject> FGtcJsonObject::DeepClone() const
{
    TSharedRef<FGtcJsonObject> Out = MakeShared<FGtcJsonObject>();
    for (const FString& Key : Keys)
    {
        const FGtcJsonValuePtr* Found = Map.Find(Key);
        Out->Set(Key, (Found && Found->IsValid()) ? (*Found)->DeepClone() : FGtcJsonValue::MakeNull());
    }
    return Out;
}

// ---- FGtcJsonValue ----------------------------------------------------------

FGtcJsonValuePtr FGtcJsonValue::MakeNull()
{
    return MakeShared<FGtcJsonValue>();
}

FGtcJsonValuePtr FGtcJsonValue::MakeBool(bool In)
{
    FGtcJsonValuePtr V = MakeShared<FGtcJsonValue>();
    V->Type = EGtcJsonType::Bool;
    V->BoolValue = In;
    return V;
}

FGtcJsonValuePtr FGtcJsonValue::MakeNumber(double In)
{
    FGtcJsonValuePtr V = MakeShared<FGtcJsonValue>();
    V->Type = EGtcJsonType::Number;
    V->NumberValue = In;
    return V;
}

FGtcJsonValuePtr FGtcJsonValue::MakeString(const FString& In)
{
    FGtcJsonValuePtr V = MakeShared<FGtcJsonValue>();
    V->Type = EGtcJsonType::String;
    V->StringValue = In;
    return V;
}

FGtcJsonValuePtr FGtcJsonValue::MakeArray(const TArray<FGtcJsonValuePtr>& In)
{
    FGtcJsonValuePtr V = MakeShared<FGtcJsonValue>();
    V->Type = EGtcJsonType::Array;
    V->ArrayValue = In;
    return V;
}

FGtcJsonValuePtr FGtcJsonValue::MakeObject(const TSharedRef<FGtcJsonObject>& In)
{
    FGtcJsonValuePtr V = MakeShared<FGtcJsonValue>();
    V->Type = EGtcJsonType::Object;
    V->ObjectValue = In;
    return V;
}

FGtcJsonValuePtr FGtcJsonValue::DeepClone() const
{
    switch (Type)
    {
    case EGtcJsonType::Null:   return MakeNull();
    case EGtcJsonType::Bool:   return MakeBool(BoolValue);
    case EGtcJsonType::Number: return MakeNumber(NumberValue);
    case EGtcJsonType::String: return MakeString(StringValue);
    case EGtcJsonType::Array:
    {
        TArray<FGtcJsonValuePtr> Cloned;
        Cloned.Reserve(ArrayValue.Num());
        for (const FGtcJsonValuePtr& Item : ArrayValue)
        {
            Cloned.Add(Item.IsValid() ? Item->DeepClone() : MakeNull());
        }
        return MakeArray(Cloned);
    }
    case EGtcJsonType::Object:
        return ObjectValue.IsValid() ? MakeObject(ObjectValue->DeepClone()) : MakeObject(MakeShared<FGtcJsonObject>());
    default:
        return MakeNull();
    }
}

// ---- Serialize (engine Json: ordered TJsonWriter) ---------------------------

namespace
{
    using FGtcJsonWriter = TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>;

    void WriteValue(const FGtcJsonValuePtr& Value, const TSharedRef<FGtcJsonWriter>& Writer);

    // Numbers carry one `double` type (the reference has no int/float split). Integral values that fit
    // exactly are written through the int64 overload to keep version/count fields clean; all
    // others go through the double overload (17 sig-digits). Exact text is NOT contractual
    // (round-trip tolerance contract) — this only mirrors the prior writer's clean integers.
    void WriteNumberValue(double V, const TSharedRef<FGtcJsonWriter>& Writer)
    {
        if (FMath::IsFinite(V) && V == FMath::TruncToDouble(V) && FMath::Abs(V) < 1e15)
        {
            Writer->WriteValue(static_cast<int64>(V));
        }
        else
        {
            Writer->WriteValue(V);
        }
    }

    // Object fields are emitted in OrderedKeys() order — this is how the reference observable
    // Dictionary key order survives serialization on top of engine JSON.
    void WriteObject(const TSharedRef<FGtcJsonObject>& Obj, const TSharedRef<FGtcJsonWriter>& Writer)
    {
        Writer->WriteObjectStart();
        for (const FString& Key : Obj->OrderedKeys())
        {
            const FGtcJsonValuePtr Field = Obj->Get(Key);
            const EGtcJsonType Type = Field.IsValid() ? Field->GetType() : EGtcJsonType::Null;
            switch (Type)
            {
            case EGtcJsonType::Object:
                Writer->WriteIdentifierPrefix(Key);
                WriteObject(Field->AsObject().IsValid() ? Field->AsObject().ToSharedRef()
                    : MakeShared<FGtcJsonObject>(), Writer);
                break;
            case EGtcJsonType::Array:
                Writer->WriteIdentifierPrefix(Key);
                WriteValue(Field, Writer);
                break;
            case EGtcJsonType::Bool:   Writer->WriteValue(Key, Field->AsBool()); break;
            case EGtcJsonType::String: Writer->WriteValue(Key, Field->AsString()); break;
            case EGtcJsonType::Number:
                Writer->WriteIdentifierPrefix(Key);
                WriteNumberValue(Field->AsNumber(), Writer);
                break;
            default:
                Writer->WriteNull(Key);
                break;
            }
        }
        Writer->WriteObjectEnd();
    }

    void WriteValue(const FGtcJsonValuePtr& Value, const TSharedRef<FGtcJsonWriter>& Writer)
    {
        if (!Value.IsValid())
        {
            Writer->WriteValue(nullptr);
            return;
        }
        switch (Value->GetType())
        {
        case EGtcJsonType::Null:   Writer->WriteValue(nullptr); break;
        case EGtcJsonType::Bool:   Writer->WriteValue(Value->AsBool()); break;
        case EGtcJsonType::Number: WriteNumberValue(Value->AsNumber(), Writer); break;
        case EGtcJsonType::String: Writer->WriteValue(Value->AsString()); break;
        case EGtcJsonType::Array:
        {
            Writer->WriteArrayStart();
            for (const FGtcJsonValuePtr& Item : Value->AsArray())
            {
                WriteValue(Item, Writer);
            }
            Writer->WriteArrayEnd();
            break;
        }
        case EGtcJsonType::Object:
            WriteObject(Value->AsObject().IsValid() ? Value->AsObject().ToSharedRef()
                : MakeShared<FGtcJsonObject>(), Writer);
            break;
        }
    }
}

namespace GtcJson
{
    FString Serialize(const TSharedRef<FGtcJsonObject>& Object)
    {
        FString Out;
        const TSharedRef<FGtcJsonWriter> Writer =
            TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
        WriteObject(Object, Writer);
        Writer->Close();
        return Out;
    }
}

// ---- Parse (engine Json: ordered TJsonReader token stream) ------------------

namespace
{
    using FGtcJsonReader = TJsonReader<TCHAR>;

    // Build a value for the token the reader currently sits on. For containers, recurse and
    // consume to the matching End. Object fields are appended in document order via Set(),
    // so the ordered key list rebuilds exactly as written. Any malformed token / reader error
    // surfaces as bOk = false (silent — the reference returns an invalid value, never logs).
    FGtcJsonValuePtr ReadValue(const TSharedRef<FGtcJsonReader>& Reader, EJsonNotation Notation, bool& bOk);

    FGtcJsonValuePtr ReadObject(const TSharedRef<FGtcJsonReader>& Reader, bool& bOk)
    {
        TSharedRef<FGtcJsonObject> Obj = MakeShared<FGtcJsonObject>();
        EJsonNotation Notation;
        while (Reader->ReadNext(Notation))
        {
            if (Notation == EJsonNotation::ObjectEnd)
            {
                return FGtcJsonValue::MakeObject(Obj);
            }
            // Every field token carries its key as the identifier.
            const FString Key = Reader->GetIdentifier();
            const FGtcJsonValuePtr Val = ReadValue(Reader, Notation, bOk);
            if (!bOk)
            {
                return nullptr;
            }
            Obj->Set(Key, Val);
        }
        bOk = false; // ran out of tokens before ObjectEnd
        return nullptr;
    }

    FGtcJsonValuePtr ReadArray(const TSharedRef<FGtcJsonReader>& Reader, bool& bOk)
    {
        TArray<FGtcJsonValuePtr> Items;
        EJsonNotation Notation;
        while (Reader->ReadNext(Notation))
        {
            if (Notation == EJsonNotation::ArrayEnd)
            {
                return FGtcJsonValue::MakeArray(Items);
            }
            const FGtcJsonValuePtr Item = ReadValue(Reader, Notation, bOk);
            if (!bOk)
            {
                return nullptr;
            }
            Items.Add(Item);
        }
        bOk = false; // ran out of tokens before ArrayEnd
        return nullptr;
    }

    FGtcJsonValuePtr ReadValue(const TSharedRef<FGtcJsonReader>& Reader, EJsonNotation Notation, bool& bOk)
    {
        switch (Notation)
        {
        case EJsonNotation::ObjectStart: return ReadObject(Reader, bOk);
        case EJsonNotation::ArrayStart:  return ReadArray(Reader, bOk);
        case EJsonNotation::Boolean:     return FGtcJsonValue::MakeBool(Reader->GetValueAsBoolean());
        case EJsonNotation::Number:      return FGtcJsonValue::MakeNumber(Reader->GetValueAsNumber());
        case EJsonNotation::String:      return FGtcJsonValue::MakeString(Reader->GetValueAsString());
        case EJsonNotation::Null:        return FGtcJsonValue::MakeNull();
        default:
            bOk = false; // EJsonNotation::Error or an End where a value was expected
            return nullptr;
        }
    }
}

namespace GtcJson
{
    FGtcJsonValuePtr Parse(const FString& Text)
    {
        const TSharedRef<FGtcJsonReader> Reader = TJsonReaderFactory<TCHAR>::Create(Text);

        EJsonNotation Notation;
        if (!Reader->ReadNext(Notation))
        {
            return nullptr; // empty / immediate error
        }

        bool bOk = true;
        FGtcJsonValuePtr Value = ReadValue(Reader, Notation, bOk);
        if (!bOk || !Value.IsValid())
        {
            return nullptr;
        }

        // Reject trailing non-whitespace (the reference rejects it). After a complete top-level value
        // the next ReadNext must report end-of-stream (ReadNext returns false), not another
        // token. A trailing-content error also returns false, which is the same reject path.
        if (Reader->ReadNext(Notation))
        {
            return nullptr;
        }
        return Value;
    }
}
