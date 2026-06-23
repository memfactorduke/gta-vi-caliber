// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SurfaceImpact.h"

/**
 * Unit tests for the surface impact taxonomy — physical-surface and tag classification, the
 * effect registry, and the tag round-trip. Prefix GTC.World.Surfaces.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceImpactTest,
	"GTC.World.Surfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceImpactTest::RunTest(const FString& Parameters)
{
	using FS = FGTCSurfaceImpact;

	// --- Physical-surface index -> category (mirrors DefaultEngine.ini SurfaceTypeN) ---
	TestEqual(TEXT("index 0 is Default"), FS::PhysicalSurfaceToSurface(0), EGTCSurface::Default);
	TestEqual(TEXT("SurfaceType1 is Wood"), FS::PhysicalSurfaceToSurface(1), EGTCSurface::Wood);
	TestEqual(TEXT("SurfaceType2 is Metal"), FS::PhysicalSurfaceToSurface(2), EGTCSurface::Metal);
	TestEqual(TEXT("SurfaceType3 is Glass"), FS::PhysicalSurfaceToSurface(3), EGTCSurface::Glass);
	TestEqual(TEXT("SurfaceType4 is Concrete"), FS::PhysicalSurfaceToSurface(4), EGTCSurface::Concrete);
	TestEqual(TEXT("SurfaceType5 is Creature"), FS::PhysicalSurfaceToSurface(5), EGTCSurface::Creature);
	TestEqual(TEXT("SurfaceType6 is Asphalt"), FS::PhysicalSurfaceToSurface(6), EGTCSurface::Asphalt);
	TestEqual(TEXT("SurfaceType7 is Brick"), FS::PhysicalSurfaceToSurface(7), EGTCSurface::Brick);
	TestEqual(TEXT("SurfaceType8 is Ceramic"), FS::PhysicalSurfaceToSurface(8), EGTCSurface::Ceramic);
	TestEqual(TEXT("SurfaceType9 is Rubber"), FS::PhysicalSurfaceToSurface(9), EGTCSurface::Rubber);
	TestEqual(TEXT("SurfaceType10 is Vegetation"), FS::PhysicalSurfaceToSurface(10), EGTCSurface::Vegetation);
	TestEqual(TEXT("SurfaceType11 is Ice"), FS::PhysicalSurfaceToSurface(11), EGTCSurface::Ice);
	TestEqual(TEXT("SurfaceType12 is Leather"), FS::PhysicalSurfaceToSurface(12), EGTCSurface::Leather);
	TestEqual(TEXT("SurfaceType13 is Paper"), FS::PhysicalSurfaceToSurface(13), EGTCSurface::Paper);
	TestEqual(TEXT("SurfaceType14 is Water"), FS::PhysicalSurfaceToSurface(14), EGTCSurface::Water);
	TestEqual(TEXT("the enum value equals its SurfaceTypeN index"),
		static_cast<uint8>(EGTCSurface::Water), static_cast<uint8>(14));
	TestEqual(TEXT("an unmapped slot falls back to Default"),
		FS::PhysicalSurfaceToSurface(42), EGTCSurface::Default);

	// --- Tag classification ---
	TestEqual(TEXT("the Creature tag classifies as Creature"),
		FS::SurfaceFromTag(GTCSurfaceTags::CreatureTag()), EGTCSurface::Creature);
	TestEqual(TEXT("Surface.Wood classifies as Wood"),
		FS::SurfaceFromTag(FName(TEXT("Surface.Wood"))), EGTCSurface::Wood);
	TestEqual(TEXT("Surface.Glass classifies as Glass"),
		FS::SurfaceFromTag(FName(TEXT("Surface.Glass"))), EGTCSurface::Glass);
	TestEqual(TEXT("an unknown tag is Default"),
		FS::SurfaceFromTag(FName(TEXT("Surface.Banana"))), EGTCSurface::Default);
	TestEqual(TEXT("None is Default"), FS::SurfaceFromTag(NAME_None), EGTCSurface::Default);

	// --- Tag round-trip: every real surface has a tag that classifies back to itself ---
	for (uint8 i = 1; i < static_cast<uint8>(EGTCSurface::Count); ++i)
	{
		const EGTCSurface Surface = static_cast<EGTCSurface>(i);
		const FName Tag = GTCSurfaceTags::SurfaceTag(Surface);
		TestFalse(TEXT("a real surface has a non-None tag"), Tag.IsNone());
		TestEqual(TEXT("a surface tag round-trips to its surface"),
			FS::SurfaceFromTag(Tag), Surface);
	}
	TestTrue(TEXT("Default has no tag"), GTCSurfaceTags::SurfaceTag(EGTCSurface::Default).IsNone());

	// --- Effect registry: every surface yields a usable particle path ---
	for (uint8 i = 0; i < static_cast<uint8>(EGTCSurface::Count); ++i)
	{
		const EGTCSurface Surface = static_cast<EGTCSurface>(i);
		const FGTCImpactEffect E = FS::ImpactEffectFor(Surface);
		TestNotNull(TEXT("a surface has a particle path"), E.ParticlePath);
		TestTrue(TEXT("the particle path points at the VFX pack"),
			FString(E.ParticlePath).StartsWith(TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/")));
		TestTrue(TEXT("scale is positive"), E.Scale > 0.0);
	}

	// --- Spot-check the signature effects line up with the right Cascade systems ---
	TestTrue(TEXT("metal sparks"),
		FString(FS::ImpactEffectFor(EGTCSurface::Metal).ParticlePath).Contains(TEXT("Sparks")));
	TestTrue(TEXT("glass shatters"),
		FString(FS::ImpactEffectFor(EGTCSurface::Glass).ParticlePath).Contains(TEXT("Glass")));
	TestTrue(TEXT("creature bleeds"),
		FString(FS::ImpactEffectFor(EGTCSurface::Creature).ParticlePath).Contains(TEXT("Blood")));
	TestEqual(TEXT("blood leaves no surface decal"),
		FS::ImpactEffectFor(EGTCSurface::Creature).DecalSizeCm, 0.0);
	TestTrue(TEXT("asphalt uses its own burst"),
		FString(FS::ImpactEffectFor(EGTCSurface::Asphalt).ParticlePath).Contains(TEXT("Asphalt")));
	TestTrue(TEXT("vegetation uses its own burst"),
		FString(FS::ImpactEffectFor(EGTCSurface::Vegetation).ParticlePath).Contains(TEXT("Vegetation")));
	TestTrue(TEXT("water splashes"),
		FString(FS::ImpactEffectFor(EGTCSurface::Water).ParticlePath).Contains(TEXT("Water")));
	TestEqual(TEXT("soft/loose surfaces leave no decal: vegetation"),
		FS::ImpactEffectFor(EGTCSurface::Vegetation).DecalSizeCm, 0.0);
	TestEqual(TEXT("soft/loose surfaces leave no decal: water"),
		FS::ImpactEffectFor(EGTCSurface::Water).DecalSizeCm, 0.0);
	TestEqual(TEXT("soft/loose surfaces leave no decal: rubber"),
		FS::ImpactEffectFor(EGTCSurface::Rubber).DecalSizeCm, 0.0);

	// --- Decal wiring: a surface with a decal size carries a decal material, and vice-versa ---
	for (uint8 i = 0; i < static_cast<uint8>(EGTCSurface::Count); ++i)
	{
		const EGTCSurface Surface = static_cast<EGTCSurface>(i);
		const FGTCImpactEffect E = FS::ImpactEffectFor(Surface);
		const bool bHasSize = E.DecalSizeCm > 0.0;
		const bool bHasPath = E.DecalPath != nullptr;
		TestEqual(TEXT("decal size and decal material agree for every surface"), bHasSize, bHasPath);
		if (bHasPath)
		{
			TestTrue(TEXT("a decal material points at the GTC decal folder"),
				FString(E.DecalPath).StartsWith(TEXT("/Game/Surfaces/Decals/")));
		}
	}
	TestNotNull(TEXT("concrete leaves a bullet hole"), FS::ImpactEffectFor(EGTCSurface::Concrete).DecalPath);
	TestNull(TEXT("a creature leaves no decal material"), FS::ImpactEffectFor(EGTCSurface::Creature).DecalPath);
	TestNull(TEXT("water leaves no decal material"), FS::ImpactEffectFor(EGTCSurface::Water).DecalPath);

	// --- Surface names ---
	TestEqual(TEXT("name of Concrete"),
		FString(FS::SurfaceName(EGTCSurface::Concrete)), FString(TEXT("Concrete")));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
