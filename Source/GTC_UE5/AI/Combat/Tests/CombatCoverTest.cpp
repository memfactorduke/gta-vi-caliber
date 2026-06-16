// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../CombatCover.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Parity oracle: game/tests/unit/test_combat_cover.gd. Each It(...) maps 1:1 to
// one Godot test_* function, asserting the SAME conditions. The Godot _cover(pos,
// normal) Dictionary helper becomes the FCombatCover::FCoverPoint value type.
BEGIN_DEFINE_SPEC(FCombatCoverSpec, "GTC.AI.Combat.CombatCover",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
    static FCombatCover::FCoverPoint Cover(const FVector& Pos, const FVector& Normal)
    {
        FCombatCover::FCoverPoint C;
        C.Pos = Pos;
        C.Normal = Normal;
        return C;
    }
END_DEFINE_SPEC(FCombatCoverSpec)

void FCombatCoverSpec::Define()
{
    // --- provides_cover ----------------------------------------------------

    // test_provides_cover_threat_on_faced_side
    It("protects when the threat is on the faced side", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestTrue(TEXT("threat +X is blocked"), FCombatCover::ProvidesCover(C, FVector(5, 0, 0)));
    });

    // test_provides_cover_false_when_threat_behind_open_side
    It("does not protect when the threat is on the open side", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestFalse(TEXT("threat -X not blocked"), FCombatCover::ProvidesCover(C, FVector(-5, 0, 0)));
    });

    // test_provides_cover_false_for_zero_normal
    It("does not protect with a zero normal", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector::ZeroVector);
        TestFalse(TEXT("zero normal blocks nothing"), FCombatCover::ProvidesCover(C, FVector(5, 0, 0)));
    });

    // test_provides_cover_ignores_vertical
    It("ignores the vertical when checking cover", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestTrue(TEXT("threat +X with Y offset still blocked"),
            FCombatCover::ProvidesCover(C, FVector(5, 99, 0)));
    });

    // test_provides_cover_threat_coincident_no_nan
    It("does not protect a coincident threat without NaN", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector(3, 0, 3), FVector(0, 0, 1));
        TestFalse(TEXT("coincident threat not protected"),
            FCombatCover::ProvidesCover(C, FVector(3, 0, 3)));
    });

    // --- cover_quality -----------------------------------------------------

    // test_quality_in_unit_range
    It("keeps cover quality in the unit range", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        const double Q = FCombatCover::CoverQuality(C, FVector(6, 0, 0), 0.5);
        TestTrue(TEXT("0 <= q <= 1"), Q >= 0.0 && Q <= 1.0);
    });

    // test_quality_square_beats_oblique
    It("scores a square face above an oblique one", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        const double Square = FCombatCover::CoverQuality(C, FVector(6, 0, 0), 0.5);
        const double Oblique = FCombatCover::CoverQuality(C, FVector(4.24, 0, 4.24), 0.5);
        TestTrue(TEXT("square > oblique"), Square > Oblique);
    });

    // test_quality_close_beats_far_for_square
    It("scores the nearer square cover no less than the far one", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        const double Near = FCombatCover::CoverQuality(C, FVector(6, 0, 0), 0.5);
        const double Far = FCombatCover::CoverQuality(C, FVector(11, 0, 0), 0.5);
        TestTrue(TEXT("near and far positive, near >= far"),
            Near > 0.0 && Far > 0.0 && Near >= Far);
    });

    // test_quality_zero_when_not_protecting
    It("scores zero when not protecting", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestTrue(TEXT("threat on open side scores zero"),
            FMath::IsNearlyEqual(FCombatCover::CoverQuality(C, FVector(-6, 0, 0), 0.5), 0.0, Eps));
    });

    // test_quality_zero_for_degenerate_normal
    It("scores zero for a degenerate normal", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector::ZeroVector);
        TestTrue(TEXT("degenerate normal scores zero"),
            FMath::IsNearlyEqual(FCombatCover::CoverQuality(C, FVector(6, 0, 0), 0.5), 0.0, Eps));
    });

    // test_quality_zero_when_coincident
    It("scores zero when coincident", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector(2, 0, 2), FVector(1, 0, 0));
        TestTrue(TEXT("coincident threat scores zero"),
            FMath::IsNearlyEqual(FCombatCover::CoverQuality(C, FVector(2, 0, 2), 0.5), 0.0, Eps));
    });

    // --- best_cover --------------------------------------------------------

    // test_best_cover_picks_protecting_and_nearest
    It("picks the nearest protecting cover", [this]()
    {
        const FCombatCover::FCoverPoint Near = Cover(FVector(1, 0, 0), FVector(1, 0, 0));
        const FCombatCover::FCoverPoint Far = Cover(FVector(8, 0, 0), FVector(1, 0, 0));
        const FCombatCover::FCoverPoint Best =
            FCombatCover::BestCover({Far, Near}, FVector::ZeroVector, FVector(20, 0, 0));
        TestTrue(TEXT("nearer wins"), Best.bValid && Best.Pos == FVector(1, 0, 0));
    });

    // test_best_cover_skips_non_protecting_even_if_nearer
    It("skips a nearer non-protecting cover", [this]()
    {
        const FCombatCover::FCoverPoint NearWrong = Cover(FVector(1, 0, 0), FVector(-1, 0, 0));
        const FCombatCover::FCoverPoint FarRight = Cover(FVector(5, 0, 0), FVector(1, 0, 0));
        const FCombatCover::FCoverPoint Best =
            FCombatCover::BestCover({NearWrong, FarRight}, FVector::ZeroVector, FVector(20, 0, 0));
        TestTrue(TEXT("protecting one chosen"), Best.bValid && Best.Pos == FVector(5, 0, 0));
    });

    // test_best_cover_empty_list
    It("returns no cover for an empty list", [this]()
    {
        const FCombatCover::FCoverPoint Best =
            FCombatCover::BestCover({}, FVector::ZeroVector, FVector(5, 0, 0));
        TestFalse(TEXT("empty list -> invalid"), Best.bValid);
    });

    // test_best_cover_none_protect
    It("returns no cover when none protect", [this]()
    {
        const FCombatCover::FCoverPoint A = Cover(FVector(1, 0, 0), FVector(-1, 0, 0));
        const FCombatCover::FCoverPoint B = Cover(FVector(2, 0, 0), FVector(-1, 0, 0));
        const FCombatCover::FCoverPoint Best =
            FCombatCover::BestCover({A, B}, FVector::ZeroVector, FVector(20, 0, 0));
        TestFalse(TEXT("none protect -> invalid"), Best.bValid);
    });

    // --- peek_position -----------------------------------------------------

    // test_peek_is_offset_sideways
    It("offsets the peek sideways", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(0, 0, 1));
        const FVector Peek = FCombatCover::PeekPosition(C, FVector(0, 0, 10), 1.5);
        TestTrue(TEXT("steps along X, not Z"),
            FMath::IsNearlyEqual(FMath::Abs(Peek.X), 1.5, Eps) && FMath::IsNearlyEqual(Peek.Z, 0.0, Eps));
    });

    // test_peek_not_on_threat_line
    It("keeps the peek off the threat line", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(0, 0, 1));
        const FVector Peek = FCombatCover::PeekPosition(C, FVector(0, 0, 10), 2.0);
        TestTrue(TEXT("lateral offset present"), FMath::Abs(Peek.X) > 0.0001);
    });

    // test_peek_sign_flips_side
    It("flips the peek side with the sign", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(0, 0, 1));
        const FVector Right = FCombatCover::PeekPosition(C, FVector(0, 0, 10), 2.0);
        const FVector Left = FCombatCover::PeekPosition(C, FVector(0, 0, 10), -2.0);
        TestTrue(TEXT("right.x == -left.x and nonzero"),
            FMath::IsNearlyEqual(Right.X, -Left.X, Eps) && FMath::Abs(Right.X) > 0.0001);
    });

    // test_peek_coincident_threat_falls_back_to_pos
    It("falls back to the cover pos for a coincident threat", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector(4, 0, 4), FVector(0, 0, 1));
        const FVector Peek = FCombatCover::PeekPosition(C, FVector(4, 0, 4), 2.0);
        TestTrue(TEXT("peek == pos"), Peek == FVector(4, 0, 4));
    });

    // --- is_exposed --------------------------------------------------------

    // test_is_exposed_false_when_tucked_behind
    It("is not exposed when tucked behind cover", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestFalse(TEXT("tucked on open side is shielded"),
            FCombatCover::IsExposed(FVector(-0.5, 0, 0), C, FVector(10, 0, 0)));
    });

    // test_is_exposed_true_when_stepped_wide
    It("is exposed when stepped onto the threat side", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestTrue(TEXT("stepped past the wall is exposed"),
            FCombatCover::IsExposed(FVector(1.0, 0, 0), C, FVector(10, 0, 0)));
    });

    // test_is_exposed_true_when_cover_cannot_protect
    It("is exposed when the cover cannot protect", [this]()
    {
        const FCombatCover::FCoverPoint C = Cover(FVector::ZeroVector, FVector(1, 0, 0));
        TestTrue(TEXT("threat on open side exposes anywhere"),
            FCombatCover::IsExposed(FVector(-0.5, 0, 0), C, FVector(-10, 0, 0)));
    });

    // --- threat_direction --------------------------------------------------

    // test_threat_direction_normalized
    It("normalizes the threat direction", [this]()
    {
        const FVector Dir = FCombatCover::ThreatDirection(FVector::ZeroVector, FVector(3, 0, 4));
        TestTrue(TEXT("unit length"), FMath::IsNearlyEqual(Dir.Size(), 1.0, Eps));
    });

    // test_threat_direction_horizontal
    It("keeps the threat direction horizontal", [this]()
    {
        const FVector Dir = FCombatCover::ThreatDirection(FVector::ZeroVector, FVector(0, 50, 10));
        TestTrue(TEXT("Y dropped, points +Z"),
            FMath::IsNearlyEqual(Dir.Y, 0.0, Eps) && FMath::IsNearlyEqual(Dir.Z, 1.0, Eps));
    });

    // test_threat_direction_zero_when_coincident
    It("returns zero direction when coincident", [this]()
    {
        const FVector Dir = FCombatCover::ThreatDirection(FVector(2, 0, 2), FVector(2, 0, 2));
        TestTrue(TEXT("zero direction"), Dir == FVector::ZeroVector);
    });
}

#endif // WITH_AUTOMATION_TESTS
