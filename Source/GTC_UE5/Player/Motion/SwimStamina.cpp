// Copyright Epic Games, Inc. All Rights Reserved.

#include "SwimStamina.h"

#include "Math/UnrealMathUtility.h"

FSwimStamina::FSwimStamina(double MaximumOxygen, double MaximumStamina)
{
    MaxOxygen = FMath::Max(MaximumOxygen, 0.0001);
    OxygenValue = MaxOxygen;
    MaxStamina = FMath::Max(MaximumStamina, 0.0001);
    StaminaValue = MaxStamina;
}

void FSwimStamina::Update(bool bIsUnderwater, bool bIsSprinting, double Depth, double Delta)
{
    const double Step = FMath::Max(Delta, 0.0);
    if (bIsUnderwater)
    {
        OxygenValue -= OxygenDrainPerSecond * PressureAt(Depth) * Step;
    }
    else
    {
        OxygenValue += OxygenRefillPerSecond * Step;
    }
    OxygenValue = FMath::Clamp(OxygenValue, 0.0, MaxOxygen);

    const bool bSwimming = bIsUnderwater || Depth > 0.0;
    if (bSwimming)
    {
        double Drain = StaminaDrainPerSecond;
        // Sprint only costs extra while there is stamina to spend it on.
        if (bIsSprinting && StaminaValue > 0.0)
        {
            Drain += SprintStaminaExtra;
        }
        StaminaValue -= Drain * Step;
    }
    else
    {
        StaminaValue += StaminaRecoverPerSecond * Step;
    }
    StaminaValue = FMath::Clamp(StaminaValue, 0.0, MaxStamina);
}

double FSwimStamina::Oxygen() const
{
    return OxygenValue;
}

double FSwimStamina::OxygenFraction() const
{
    return OxygenValue / MaxOxygen;
}

double FSwimStamina::Stamina() const
{
    return StaminaValue;
}

double FSwimStamina::StaminaFraction() const
{
    return StaminaValue / MaxStamina;
}

bool FSwimStamina::IsDrowning(bool bIsUnderwater) const
{
    return bIsUnderwater && OxygenValue <= 0.0;
}

bool FSwimStamina::IsExhausted() const
{
    return StaminaValue <= 0.0;
}

double FSwimStamina::DrownDamage(bool bIsUnderwater, double Delta, double Dps) const
{
    if (!IsDrowning(bIsUnderwater))
    {
        return 0.0;
    }
    return FMath::Max(Dps, 0.0) * FMath::Max(Delta, 0.0);
}

double FSwimStamina::SwimSpeed(double BaseSpeed, bool bIsSprinting) const
{
    if (IsExhausted())
    {
        return BaseSpeed * ExhaustedSpeedFactor;
    }
    if (bIsSprinting)
    {
        return BaseSpeed * SprintSpeedFactor;
    }
    return BaseSpeed;
}

double FSwimStamina::PressureAt(double Depth) const
{
    return 1.0 + FMath::Max(Depth, 0.0) / DepthPerAtmosphere;
}

void FSwimStamina::Surface()
{
    OxygenValue = MaxOxygen;
}

void FSwimStamina::Reset()
{
    OxygenValue = MaxOxygen;
    StaminaValue = MaxStamina;
}
