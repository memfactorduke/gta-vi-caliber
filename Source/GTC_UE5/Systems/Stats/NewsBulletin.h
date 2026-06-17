// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure reactive-news model — the UE 5.7 port of Godot's NewsBulletin (RefCounted).
 * Turns the player's notable deeds (crimes, rampages, heists, clean escapes, stunts)
 * into radio/TV headlines, so the world talks about YOU. A controller calls Report()
 * when something newsworthy happens; the radio's NEWS slot pulls NextBulletin() for the
 * anchor to read. Wording escalates with severity. Plain C++ value type (no UObject).
 * Headless-testable (parity oracle game/tests/unit/test_news_bulletin.gd).
 *
 * No nodes, no scene access, deterministic (templates chosen by severity, not RNG): a
 * controller (CrimeReactionDirector) owns one and drains it. Report(kind, severity,
 * context): severity is 1..5 (clamped) and picks the wording tier; context fills {place}
 * and {count} placeholders (defaults "the city" / 0).
 *
 * Parity note: HEADLINES kinds iterate in declaration order (Kinds()), so an ordered
 * TArray of {Kind, Templates} plus a TMap kind->index preserves that observable order.
 */
class GTC_UE5_API FNewsBulletin
{
public:
    /** Generic line for the news slot when nothing newsworthy is queued. */
    static const FString Filler;

    /** How many pending bulletins to hold before dropping the oldest. */
    static constexpr int32 MaxQueue = 12;

    /** One pending bulletin: {Kind, Severity, Text}. */
    struct FItem
    {
        FString Kind;
        int32 Severity = 1;
        FString Text;
    };

    FNewsBulletin() = default;

    /** Every recognised event kind, in declaration order. */
    TArray<FString> Kinds() const;

    /**
     * Report a newsworthy event, enqueuing a headline. Severity is clamped to 1..5;
     * Place defaults to "the city" and Count to 0. Returns the headline text, or "" for
     * an unknown kind. Mirrors Godot report(kind, severity, context).
     */
    FString Report(const FString& Kind, int32 Severity, const FString& Place = TEXT("the city"), int32 Count = 0);

    bool HasPending() const { return _Queue.Num() > 0; }

    int32 PendingCount() const { return _Queue.Num(); }

    /**
     * Pull the next headline for the news anchor (FIFO). Returns Filler when nothing is
     * queued, so the slot always has something to read. Mirrors Godot next_bulletin().
     */
    FString NextBulletin();

    /** The most recently reported headline still in the queue, or "" if empty. */
    FString PeekLatest() const;

    /** Up to Limit of the most recent queued headlines (newest first). */
    TArray<FString> Recent(int32 Limit) const;

    /** Drop all pending bulletins. */
    void Clear();

private:
    /** kind -> headline templates, ascending by severity. */
    static const TArray<TPair<FString, TArray<FString>>>& HeadlineTable();

    /** Find the templates for a kind, or nullptr if unknown. */
    static const TArray<FString>* FindTemplates(const FString& Kind);

    /** FIFO of pending bulletins. */
    TArray<FItem> _Queue;
};
