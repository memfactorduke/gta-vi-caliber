// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcHail.h"
#include "../../Decision/NpcOccupation.h"
#include "../../Archetypes/NpcArchetypes.h"

/**
 * Behavioural tests for FNpcHail: each talky job role hails the player with
 * non-empty lines that wrap by index, the mime stays silent, unknown roles say
 * nothing, and every hailing role is a real FNpcOccupation role (kept in step).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcHailTest,
	"GTC.NPC.Dialogue.NpcHail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcHailTest::RunTest(const FString& Parameters)
{
	// --- A job role hails, and selection wraps --------------------------------
	{
		const FName Role = TEXT("vendor");
		const int32 Count = FNpcHail::Count(Role);
		TestTrue(TEXT("vendor hails"), Count > 0);
		TestNotEqual(TEXT("vendor hail non-empty"), FNpcHail::Line(Role, 0), FString());
		TestEqual(TEXT("index wraps (posmod)"), FNpcHail::Line(Role, Count), FNpcHail::Line(Role, 0));
		TestNotEqual(TEXT("negative index safe"), FNpcHail::Line(Role, -1), FString());
	}

	// --- Mime and unknown roles say nothing -----------------------------------
	{
		TestEqual(TEXT("mime does not hail"), FNpcHail::Count(TEXT("mime")), 0);
		TestEqual(TEXT("mime hail is empty"), FNpcHail::Line(TEXT("mime"), 0), FString());
		TestEqual(TEXT("unknown role does not hail"), FNpcHail::Count(TEXT("nobody")), 0);
		TestEqual(TEXT("None role is empty"), FNpcHail::Line(NAME_None, 0), FString());
	}

	// --- Every NON-mime archetype's role hails; the mime's does not -----------
	// Keeps the hail banks in lockstep with FNpcOccupation role coverage.
	{
		bool bAllConsistent = true;
		for (const FNpcArchetype& A : FNpcArchetypes::All())
		{
			const FName Role = FNpcOccupation::RoleFor(A.Id);
			const bool bMime = (Role == FName(TEXT("mime")));
			const bool bHails = FNpcHail::Count(Role) > 0;
			if (bMime && bHails) { bAllConsistent = false; AddError(TEXT("mime should not hail")); }
			if (!bMime && !bHails)
			{
				bAllConsistent = false;
				AddError(FString::Printf(TEXT("role '%s' (%s) has no hail bank"), *Role.ToString(), *A.Id));
			}
		}
		TestTrue(TEXT("every non-mime role hails, mime stays silent"), bAllConsistent);
	}

	// --- Determinism ----------------------------------------------------------
	{
		TestEqual(TEXT("same role+index is the same hail"),
			FNpcHail::Line(TEXT("life_coach"), 4), FNpcHail::Line(TEXT("life_coach"), 4));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
