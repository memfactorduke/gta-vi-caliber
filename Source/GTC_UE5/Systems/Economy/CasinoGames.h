// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure casino gambling model — blackjack, slots, and roulette payout/odds math plus
 * a small stateful chip bankroll for a session. The static helpers are pure odds/
 * payout functions; every spin/deal takes a caller-supplied FRandomStream so outcomes
 * are reproducible in tests. Headless-testable (parity oracle test_casino_games.gd).
 *
 * Payout helpers return the TOTAL chips returned to the player (stake included), or 0
 * on a loss.
 *
 * RNG parity note: spins use FRandomStream — deterministic and seed-reproducible
 * WITHIN UE5, NOT a Godot PCG reimplementation, so draws are NOT byte-identical to
 * Godot. The oracle only pins determinism (same seed -> same result) and range, never
 * an exact seed->value sequence, so FRandomStream satisfies every assertion. Mapping:
 * randi_range(lo, hi) -> RandRange(lo, hi) (inclusive both ends).
 */
class GTC_UE5_API CasinoGames
{
public:
    // --- Roulette ---
    /** European single-zero wheel: pockets 0..36. */
    static constexpr int32 RoulettePockets = 37;

    // --- Slots ---
    /** Two-of-a-kind (partial) pays this multiple of the bet. */
    static constexpr int32 SlotPartialMult = 2;

    // --- Blackjack ---
    /** Dealer stands on 17 and above, hits below. */
    static constexpr int32 DealerStand = 17;
    /** A blackjack pays 3:2 — total returned is 2.5x the stake. */
    static constexpr double BlackjackReturnMult = 2.5;

    /** Symbol ids on a reel, weakest to strongest. */
    static const TArray<FString>& SlotSymbols();
    /** Red numbers on a European roulette wheel. */
    static const TArray<int32>& RouletteRed();
    /** Three-of-a-kind payout multiplier per symbol id. */
    static const TMap<FString, int32>& SlotTripleMult();

    explicit CasinoGames(int32 StartingChips = 1000);

    // === Roulette ===
    static int32 RouletteSpin(FRandomStream& Rng);
    static bool RouletteIsRed(int32 NumberLanded);
    static int32 RoulettePayout(const FString& BetType, int32 NumberLanded, int32 BetAmount);

    // === Slots ===
    static TArray<FString> SlotSpin(FRandomStream& Rng, int32 Reels = 3);
    static int32 SlotPayout(const TArray<FString>& Result, int32 BetAmount);

    // === Blackjack ===
    /** Value of a hand; aces count 11 then drop to 1 to avoid a bust. */
    static int32 HandValue(const TArray<FString>& Cards);
    static bool IsBlackjack(const TArray<FString>& Cards);
    static bool IsBust(int32 Value);
    static bool DealerShouldHit(int32 Value);
    static int32 BlackjackSettle(int32 PlayerValue, int32 DealerValue, int32 Bet, int32 PlayerCardCount = 2);

    // === Bankroll ===
    bool PlaceBet(int32 Amount);
    void Win(int32 Amount);
    int32 Chips() const;
    bool IsBroke() const;
    void Reset();

    // === Info ===
    /** Documented nominal house edge per game (informational). Unknown -> 0.0. */
    static double HouseEdge(const FString& Game);

    /** Determinism helper: a fresh stream seeded with Seed. */
    static FRandomStream MakeRng(int32 Seed);

private:
    int32 StartingChips = 0;
    int32 ChipBalance = 0;

    static int32 NaturalOrEven(bool bPlayerNatural, int32 Stake);
    static int32 CardRank(const FString& Card);
};
