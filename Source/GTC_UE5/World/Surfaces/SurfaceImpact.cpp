// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurfaceImpact.h"

FName GTCSurfaceTags::SurfaceTag(EGTCSurface Surface)
{
	switch (Surface)
	{
	case EGTCSurface::Wood:       return FName(TEXT("Surface.Wood"));
	case EGTCSurface::Metal:      return FName(TEXT("Surface.Metal"));
	case EGTCSurface::Glass:      return FName(TEXT("Surface.Glass"));
	case EGTCSurface::Concrete:   return FName(TEXT("Surface.Concrete"));
	case EGTCSurface::Creature:   return FName(TEXT("Creature"));
	case EGTCSurface::Asphalt:    return FName(TEXT("Surface.Asphalt"));
	case EGTCSurface::Brick:      return FName(TEXT("Surface.Brick"));
	case EGTCSurface::Ceramic:    return FName(TEXT("Surface.Ceramic"));
	case EGTCSurface::Rubber:     return FName(TEXT("Surface.Rubber"));
	case EGTCSurface::Vegetation: return FName(TEXT("Surface.Vegetation"));
	case EGTCSurface::Ice:        return FName(TEXT("Surface.Ice"));
	case EGTCSurface::Leather:    return FName(TEXT("Surface.Leather"));
	case EGTCSurface::Paper:      return FName(TEXT("Surface.Paper"));
	case EGTCSurface::Water:      return FName(TEXT("Surface.Water"));
	default:                      return NAME_None;
	}
}

FName GTCSurfaceTags::CreatureTag()
{
	return SurfaceTag(EGTCSurface::Creature);
}

EGTCSurface FGTCSurfaceImpact::PhysicalSurfaceToSurface(uint8 PhysicalSurfaceIndex)
{
	// Indices match the SurfaceTypeN slots authored in Config/DefaultEngine.ini. Kept as an
	// explicit switch (not a cast) so reordering EGTCSurface can't silently misclassify.
	switch (PhysicalSurfaceIndex)
	{
	case 1:  return EGTCSurface::Wood;
	case 2:  return EGTCSurface::Metal;
	case 3:  return EGTCSurface::Glass;
	case 4:  return EGTCSurface::Concrete;
	case 5:  return EGTCSurface::Creature;
	case 6:  return EGTCSurface::Asphalt;
	case 7:  return EGTCSurface::Brick;
	case 8:  return EGTCSurface::Ceramic;
	case 9:  return EGTCSurface::Rubber;
	case 10: return EGTCSurface::Vegetation;
	case 11: return EGTCSurface::Ice;
	case 12: return EGTCSurface::Leather;
	case 13: return EGTCSurface::Paper;
	case 14: return EGTCSurface::Water;
	default: return EGTCSurface::Default; // 0 == SurfaceType_Default, plus any unmapped slot.
	}
}

EGTCSurface FGTCSurfaceImpact::SurfaceFromTag(FName Tag)
{
	if (Tag.IsNone())
	{
		return EGTCSurface::Default;
	}
	for (uint8 i = 1; i < static_cast<uint8>(EGTCSurface::Count); ++i)
	{
		const EGTCSurface Surface = static_cast<EGTCSurface>(i);
		if (Tag == GTCSurfaceTags::SurfaceTag(Surface))
		{
			return Surface;
		}
	}
	return EGTCSurface::Default;
}

FGTCImpactEffect FGTCSurfaceImpact::ImpactEffectFor(EGTCSurface Surface)
{
	// Shared bullet-mark decals stamped on hard surfaces. Authored by
	// Scripts/gtc_surfaces/apply_bullet_decals.py (deferred-decal instances off the project's
	// MM_Decal master). Until those assets exist the adapter's LoadDecalCached returns null and
	// simply skips the decal — wiring the paths early is a harmless no-op.
	static const TCHAR* const HoleDecal =
		TEXT("/Game/Surfaces/Decals/MI_GTC_BulletHole.MI_GTC_BulletHole");      // generic dark pit
	static const TCHAR* const ScorchDecal =
		TEXT("/Game/Surfaces/Decals/MI_GTC_BulletScorch.MI_GTC_BulletScorch");  // metal scorch ring
	static const TCHAR* const CrackDecal =
		TEXT("/Game/Surfaces/Decals/MI_GTC_GlassCrack.MI_GTC_GlassCrack");      // radial glass crack

	FGTCImpactEffect E;
	switch (Surface)
	{
	case EGTCSurface::Wood:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Wood.P_Wood");
		E.DebugColor = FColor(150, 95, 40); // splinter brown
		E.DecalSizeCm = 8.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Metal:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Sparks_A.P_Sparks_A");
		E.DebugColor = FColor(255, 220, 90); // spark yellow
		E.DecalSizeCm = 6.0;
		E.DecalPath = ScorchDecal;
		break;
	case EGTCSurface::Glass:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Destruction/P_Destruction_Glass.P_Destruction_Glass");
		E.DebugColor = FColor(140, 220, 235); // shard cyan
		E.Scale = 0.6; // a bullet hole, not a full pane shattering
		E.DecalSizeCm = 7.0;
		E.DecalPath = CrackDecal; // the round leaves a radial crack in the pane
		break;
	case EGTCSurface::Concrete:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Concrete.P_Concrete");
		E.DebugColor = FColor(170, 170, 165); // dust grey
		E.DecalSizeCm = 10.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Creature:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Blood/P_Blood_Splat_Cone.P_Blood_Splat_Cone");
		E.DebugColor = FColor(160, 20, 20); // blood red — no decal on a moving body
		break;
	case EGTCSurface::Asphalt:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Asphalt.P_Asphalt");
		E.DebugColor = FColor(60, 60, 65); // road grit
		E.DecalSizeCm = 10.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Brick:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Brick.P_Brick");
		E.DebugColor = FColor(170, 85, 60); // terracotta dust
		E.DecalSizeCm = 9.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Ceramic:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Ceramic.P_Ceramic");
		E.DebugColor = FColor(230, 225, 215); // white shards
		E.DecalSizeCm = 7.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Rubber:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Rubber.P_Rubber");
		E.DebugColor = FColor(40, 40, 42); // dull scuff — no decal, rubber doesn't pit
		break;
	case EGTCSurface::Vegetation:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Vegetation.P_Vegetation");
		E.DebugColor = FColor(70, 140, 55); // leaf green — no decal on foliage
		break;
	case EGTCSurface::Ice:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Ice.P_Ice");
		E.DebugColor = FColor(200, 230, 245); // pale crystalline
		E.DecalSizeCm = 6.0;
		E.DecalPath = CrackDecal; // ice fractures like glass
		break;
	case EGTCSurface::Leather:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Leather.P_Leather");
		E.DebugColor = FColor(110, 70, 45); // soft brown puff
		E.DecalSizeCm = 5.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Paper:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Paper.P_Paper");
		E.DebugColor = FColor(235, 230, 210); // cream shred
		E.DecalSizeCm = 5.0;
		E.DecalPath = HoleDecal;
		break;
	case EGTCSurface::Water:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Water/P_Water_Dripping_A.P_Water_Dripping_A");
		E.DebugColor = FColor(60, 130, 200); // splash blue — no decal on water
		E.Scale = 1.2;
		break;
	case EGTCSurface::Default:
	default:
		E.ParticlePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Default.P_Default");
		E.DebugColor = FColor::White;
		E.DecalSizeCm = 6.0;
		E.DecalPath = HoleDecal;
		break;
	}
	return E;
}

const TCHAR* FGTCSurfaceImpact::SurfaceName(EGTCSurface Surface)
{
	switch (Surface)
	{
	case EGTCSurface::Wood:       return TEXT("Wood");
	case EGTCSurface::Metal:      return TEXT("Metal");
	case EGTCSurface::Glass:      return TEXT("Glass");
	case EGTCSurface::Concrete:   return TEXT("Concrete");
	case EGTCSurface::Creature:   return TEXT("Creature");
	case EGTCSurface::Asphalt:    return TEXT("Asphalt");
	case EGTCSurface::Brick:      return TEXT("Brick");
	case EGTCSurface::Ceramic:    return TEXT("Ceramic");
	case EGTCSurface::Rubber:     return TEXT("Rubber");
	case EGTCSurface::Vegetation: return TEXT("Vegetation");
	case EGTCSurface::Ice:        return TEXT("Ice");
	case EGTCSurface::Leather:    return TEXT("Leather");
	case EGTCSurface::Paper:      return TEXT("Paper");
	case EGTCSurface::Water:      return TEXT("Water");
	case EGTCSurface::Default:    return TEXT("Default");
	default:                      return TEXT("Default");
	}
}
