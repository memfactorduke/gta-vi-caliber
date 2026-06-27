// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RadioTuner.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"
#include <initializer_list>

using GtcTest::Eps;

/**
 * Tests for FRadioTuner — the in-vehicle radio dial. GTC-original (no Godot
 * oracle). Pins the ring scrolling through stations + Radio Off, the continuous
 * "live broadcast" clock (you rejoin a station mid-track, not where you left it),
 * mid-track join offsets, the looping playlist + track boundaries, per-station
 * phase desync, skipped zero-length tracks, and the safe degenerate cases
 * (no stations, dead-air station, out-of-range dial). Prefix GTC.Systems.Radio.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace RadioTunerTest
{
    using FStation = FRadioTuner::FStation;

    // A station whose tracks are all the same length, with an optional phase.
    FStation Even(int32 Count, double Length, double Phase = 0.0)
    {
        FStation S;
        S.PhaseSeconds = Phase;
        for (int32 I = 0; I < Count; ++I)
        {
            S.TrackSeconds.Add(Length);
        }
        return S;
    }

    FStation FromLengths(std::initializer_list<double> Lengths, double Phase = 0.0)
    {
        FStation S;
        S.PhaseSeconds = Phase;
        for (const double L : Lengths)
        {
            S.TrackSeconds.Add(L);
        }
        return S;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRadioTunerTest,
    "GTC.Systems.Radio",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRadioTunerTest::RunTest(const FString& Parameters)
{
    using namespace RadioTunerTest;
    using FNowPlaying = FRadioTuner::FNowPlaying;

    // ---- A fresh dial is Off, clock at zero, playing nothing -------------------
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0)});
        TestFalse(TEXT("fresh dial is off"), R.IsOn());
        TestEqual(TEXT("fresh dial reports OffStation"), R.Station(), FRadioTuner::OffStation);
        TestEqual(TEXT("one station configured"), R.NumStations(), 1);
        TestTrue(TEXT("fresh clock is zero"), FMath::Abs(R.Clock()) <= Eps);
        TestEqual(TEXT("off dial plays no track"), R.NowPlaying().TrackIndex, INDEX_NONE);
    }

    // ---- TuneTo selects a station; NowPlaying starts at track 0, offset 0 ------
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0)});
        R.TuneTo(0);
        TestTrue(TEXT("tuned dial is on"), R.IsOn());
        TestEqual(TEXT("tuned to station 0"), R.Station(), 0);
        const FNowPlaying NP = R.NowPlaying();
        TestEqual(TEXT("starts on track 0"), NP.TrackIndex, 0);
        TestTrue(TEXT("starts at offset 0"), FMath::Abs(NP.TrackOffset) <= Eps);
    }

    // ---- Out-of-range TuneTo is treated as Off (safe restore) ------------------
    {
        FRadioTuner R;
        R.Configure({Even(2, 50.0)});
        R.TuneTo(0);
        R.TuneTo(99);
        TestFalse(TEXT("out-of-range tune falls to Off"), R.IsOn());
        R.TuneTo(-5);
        TestFalse(TEXT("negative tune falls to Off"), R.IsOn());
    }

    // ---- Advancing the clock walks the offset, then rolls to the next track ----
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0)});
        R.TuneTo(0);
        R.Advance(30.0);
        FNowPlaying NP = R.NowPlaying();
        TestEqual(TEXT("still track 0 at t=30"), NP.TrackIndex, 0);
        TestTrue(TEXT("offset 30 into track 0"), FMath::IsNearlyEqual(NP.TrackOffset, 30.0, Eps));

        R.Advance(80.0); // t=110 -> 10s into track 1
        NP = R.NowPlaying();
        TestEqual(TEXT("rolled to track 1 at t=110"), NP.TrackIndex, 1);
        TestTrue(TEXT("offset 10 into track 1"), FMath::IsNearlyEqual(NP.TrackOffset, 10.0, Eps));
    }

    // ---- The playlist loops: one full lap returns to track 0, offset 0 ---------
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0)}); // 300s loop
        R.TuneTo(0);
        R.Advance(300.0);
        const FNowPlaying NP = R.NowPlaying();
        TestEqual(TEXT("wraps to track 0 after a full loop"), NP.TrackIndex, 0);
        TestTrue(TEXT("offset 0 after a full loop"), FMath::Abs(NP.TrackOffset) <= Eps);

        R.Advance(150.0); // t=450 -> 150 into loop -> 50 into track 1
        const FNowPlaying NP2 = R.NowPlaying();
        TestEqual(TEXT("track 1 after 1.5 loops"), NP2.TrackIndex, 1);
        TestTrue(TEXT("offset 50 after 1.5 loops"), FMath::IsNearlyEqual(NP2.TrackOffset, 50.0, Eps));
    }

    // ---- THE LIVE BROADCAST: tuning away and back rejoins further on -----------
    // Stations keep playing while you're not listening; you do NOT resume where
    // you left. Tune A, listen 40s, tune to B for 200s, tune back to A: A must be
    // at t=240 on its own clock (track 2, offset 40), not back at t=40.
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0), Even(3, 100.0)});
        R.TuneTo(0);
        R.Advance(40.0);
        TestEqual(TEXT("A is on track 0 at t=40"), R.NowPlaying().TrackIndex, 0);

        R.TuneTo(1);     // hop to B; A keeps broadcasting in the background
        R.Advance(200.0); // total clock now 240
        TestTrue(TEXT("B is on while tuned to it"), R.IsOn());

        R.TuneTo(0);     // back to A — should have advanced the whole 240s
        const FNowPlaying NP = R.NowPlaying();
        TestEqual(TEXT("A rejoined live at track 2 (t=240)"), NP.TrackIndex, 2);
        TestTrue(TEXT("A offset 40 into track 2"), FMath::IsNearlyEqual(NP.TrackOffset, 40.0, Eps));
    }

    // ---- Per-station phase desyncs stations at the same world time -------------
    {
        FRadioTuner R;
        // Same playlist, but station 1 is phased 150s ahead.
        R.Configure({Even(3, 100.0, /*phase*/ 0.0), Even(3, 100.0, /*phase*/ 150.0)});
        R.TuneTo(0);
        const FNowPlaying A = R.NowPlaying(); // t=0 -> track 0, offset 0
        R.TuneTo(1);
        const FNowPlaying B = R.NowPlaying(); // t=0 + phase 150 -> track 1, offset 50
        TestEqual(TEXT("phased station starts on a different track"), B.TrackIndex, 1);
        TestTrue(TEXT("phased station starts at offset 50"), FMath::IsNearlyEqual(B.TrackOffset, 50.0, Eps));
        TestNotEqual(TEXT("the two stations are desynced"), A.TrackIndex, B.TrackIndex);
    }

    // ---- Next/Previous scroll the ring: stations 0..N-1 then Off, wrapping -----
    {
        FRadioTuner R;
        R.Configure({Even(1, 10.0), Even(1, 10.0)}); // 2 stations + Off = ring of 3
        TestFalse(TEXT("starts Off"), R.IsOn());

        R.Next();
        TestEqual(TEXT("Next from Off -> station 0"), R.Station(), 0);
        R.Next();
        TestEqual(TEXT("Next -> station 1"), R.Station(), 1);
        R.Next();
        TestFalse(TEXT("Next past last station -> Off"), R.IsOn());
        R.Next();
        TestEqual(TEXT("Next wraps Off -> station 0"), R.Station(), 0);

        R.Previous();
        TestFalse(TEXT("Previous from station 0 -> Off"), R.IsOn());
        R.Previous();
        TestEqual(TEXT("Previous wraps Off -> station 1"), R.Station(), 1);
    }

    // ---- Zero-length tracks are skipped (never reported, no dead air) ----------
    {
        FRadioTuner R;
        R.Configure({FromLengths({60.0, 0.0, 40.0})}); // middle track is empty
        R.TuneTo(0);
        R.Advance(60.0); // exactly past track 0 -> should land on track 2, not the 0-length track 1
        const FNowPlaying NP = R.NowPlaying();
        TestEqual(TEXT("skips the zero-length track"), NP.TrackIndex, 2);
        TestTrue(TEXT("offset 0 into track 2"), FMath::Abs(NP.TrackOffset) <= Eps);

        R.Advance(40.0); // t=100 = full loop (60+40) -> back to track 0
        TestEqual(TEXT("loop length ignores empty track"), R.NowPlaying().TrackIndex, 0);
    }

    // ---- A dead-air station (all tracks empty) plays nothing safely ------------
    {
        FRadioTuner R;
        R.Configure({FromLengths({0.0, 0.0})});
        R.TuneTo(0);
        TestTrue(TEXT("dead-air station is still 'on'"), R.IsOn());
        TestEqual(TEXT("dead-air station plays no track"), R.NowPlaying().TrackIndex, INDEX_NONE);
        R.Advance(1000.0); // must not divide by zero or loop forever
        TestEqual(TEXT("dead-air station still plays nothing"), R.NowPlaying().TrackIndex, INDEX_NONE);
    }

    // ---- No stations configured: the ring is just Off; nothing crashes ---------
    {
        FRadioTuner R;
        R.Configure({});
        TestEqual(TEXT("no stations"), R.NumStations(), 0);
        R.Next();
        TestFalse(TEXT("Next with no stations stays Off"), R.IsOn());
        R.Previous();
        TestFalse(TEXT("Previous with no stations stays Off"), R.IsOn());
        R.TuneTo(0);
        TestFalse(TEXT("TuneTo with no stations stays Off"), R.IsOn());
        TestEqual(TEXT("NowPlaying with no stations is empty"), R.NowPlaying().TrackIndex, INDEX_NONE);
    }

    // ---- A negative Dt never runs the clock backwards -------------------------
    {
        FRadioTuner R;
        R.Configure({Even(3, 100.0)});
        R.TuneTo(0);
        R.Advance(50.0);
        R.Advance(-30.0); // ignored
        TestTrue(TEXT("negative Dt is ignored"), FMath::IsNearlyEqual(R.Clock(), 50.0, Eps));
        TestTrue(TEXT("offset unchanged by negative Dt"),
            FMath::IsNearlyEqual(R.NowPlaying().TrackOffset, 50.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
