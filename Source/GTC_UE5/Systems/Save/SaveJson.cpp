// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveJson.h"

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

// ---- Serialize --------------------------------------------------------------

namespace
{
    void EscapeString(const FString& In, FString& Out)
    {
        Out.AppendChar(TEXT('"'));
        for (const TCHAR C : In)
        {
            switch (C)
            {
            case TEXT('"'):  Out += TEXT("\\\""); break;
            case TEXT('\\'): Out += TEXT("\\\\"); break;
            case TEXT('\b'): Out += TEXT("\\b"); break;
            case TEXT('\f'): Out += TEXT("\\f"); break;
            case TEXT('\n'): Out += TEXT("\\n"); break;
            case TEXT('\r'): Out += TEXT("\\r"); break;
            case TEXT('\t'): Out += TEXT("\\t"); break;
            default:
                if (C < 0x20)
                {
                    Out += FString::Printf(TEXT("\\u%04x"), static_cast<int32>(C));
                }
                else
                {
                    Out.AppendChar(C);
                }
                break;
            }
        }
        Out.AppendChar(TEXT('"'));
    }

    // Shortest round-trippable double text. UE's SanitizeFloat on a double via %.17g
    // preserves full precision; integral values render without a fractional part to keep
    // version/count fields clean. Note: exact text is NOT contractual (tolerance contract).
    void WriteNumber(double V, FString& Out)
    {
        if (FMath::IsFinite(V) && V == FMath::TruncToDouble(V) && FMath::Abs(V) < 1e15)
        {
            Out += FString::Printf(TEXT("%lld"), static_cast<int64>(V));
        }
        else
        {
            Out += FString::Printf(TEXT("%.17g"), V);
        }
    }

    void WriteValue(const FGtcJsonValuePtr& Value, FString& Out);

    void WriteObject(const TSharedRef<FGtcJsonObject>& Obj, FString& Out)
    {
        Out.AppendChar(TEXT('{'));
        const TArray<FString>& Keys = Obj->OrderedKeys();
        for (int32 i = 0; i < Keys.Num(); ++i)
        {
            if (i > 0)
            {
                Out.AppendChar(TEXT(','));
            }
            EscapeString(Keys[i], Out);
            Out.AppendChar(TEXT(':'));
            WriteValue(Obj->Get(Keys[i]), Out);
        }
        Out.AppendChar(TEXT('}'));
    }

    void WriteValue(const FGtcJsonValuePtr& Value, FString& Out)
    {
        if (!Value.IsValid())
        {
            Out += TEXT("null");
            return;
        }
        switch (Value->GetType())
        {
        case EGtcJsonType::Null:   Out += TEXT("null"); break;
        case EGtcJsonType::Bool:   Out += Value->AsBool() ? TEXT("true") : TEXT("false"); break;
        case EGtcJsonType::Number: WriteNumber(Value->AsNumber(), Out); break;
        case EGtcJsonType::String: EscapeString(Value->AsString(), Out); break;
        case EGtcJsonType::Array:
        {
            Out.AppendChar(TEXT('['));
            const TArray<FGtcJsonValuePtr>& Arr = Value->AsArray();
            for (int32 i = 0; i < Arr.Num(); ++i)
            {
                if (i > 0)
                {
                    Out.AppendChar(TEXT(','));
                }
                WriteValue(Arr[i], Out);
            }
            Out.AppendChar(TEXT(']'));
            break;
        }
        case EGtcJsonType::Object:
            WriteObject(Value->AsObject().IsValid() ? Value->AsObject().ToSharedRef()
                : MakeShared<FGtcJsonObject>(), Out);
            break;
        }
    }
}

namespace GtcJson
{
    FString Serialize(const TSharedRef<FGtcJsonObject>& Object)
    {
        FString Out;
        WriteObject(Object, Out);
        return Out;
    }
}

// ---- Parse ------------------------------------------------------------------

namespace
{
    // A tiny recursive-descent JSON parser. Silent on error: any failure returns nullptr.
    struct FJsonParser
    {
        const TCHAR* Ptr;
        const TCHAR* End;

        explicit FJsonParser(const FString& Text)
            : Ptr(*Text)
            , End(*Text + Text.Len())
        {
        }

        void SkipWhitespace()
        {
            while (Ptr < End && (*Ptr == TEXT(' ') || *Ptr == TEXT('\t') || *Ptr == TEXT('\n') || *Ptr == TEXT('\r')))
            {
                ++Ptr;
            }
        }

        bool AtEnd() const { return Ptr >= End; }
        TCHAR Peek() const { return Ptr < End ? *Ptr : TEXT('\0'); }

        bool Match(const TCHAR* Literal)
        {
            const TCHAR* P = Ptr;
            for (const TCHAR* L = Literal; *L; ++L, ++P)
            {
                if (P >= End || *P != *L)
                {
                    return false;
                }
            }
            Ptr = P;
            return true;
        }

        FGtcJsonValuePtr ParseValue()
        {
            SkipWhitespace();
            if (AtEnd())
            {
                return nullptr;
            }
            const TCHAR C = Peek();
            switch (C)
            {
            case TEXT('{'): return ParseObject();
            case TEXT('['): return ParseArray();
            case TEXT('"'):
            {
                FString S;
                return ParseString(S) ? FGtcJsonValue::MakeString(S) : nullptr;
            }
            case TEXT('t'): return Match(TEXT("true")) ? FGtcJsonValue::MakeBool(true) : nullptr;
            case TEXT('f'): return Match(TEXT("false")) ? FGtcJsonValue::MakeBool(false) : nullptr;
            case TEXT('n'): return Match(TEXT("null")) ? FGtcJsonValue::MakeNull() : nullptr;
            default: return ParseNumber();
            }
        }

        bool ParseString(FString& Out)
        {
            if (Peek() != TEXT('"'))
            {
                return false;
            }
            ++Ptr; // opening quote
            while (Ptr < End)
            {
                const TCHAR C = *Ptr++;
                if (C == TEXT('"'))
                {
                    return true;
                }
                if (C == TEXT('\\'))
                {
                    if (Ptr >= End)
                    {
                        return false;
                    }
                    const TCHAR Esc = *Ptr++;
                    switch (Esc)
                    {
                    case TEXT('"'):  Out.AppendChar(TEXT('"')); break;
                    case TEXT('\\'): Out.AppendChar(TEXT('\\')); break;
                    case TEXT('/'):  Out.AppendChar(TEXT('/')); break;
                    case TEXT('b'):  Out.AppendChar(TEXT('\b')); break;
                    case TEXT('f'):  Out.AppendChar(TEXT('\f')); break;
                    case TEXT('n'):  Out.AppendChar(TEXT('\n')); break;
                    case TEXT('r'):  Out.AppendChar(TEXT('\r')); break;
                    case TEXT('t'):  Out.AppendChar(TEXT('\t')); break;
                    case TEXT('u'):
                    {
                        if (Ptr + 4 > End)
                        {
                            return false;
                        }
                        int32 Code = 0;
                        for (int32 i = 0; i < 4; ++i)
                        {
                            const TCHAR H = *Ptr++;
                            int32 Digit;
                            if (H >= TEXT('0') && H <= TEXT('9')) Digit = H - TEXT('0');
                            else if (H >= TEXT('a') && H <= TEXT('f')) Digit = 10 + (H - TEXT('a'));
                            else if (H >= TEXT('A') && H <= TEXT('F')) Digit = 10 + (H - TEXT('A'));
                            else return false;
                            Code = (Code << 4) | Digit;
                        }
                        Out.AppendChar(static_cast<TCHAR>(Code));
                        break;
                    }
                    default:
                        return false;
                    }
                }
                else
                {
                    Out.AppendChar(C);
                }
            }
            return false; // unterminated
        }

        FGtcJsonValuePtr ParseNumber()
        {
            const TCHAR* Start = Ptr;
            if (Peek() == TEXT('-'))
            {
                ++Ptr;
            }
            bool AnyDigit = false;
            while (Ptr < End && FChar::IsDigit(*Ptr)) { ++Ptr; AnyDigit = true; }
            if (Ptr < End && *Ptr == TEXT('.'))
            {
                ++Ptr;
                while (Ptr < End && FChar::IsDigit(*Ptr)) { ++Ptr; AnyDigit = true; }
            }
            if (Ptr < End && (*Ptr == TEXT('e') || *Ptr == TEXT('E')))
            {
                ++Ptr;
                if (Ptr < End && (*Ptr == TEXT('+') || *Ptr == TEXT('-')))
                {
                    ++Ptr;
                }
                while (Ptr < End && FChar::IsDigit(*Ptr)) { ++Ptr; }
            }
            if (!AnyDigit)
            {
                return nullptr;
            }
            const FString NumStr(FStringView(Start, static_cast<int32>(Ptr - Start)));
            return FGtcJsonValue::MakeNumber(FCString::Atod(*NumStr));
        }

        FGtcJsonValuePtr ParseArray()
        {
            ++Ptr; // '['
            TArray<FGtcJsonValuePtr> Items;
            SkipWhitespace();
            if (Peek() == TEXT(']'))
            {
                ++Ptr;
                return FGtcJsonValue::MakeArray(Items);
            }
            while (true)
            {
                FGtcJsonValuePtr Item = ParseValue();
                if (!Item.IsValid())
                {
                    return nullptr;
                }
                Items.Add(Item);
                SkipWhitespace();
                const TCHAR C = Peek();
                if (C == TEXT(','))
                {
                    ++Ptr;
                    continue;
                }
                if (C == TEXT(']'))
                {
                    ++Ptr;
                    return FGtcJsonValue::MakeArray(Items);
                }
                return nullptr;
            }
        }

        FGtcJsonValuePtr ParseObject()
        {
            ++Ptr; // '{'
            TSharedRef<FGtcJsonObject> Obj = MakeShared<FGtcJsonObject>();
            SkipWhitespace();
            if (Peek() == TEXT('}'))
            {
                ++Ptr;
                return FGtcJsonValue::MakeObject(Obj);
            }
            while (true)
            {
                SkipWhitespace();
                FString Key;
                if (!ParseString(Key))
                {
                    return nullptr;
                }
                SkipWhitespace();
                if (Peek() != TEXT(':'))
                {
                    return nullptr;
                }
                ++Ptr;
                FGtcJsonValuePtr Val = ParseValue();
                if (!Val.IsValid())
                {
                    return nullptr;
                }
                Obj->Set(Key, Val);
                SkipWhitespace();
                const TCHAR C = Peek();
                if (C == TEXT(','))
                {
                    ++Ptr;
                    continue;
                }
                if (C == TEXT('}'))
                {
                    ++Ptr;
                    return FGtcJsonValue::MakeObject(Obj);
                }
                return nullptr;
            }
        }
    };
}

namespace GtcJson
{
    FGtcJsonValuePtr Parse(const FString& Text)
    {
        const FString Trimmed = Text;
        FJsonParser Parser(Trimmed);
        Parser.SkipWhitespace();
        if (Parser.AtEnd())
        {
            return nullptr;
        }
        FGtcJsonValuePtr Value = Parser.ParseValue();
        if (!Value.IsValid())
        {
            return nullptr;
        }
        // Trailing non-whitespace makes the whole parse invalid (Godot rejects it).
        Parser.SkipWhitespace();
        if (!Parser.AtEnd())
        {
            return nullptr;
        }
        return Value;
    }
}
