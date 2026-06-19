// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, runtime-free wake & foam *parameter* generation for boats on the ocean.
 * No particles, no materials — just the scalars an emitter/material adapter needs:
 * the Kelvin wake half-angle (and how it narrows at speed), how strong the wake is,
 * the bow-foam spawn rate, whitecap foam from the surface's folding Jacobian, and
 * how a foam trail fades with age. Scene-free pure core (static methods, no
 * UObject, double precision), the FSkyModel / FOceanSurface pattern; the boat
 * pawn / Niagara adapter reads these per frame. Unit-tested headless in
 * Tests/OceanWakeTest.cpp (prefix GTC.World.Ocean.Wake).
 *
 * Background: a displacement hull's wake is bounded by the classic Kelvin angle
 * (~19.47 deg half-angle) at low Froude number; above a threshold the visible
 * wake narrows ~1/Fr (Rabaud & Moisy), which is why fast planing boats leave a
 * thin spear of a wake. Whitecaps appear where the Gerstner surface folds, i.e.
 * where FOceanSurface's Jacobian drops toward (and below) zero.
 */
struct GTC_UE5_API FOceanWake
{
	/** Classic deep-water Kelvin wake half-angle, degrees (arcsin(1/3)). */
	static constexpr double KelvinHalfAngleDeg = 19.471220634490695;

	/** Froude threshold below which the wake stays at the full Kelvin angle. */
	static constexpr double NarrowingFroude = 0.5;

	static constexpr double DefaultGravity = 9.81;

	/** Froude number Fr = Speed / sqrt(g * HullLength). 0 for a non-positive hull. */
	static double FroudeNumber(double Speed, double HullLength, double Gravity = DefaultGravity);

	/**
	 * Visible wake half-angle (degrees): the full Kelvin angle up to NarrowingFroude,
	 * then narrowing ~1/Fr above it (continuous at the threshold). Always in
	 * (0, KelvinHalfAngleDeg].
	 */
	static double WakeHalfAngleDeg(double Speed, double HullLength, double Gravity = DefaultGravity);

	/**
	 * Wake strength in [0,1]: a smooth ramp from rest to full as Speed approaches
	 * PlaningSpeed. 0 at a standstill (no wake), ~1 once planing.
	 */
	static double WakeStrength01(double Speed, double PlaningSpeed);

	/**
	 * Bow/stern foam spawn rate (arbitrary particles-per-second units): grows with
	 * both wake strength and speed, capped at MaxRate so a fast boat can't flood
	 * the emitter.
	 */
	static double BowFoamRate(double Speed, double PlaningSpeed, double MaxRate = 200.0);

	/**
	 * Whitecap foam coverage in [0,1] from the surface folding Jacobian: 0 where
	 * the sea is smooth (Jacobian >= FoamJacobian), ramping to 1 as it folds
	 * (Jacobian -> 0 and below). FoamJacobian is the onset threshold.
	 */
	static double WhitecapFoam01(double Jacobian, double FoamJacobian = 0.6);

	/** Foam-trail fade in [0,1]: 1 freshly laid, 0 once Age reaches Lifetime. */
	static double TrailFade01(double AgeSeconds, double LifetimeSeconds);
};
