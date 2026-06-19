// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcOccupation.h"
#include "../../Archetypes/NpcArchetypes.h"

/**
 * Behavioural tests for FNpcOccupation: every archetype maps to a job role, each
 * role yields role-specific work actions, selection wraps deterministically, and
 * unknown ids fall back to generic on-shift business.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNpcOccupationTest,
	"GTC.NPC.Decision.NpcOccupation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcOccupationTest::RunTest(const FString& Parameters)
{
	// --- Every shipped archetype has a concrete job role ----------------------
	{
		bool bAllHaveRoles = true;
		for (const FNpcArchetype& A : FNpcArchetypes::All())
		{
			if (FNpcOccupation::RoleFor(A.Id).IsNone())
			{
				bAllHaveRoles = false;
				AddError(FString::Printf(TEXT("archetype '%s' has no role"), *A.Id));
			}
		}
		TestTrue(TEXT("every archetype maps to a job role"), bAllHaveRoles);
	}

	// --- Specific mappings ----------------------------------------------------
	{
		TestEqual(TEXT("doomsday barista -> barista"),
			FNpcOccupation::RoleFor(TEXT("doomsday_barista")), FName(TEXT("barista")));
		TestEqual(TEXT("method crossing guard -> crossing_guard"),
			FNpcOccupation::RoleFor(TEXT("method_crossing_guard")), FName(TEXT("crossing_guard")));
		TestTrue(TEXT("unknown id has no role"), FNpcOccupation::RoleFor(TEXT("nobody")).IsNone());
		TestTrue(TEXT("empty id has no role"), FNpcOccupation::RoleFor(TEXT("")).IsNone());
	}

	// --- Work actions are present and selection wraps -------------------------
	{
		const FName Role = TEXT("barista");
		const int32 Count = FNpcOccupation::WorkActionCount(Role);
		TestTrue(TEXT("barista has work actions"), Count > 0);
		TestFalse(TEXT("barista work action is not None"), FNpcOccupation::WorkAction(Role, 0).IsNone());
		TestEqual(TEXT("work action index wraps (posmod)"),
			FNpcOccupation::WorkAction(Role, Count), FNpcOccupation::WorkAction(Role, 0));
		TestFalse(TEXT("negative seed is safe"), FNpcOccupation::WorkAction(Role, -1).IsNone());
	}

	// --- Different roles do different work ------------------------------------
	{
		const FName Barista = FNpcOccupation::WorkAction(TEXT("barista"), 0);
		const FName Guard = FNpcOccupation::WorkAction(TEXT("crossing_guard"), 0);
		TestNotEqual(TEXT("barista and crossing guard do different work"), Barista, Guard);
	}

	// --- Unknown role falls back to generic on-shift business -----------------
	{
		TestTrue(TEXT("unknown role has a generic bank"), FNpcOccupation::WorkActionCount(NAME_None) > 0);
		TestFalse(TEXT("unknown role still yields an action"),
			FNpcOccupation::WorkAction(NAME_None, 3).IsNone());
	}

	// --- Determinism ----------------------------------------------------------
	{
		TestEqual(TEXT("same role+seed is the same action"),
			FNpcOccupation::WorkAction(TEXT("vendor"), 17), FNpcOccupation::WorkAction(TEXT("vendor"), 17));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
