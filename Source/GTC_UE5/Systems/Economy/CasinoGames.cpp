// Copyright Epic Games, Inc. All Rights Reserved.

#include "CasinoGames.h"

const TArray<FString>& CasinoGames::SlotSymbols()
{
    static const TArray<FString> Symbols = {
        TEXT("cherry"), TEXT("lemon"), TEXT("bell"), TEXT("bar"), TEXT("seven")
    };
    return Symbols;
}

const TArray<int32>& CasinoGames::RouletteRed()
{
    static const TArray<int32> Red = {
        1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36
    };
    return Red;
}

const TMap<FString, int32>& CasinoGames::SlotTripleMult()
{
    static const TMap<FString, int32> Mult = {
        { TEXT("cherry"), 5 },
        { TEXT("lemon"), 10 },
        { TEXT("bell"), 20 },
        { TEXT("bar"), 50 },
        { TEXT("seven"), 100 },
    };
    return Mult;
}

CasinoGames::CasinoGames(int32 InStartingChips)
{
    StartingChips = FMath::Max(InStartingChips, 0);
    ChipBalance = StartingChips;
}

int32 CasinoGames::RouletteSpin(FRandomStream& Rng)
{
    return Rng.RandRange(0, RoulettePockets - 1);
}

bool CasinoGames::RouletteIsRed(int32 NumberLanded)
{
    return RouletteRed().Contains(NumberLanded);
}

int32 CasinoGames::RoulettePayout(const FString& BetType, int32 NumberLanded, int32 BetAmount)
{
    const int32 Stake = FMath::Max(BetAmount, 0);
    if (Stake == 0)
    {
        return 0;
    }
    if (NumberLanded < 0 || NumberLanded >= RoulettePockets)
    {
        return 0;
    }
    if (BetType == TEXT("straight"))
    {
        return Stake * 36;
    }
    // 0 loses every outside bet.
    if (NumberLanded == 0)
    {
        return 0;
    }
    if (BetType == TEXT("dozen1"))
    {
        return NumberLanded <= 12 ? Stake * 3 : 0;
    }
    if (BetType == TEXT("dozen2"))
    {
        return (NumberLanded >= 13 && NumberLanded <= 24) ? Stake * 3 : 0;
    }
    if (BetType == TEXT("dozen3"))
    {
        return NumberLanded >= 25 ? Stake * 3 : 0;
    }
    bool bWon = false;
    if (BetType == TEXT("red"))
    {
        bWon = RouletteIsRed(NumberLanded);
    }
    else if (BetType == TEXT("black"))
    {
        bWon = !RouletteIsRed(NumberLanded);
    }
    else if (BetType == TEXT("even"))
    {
        bWon = NumberLanded % 2 == 0;
    }
    else if (BetType == TEXT("odd"))
    {
        bWon = NumberLanded % 2 == 1;
    }
    else if (BetType == TEXT("low"))
    {
        bWon = NumberLanded >= 1 && NumberLanded <= 18;
    }
    else if (BetType == TEXT("high"))
    {
        bWon = NumberLanded >= 19 && NumberLanded <= 36;
    }
    else
    {
        return 0;
    }
    return bWon ? Stake * 2 : 0;
}

TArray<FString> CasinoGames::SlotSpin(FRandomStream& Rng, int32 Reels)
{
    TArray<FString> Out;
    const int32 Count = FMath::Max(Reels, 1);
    const TArray<FString>& Symbols = SlotSymbols();
    for (int32 I = 0; I < Count; ++I)
    {
        Out.Add(Symbols[Rng.RandRange(0, Symbols.Num() - 1)]);
    }
    return Out;
}

int32 CasinoGames::SlotPayout(const TArray<FString>& Result, int32 BetAmount)
{
    const int32 Stake = FMath::Max(BetAmount, 0);
    if (Stake == 0 || Result.Num() == 0)
    {
        return 0;
    }
    TMap<FString, int32> Counts;
    for (const FString& Symbol : Result)
    {
        Counts.FindOrAdd(Symbol) += 1;
    }
    int32 Best = 0;
    for (const TPair<FString, int32>& Pair : Counts)
    {
        const int32 N = Pair.Value;
        const int32* TripleMult = SlotTripleMult().Find(Pair.Key);
        if (N >= 3 && TripleMult)
        {
            Best = FMath::Max(Best, Stake * (*TripleMult));
        }
        else if (N == 2)
        {
            Best = FMath::Max(Best, Stake * SlotPartialMult);
        }
    }
    return Best;
}

int32 CasinoGames::HandValue(const TArray<FString>& Cards)
{
    int32 Total = 0;
    int32 Aces = 0;
    for (const FString& Card : Cards)
    {
        const int32 Rank = CardRank(Card);
        if (Rank == 1)
        {
            Aces += 1;
            Total += 11;
        }
        else if (Rank >= 10)
        {
            Total += 10;
        }
        else
        {
            Total += Rank;
        }
    }
    while (Total > 21 && Aces > 0)
    {
        Total -= 10;
        Aces -= 1;
    }
    return Total;
}

bool CasinoGames::IsBlackjack(const TArray<FString>& Cards)
{
    return Cards.Num() == 2 && HandValue(Cards) == 21;
}

bool CasinoGames::IsBust(int32 Value)
{
    return Value > 21;
}

bool CasinoGames::DealerShouldHit(int32 Value)
{
    return Value < DealerStand;
}

int32 CasinoGames::BlackjackSettle(int32 PlayerValue, int32 DealerValue, int32 Bet, int32 PlayerCardCount)
{
    const int32 Stake = FMath::Max(Bet, 0);
    if (Stake == 0)
    {
        return 0;
    }
    if (IsBust(PlayerValue))
    {
        return 0;
    }
    const bool bPlayerNatural = PlayerValue == 21 && PlayerCardCount == 2;
    if (IsBust(DealerValue))
    {
        return NaturalOrEven(bPlayerNatural, Stake);
    }
    if (PlayerValue > DealerValue)
    {
        return NaturalOrEven(bPlayerNatural, Stake);
    }
    if (PlayerValue == DealerValue)
    {
        return Stake;
    }
    return 0;
}

bool CasinoGames::PlaceBet(int32 Amount)
{
    if (Amount <= 0 || Amount > ChipBalance)
    {
        return false;
    }
    ChipBalance -= Amount;
    return true;
}

void CasinoGames::Win(int32 Amount)
{
    ChipBalance += FMath::Max(Amount, 0);
}

int32 CasinoGames::Chips() const
{
    return ChipBalance;
}

bool CasinoGames::IsBroke() const
{
    return ChipBalance <= 0;
}

void CasinoGames::Reset()
{
    ChipBalance = StartingChips;
}

double CasinoGames::HouseEdge(const FString& Game)
{
    if (Game == TEXT("roulette"))
    {
        return 0.027;
    }
    if (Game == TEXT("slots"))
    {
        return 0.08;
    }
    if (Game == TEXT("blackjack"))
    {
        return 0.005;
    }
    return 0.0;
}

FRandomStream CasinoGames::MakeRng(int32 Seed)
{
    return FRandomStream(Seed);
}

int32 CasinoGames::NaturalOrEven(bool bPlayerNatural, int32 Stake)
{
    if (bPlayerNatural)
    {
        return static_cast<int32>(static_cast<double>(Stake) * BlackjackReturnMult);
    }
    return Stake * 2;
}

int32 CasinoGames::CardRank(const FString& Card)
{
    const FString Name = Card.ToUpper();
    if (Name == TEXT("A") || Name == TEXT("ACE"))
    {
        return 1;
    }
    if (Name == TEXT("J") || Name == TEXT("JACK"))
    {
        return 11;
    }
    if (Name == TEXT("Q") || Name == TEXT("QUEEN"))
    {
        return 12;
    }
    if (Name == TEXT("K") || Name == TEXT("KING"))
    {
        return 13;
    }
    return FMath::Clamp(FCString::Atoi(*Name), 1, 13);
}
