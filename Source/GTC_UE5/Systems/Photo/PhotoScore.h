// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Photo-mode composition scoring — the phone camera's eye for a good shot. The
 * setting runs on phones and feeds (the citizens livestream, the player will post),
 * but nothing judges whether a framed photo is actually any GOOD. FPhotoScore rates
 * a composed shot 0..1 from the things that make a photo work — where the subject
 * sits in the frame, how much of it the subject fills, whether the horizon is level,
 * how much of interest is in shot, and the light. That 0..1 is, by design, exactly
 * the Appeal a social post is scored on, so the phone camera drops straight into the
 * clout economy: snap a well-composed shot, post it, gain followers (the adapter is
 * just clout.Post(FPhotoScore::Appeal(shot)) — kept decoupled here, wired later).
 *
 * The sub-scores are deterministic and each defensible on its own:
 *   - FRAMING rewards the rule of thirds: the closer the subject sits to a thirds
 *     intersection the better, normalized so a corner reads 0 and a thirds point 1.
 *     Dead-centre lands at exactly 0.5 — centred is fine, off-centre is better.
 *   - FILL peaks when the subject fills a sweet-spot fraction of the frame and falls
 *     off when it's a distant speck or cropped too tight.
 *   - LEVEL falls with horizon tilt to zero past MaxTilt — a dutch-angle snapshot.
 *   - INTEREST rewards more notable things in frame, up to a cap.
 *   - LIGHTING is the caller's light-quality read (golden hour ~1) passed through.
 * Appeal is their weighted blend, normalized by the weights so it's always 0..1.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject,
 * all static — unit-tested headless (Tests/PhotoScoreTest.cpp, prefix
 * GTC.Systems.Photo).
 *
 * PURE-CORE boundary: projecting the world subject to a screen position, measuring
 * its fill from the bounds, reading the camera roll and the time-of-day light, and
 * handing Appeal to the clout/post system is the DEFERRED adapter and is NOT covered
 * by the tests. Units: Subject is normalized screen space [0,1]x[0,1] with (0.5,0.5)
 * the centre; Fill/Lighting/all scores are 0..1; TiltDegrees is degrees of roll.
 */
struct GTC_UE5_API FPhotoScore
{
    /** One composed shot, as the camera adapter measures it. */
    struct FShot
    {
        /** Subject's normalized screen position; (0.5,0.5) is centre. Clamped to [0,1]^2. */
        FVector2D Subject = FVector2D(0.5, 0.5);
        /** Fraction of the frame the subject fills, 0..1. */
        double SubjectFill = 0.0;
        /** Horizon roll magnitude in degrees (0 = level). */
        double TiltDegrees = 0.0;
        /** Count of notable things in frame (the subject, landmarks, other actors). */
        int32 PointsOfInterest = 0;
        /** Caller's light-quality read, 0..1 (golden hour ~1). Clamped. */
        double Lighting = 0.5;
    };

    /** Scoring tuning. Defaults give a forgiving phone-camera feel; weights sum to 1. */
    struct FParams
    {
        /** Subject fill that scores best. */
        double IdealFill = 0.35;
        /** How far from IdealFill the fill score reaches zero. */
        double FillTolerance = 0.30;
        /** Tilt (degrees) at/over which the level score is zero. */
        double MaxTilt = 30.0;
        /** Points of interest at/over which interest is maxed. */
        int32 InterestCap = 4;

        double FramingWeight = 0.30;
        double FillWeight = 0.25;
        double LevelWeight = 0.20;
        double InterestWeight = 0.15;
        double LightingWeight = 0.10;
    };

    /** Rule-of-thirds framing score 0..1 for a subject position (clamped to [0,1]^2). */
    static double FramingScore(const FVector2D& Subject);

    /** Subject-fill score 0..1: peaks at IdealFill, falls to 0 past FillTolerance. */
    static double FillScore(double SubjectFill, const FParams& Params);

    /** Level score 0..1: 1 when level, 0 at/over MaxTilt. */
    static double LevelScore(double TiltDegrees, const FParams& Params);

    /** Interest score 0..1: PointsOfInterest / InterestCap, clamped. */
    static double InterestScore(int32 PointsOfInterest, const FParams& Params);

    /**
     * The shot's overall Appeal 0..1 — the weighted blend of the sub-scores, the same
     * 0..1 a social post is scored on. Always finite and in range.
     */
    static double Appeal(const FShot& Shot, const FParams& Params);
};
