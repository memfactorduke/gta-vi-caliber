// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcDuress.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FNpcDuress — cower-vs-flee-vs-none at gunpoint. Prefix
 * GTC.NPC.Decision.NpcDuress.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcDuressTest,
    "GTC.NPC.Decision.NpcDuress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcDuressTest::RunTest(const FString& Parameters)
{
    using D = ENpcDuress;

    // Unarmed is never duress, at any range.
    TestEqual(TEXT("unarmed point-blank -> none"), FNpcDuress::Decide(1.0, false, 0.1), D::None);
    TestEqual(TEXT("unarmed far -> none"), FNpcDuress::Decide(50.0, false, 0.1), D::None);

    // Armed but beyond flee range: not yet immediate duress (ordinary threat read applies).
    TestEqual(TEXT("armed far -> none"), FNpcDuress::Decide(20.0, true, 0.1), D::None);

    // Armed at medium range: flee, regardless of nerve.
    TestEqual(TEXT("armed medium, timid -> flee"), FNpcDuress::Decide(8.0, true, 0.1), D::Flee);
    TestEqual(TEXT("armed medium, brave -> flee"), FNpcDuress::Decide(8.0, true, 0.95), D::Flee);

    // Armed point-blank: a scared citizen freezes (cower); a brave one still bolts.
    TestEqual(TEXT("point-blank, timid -> cower"), FNpcDuress::Decide(2.0, true, 0.2), D::Cower);
    TestEqual(TEXT("point-blank, brave -> flee"), FNpcDuress::Decide(2.0, true, 0.9), D::Flee);

    // --- Boundaries (lock the thresholds) ---------------------------------------
    // At exactly CowerRangeM, a timid citizen still cowers (inclusive <=).
    TestEqual(TEXT("at cower range, timid -> cower"),
        FNpcDuress::Decide(FNpcDuress::CowerRangeM, true, 0.2), D::Cower);
    // Just past cower range -> flee even when timid.
    TestEqual(TEXT("just past cower range -> flee"),
        FNpcDuress::Decide(FNpcDuress::CowerRangeM + GtcTest::Eps, true, 0.2), D::Flee);
    // At exactly FleeRangeM, armed -> flee; just past -> none.
    TestEqual(TEXT("at flee range -> flee"),
        FNpcDuress::Decide(FNpcDuress::FleeRangeM, true, 0.5), D::Flee);
    TestEqual(TEXT("just past flee range -> none"),
        FNpcDuress::Decide(FNpcDuress::FleeRangeM + GtcTest::Eps, true, 0.5), D::None);
    // Bravery boundary: at CowerNerve the citizen no longer cowers (it flees).
    TestEqual(TEXT("point-blank at CowerNerve -> flee"),
        FNpcDuress::Decide(2.0, true, FNpcDuress::CowerNerve), D::Flee);
    TestEqual(TEXT("point-blank just below CowerNerve -> cower"),
        FNpcDuress::Decide(2.0, true, FNpcDuress::CowerNerve - GtcTest::Eps), D::Cower);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
