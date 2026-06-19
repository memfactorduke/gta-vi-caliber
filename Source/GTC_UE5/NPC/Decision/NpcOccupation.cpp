// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcOccupation.h"

namespace
{
	// Positive modulo so a negative or large seed still indexes a real action
	// (mirrors Godot posmod, as used across FBarkPool / FNpcIdleAction).
	int32 PosMod(int32 Value, int32 Modulus)
	{
		if (Modulus <= 0)
		{
			return 0;
		}
		return ((Value % Modulus) + Modulus) % Modulus;
	}

	// Generic "on the clock" business for a citizen whose job has no specific bank.
	const TArray<FName>& GenericWorkBank()
	{
		static const TArray<FName> Bank = {
			TEXT("tidy_up"), TEXT("check_phone"), TEXT("glance_around"),
			TEXT("shift_weight"), TEXT("mutter_to_self")
		};
		return Bank;
	}

	// Role -> its work-action bank. Each entry is a token the anim layer plays;
	// they read as that specific job being done. GTC-original, free to grow.
	const TArray<FName>* BankForRole(FName Role)
	{
		static const TArray<FName> Barista = {
			TEXT("pull_espresso"), TEXT("steam_milk"), TEXT("call_order"),
			TEXT("wipe_counter"), TEXT("foretell_doom") };
		static const TArray<FName> Influencer = {
			TEXT("frame_shot"), TEXT("film_selfie"), TEXT("check_views"),
			TEXT("coax_pigeon"), TEXT("narrate_to_camera") };
		static const TArray<FName> CrossingGuard = {
			TEXT("raise_stop_sign"), TEXT("wave_cross"), TEXT("blow_whistle"),
			TEXT("salute_driver"), TEXT("study_motivation") };
		static const TArray<FName> Vendor = {
			TEXT("grill_dog"), TEXT("hawk_wares"), TEXT("make_change"),
			TEXT("scan_for_drones"), TEXT("restock_cart") };
		static const TArray<FName> Yogi = {
			TEXT("demo_pose"), TEXT("correct_form"), TEXT("deep_breath"),
			TEXT("chime_bell"), TEXT("enforce_serenity") };
		static const TArray<FName> StuntDouble = {
			TEXT("stretch"), TEXT("mime_a_roll"), TEXT("recall_glory_days"),
			TEXT("test_the_railing"), TEXT("wince_at_knee") };
		static const TArray<FName> Mime = {
			TEXT("trapped_in_box"), TEXT("pull_invisible_rope"), TEXT("lean_on_nothing"),
			TEXT("walk_against_wind"), TEXT("silent_scream") };
		static const TArray<FName> Intern = {
			TEXT("sip_cold_brew"), TEXT("sprint_an_errand"), TEXT("take_frantic_notes"),
			TEXT("refresh_inbox"), TEXT("vibrate_slightly") };
		static const TArray<FName> DogWalker = {
			TEXT("untangle_leashes"), TEXT("scoop_after_dog"), TEXT("ponder_existence"),
			TEXT("praise_good_boy"), TEXT("pause_to_reflect") };
		static const TArray<FName> FoodCritic = {
			TEXT("sniff_the_dish"), TEXT("jot_a_rating"), TEXT("judge_silently"),
			TEXT("demand_the_menu"), TEXT("recoil_in_disgust") };
		static const TArray<FName> LifeCoach = {
			TEXT("point_at_you"), TEXT("affirm_loudly"), TEXT("hand_business_card"),
			TEXT("visualize_success"), TEXT("high_five_the_air") };
		static const TArray<FName> WeatherAnchor = {
			TEXT("gesture_at_sky"), TEXT("deliver_forecast"), TEXT("point_to_map"),
			TEXT("sign_off"), TEXT("lick_finger_for_wind") };

		// Wave 2 trades.
		static const TArray<FName> Lifeguard = {
			TEXT("scan_water"), TEXT("twirl_whistle"), TEXT("reapply_zinc"),
			TEXT("wave_swimmers_in"), TEXT("pose_heroically") };
		static const TArray<FName> Electrician = {
			TEXT("splice_wire"), TEXT("test_neon"), TEXT("curse_at_flicker"),
			TEXT("check_breaker"), TEXT("admire_the_glow") };
		static const TArray<FName> Courier = {
			TEXT("check_app"), TEXT("rev_scooter"), TEXT("balance_the_bags"),
			TEXT("sprint_upstairs"), TEXT("chug_water") };
		static const TArray<FName> Caricaturist = {
			TEXT("sketch_fast"), TEXT("exaggerate_the_nose"), TEXT("hawk_a_portrait"),
			TEXT("sharpen_pencil"), TEXT("size_up_a_face") };
		static const TArray<FName> TourGuide = {
			TEXT("point_at_facade"), TEXT("recite_history"), TEXT("count_heads"),
			TEXT("raise_the_flag"), TEXT("dodge_a_question") };
		static const TArray<FName> Fishmonger = {
			TEXT("ice_the_catch"), TEXT("shout_specials"), TEXT("gut_a_fish"),
			TEXT("shoo_the_pelican"), TEXT("weigh_an_order") };
		static const TArray<FName> Paramedic = {
			TEXT("check_pulse"), TEXT("unpack_kit"), TEXT("radio_dispatch"),
			TEXT("reassure_patient"), TEXT("roll_the_gurney") };
		static const TArray<FName> Bartender = {
			TEXT("shake_cocktail"), TEXT("slide_a_drink"), TEXT("cue_karaoke"),
			TEXT("wipe_the_bar"), TEXT("cut_off_a_regular") };

		if (Role == FName(TEXT("barista")))        return &Barista;
		if (Role == FName(TEXT("influencer")))     return &Influencer;
		if (Role == FName(TEXT("crossing_guard"))) return &CrossingGuard;
		if (Role == FName(TEXT("vendor")))         return &Vendor;
		if (Role == FName(TEXT("yogi")))           return &Yogi;
		if (Role == FName(TEXT("stunt_double")))   return &StuntDouble;
		if (Role == FName(TEXT("mime")))           return &Mime;
		if (Role == FName(TEXT("intern")))         return &Intern;
		if (Role == FName(TEXT("dog_walker")))     return &DogWalker;
		if (Role == FName(TEXT("food_critic")))    return &FoodCritic;
		if (Role == FName(TEXT("life_coach")))     return &LifeCoach;
		if (Role == FName(TEXT("weather_anchor"))) return &WeatherAnchor;
		if (Role == FName(TEXT("lifeguard")))      return &Lifeguard;
		if (Role == FName(TEXT("electrician")))    return &Electrician;
		if (Role == FName(TEXT("courier")))        return &Courier;
		if (Role == FName(TEXT("caricaturist")))   return &Caricaturist;
		if (Role == FName(TEXT("tour_guide")))     return &TourGuide;
		if (Role == FName(TEXT("fishmonger")))     return &Fishmonger;
		if (Role == FName(TEXT("paramedic")))      return &Paramedic;
		if (Role == FName(TEXT("bartender")))      return &Bartender;
		return nullptr;
	}
}

FName FNpcOccupation::RoleFor(const FString& ArchetypeId)
{
	// Archetype ids are unique + descriptive (FNpcArchetypes); map each to its job.
	if (ArchetypeId == TEXT("doomsday_barista"))            return TEXT("barista");
	if (ArchetypeId == TEXT("pigeon_influencer"))           return TEXT("influencer");
	if (ArchetypeId == TEXT("method_crossing_guard"))       return TEXT("crossing_guard");
	if (ArchetypeId == TEXT("conspiracy_vendor"))           return TEXT("vendor");
	if (ArchetypeId == TEXT("aggressively_calm_yogi"))      return TEXT("yogi");
	if (ArchetypeId == TEXT("retired_stunt_double"))        return TEXT("stunt_double");
	if (ArchetypeId == TEXT("off_duty_mime"))               return TEXT("mime");
	if (ArchetypeId == TEXT("overcaffeinated_intern"))      return TEXT("intern");
	if (ArchetypeId == TEXT("philosophical_dog_walker"))    return TEXT("dog_walker");
	if (ArchetypeId == TEXT("feral_food_critic"))           return TEXT("food_critic");
	if (ArchetypeId == TEXT("unlicensed_life_coach"))       return TEXT("life_coach");
	if (ArchetypeId == TEXT("weather_anchor_nobody_hired")) return TEXT("weather_anchor");
	// Wave 2 trades.
	if (ArchetypeId == TEXT("tide_pool_lifeguard"))         return TEXT("lifeguard");
	if (ArchetypeId == TEXT("neon_sign_electrician"))       return TEXT("electrician");
	if (ArchetypeId == TEXT("midnight_food_courier"))       return TEXT("courier");
	if (ArchetypeId == TEXT("boardwalk_caricaturist"))      return TEXT("caricaturist");
	if (ArchetypeId == TEXT("art_deco_tour_guide"))         return TEXT("tour_guide");
	if (ArchetypeId == TEXT("pelican_fishmonger"))          return TEXT("fishmonger");
	if (ArchetypeId == TEXT("rollerblade_paramedic"))       return TEXT("paramedic");
	if (ArchetypeId == TEXT("karaoke_bartender"))           return TEXT("bartender");
	return NAME_None;
}

int32 FNpcOccupation::WorkActionCount(FName Role)
{
	const TArray<FName>* Bank = BankForRole(Role);
	return Bank ? Bank->Num() : GenericWorkBank().Num();
}

FName FNpcOccupation::WorkAction(FName Role, int32 Seed)
{
	const TArray<FName>* Bank = BankForRole(Role);
	const TArray<FName>& Use = Bank ? *Bank : GenericWorkBank();
	if (Use.Num() == 0)
	{
		return NAME_None;
	}
	return Use[PosMod(Seed, Use.Num())];
}
