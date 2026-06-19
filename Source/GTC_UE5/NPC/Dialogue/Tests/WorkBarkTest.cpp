// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WorkBark.h"
#include "../../Decision/NpcOccupation.h"

/**
 * Behavioural tests for FWorkBark: talky job actions have lines, selection wraps
 * deterministically, silent/unknown actions return "", and the talky tokens are
 * real FNpcOccupation work actions (the two stay in lockstep).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorkBarkTest,
	"GTC.NPC.Dialogue.WorkBark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorkBarkTest::RunTest(const FString& Parameters)
{
	// --- A talky action has lines, and selection wraps (posmod) ---------------
	{
		const FName Action = TEXT("call_order");
		const int32 Count = FWorkBark::Count(Action);
		TestTrue(TEXT("call_order has lines"), Count > 0);
		TestNotEqual(TEXT("call_order line is non-empty"), FWorkBark::Line(Action, 0), FString());
		TestEqual(TEXT("index wraps (posmod)"),
			FWorkBark::Line(Action, Count), FWorkBark::Line(Action, 0));
		TestNotEqual(TEXT("negative index safe"), FWorkBark::Line(Action, -1), FString());
	}

	// --- Silent / unknown actions say nothing ---------------------------------
	{
		TestEqual(TEXT("a silent physical action has no lines"), FWorkBark::Count(TEXT("wipe_counter")), 0);
		TestEqual(TEXT("silent action returns empty"), FWorkBark::Line(TEXT("wipe_counter"), 0), FString());
		TestEqual(TEXT("unknown token returns empty"), FWorkBark::Line(TEXT("nonsense_token"), 0), FString());
	}

	// --- The mime stays silent at work (no work-bark lines for any mime action)
	{
		const FName MimeActions[] = {
			TEXT("trapped_in_box"), TEXT("pull_invisible_rope"), TEXT("lean_on_nothing"),
			TEXT("walk_against_wind"), TEXT("silent_scream") };
		bool bAllSilent = true;
		for (const FName& A : MimeActions)
		{
			if (FWorkBark::Count(A) != 0) { bAllSilent = false; }
		}
		TestTrue(TEXT("every mime work action is silent"), bAllSilent);
	}

	// --- Every talky token is a real occupation work action -------------------
	// Guards against typos drifting the bark banks away from FNpcOccupation.
	{
		const FName Roles[] = {
			TEXT("barista"), TEXT("influencer"), TEXT("crossing_guard"), TEXT("vendor"),
			TEXT("yogi"), TEXT("stunt_double"), TEXT("mime"), TEXT("intern"),
			TEXT("dog_walker"), TEXT("food_critic"), TEXT("life_coach"), TEXT("weather_anchor") };

		// Collect every action token FNpcOccupation can emit.
		TSet<FName> ValidActions;
		for (const FName& Role : Roles)
		{
			const int32 N = FNpcOccupation::WorkActionCount(Role);
			for (int32 i = 0; i < N; ++i)
			{
				ValidActions.Add(FNpcOccupation::WorkAction(Role, i));
			}
		}

		// Probe the tokens we authored lines for and assert each is a real action.
		const FName Authored[] = {
			TEXT("call_order"), TEXT("foretell_doom"), TEXT("narrate_to_camera"), TEXT("check_views"),
			TEXT("blow_whistle"), TEXT("study_motivation"), TEXT("hawk_wares"), TEXT("scan_for_drones"),
			TEXT("enforce_serenity"), TEXT("correct_form"), TEXT("recall_glory_days"),
			TEXT("sprint_an_errand"), TEXT("vibrate_slightly"), TEXT("praise_good_boy"),
			TEXT("ponder_existence"), TEXT("demand_the_menu"), TEXT("recoil_in_disgust"),
			TEXT("affirm_loudly"), TEXT("hand_business_card"), TEXT("deliver_forecast") };
		bool bAllValid = true;
		for (const FName& A : Authored)
		{
			if (FWorkBark::Count(A) <= 0) { bAllValid = false; AddError(FString::Printf(TEXT("authored token '%s' has no lines"), *A.ToString())); }
			if (!ValidActions.Contains(A)) { bAllValid = false; AddError(FString::Printf(TEXT("'%s' is not an FNpcOccupation action"), *A.ToString())); }
		}
		TestTrue(TEXT("every authored work-bark token is a real occupation action"), bAllValid);
	}

	// --- Determinism ----------------------------------------------------------
	{
		TestEqual(TEXT("same action+index is the same line"),
			FWorkBark::Line(TEXT("hawk_wares"), 5), FWorkBark::Line(TEXT("hawk_wares"), 5));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
