// Copyright (c) 2026 GTC contributors

#include "ExplosionMath.h"

double FExplosionMath::RadialDamage(double Distance, double InnerRadius, double OuterRadius, double MaxDamage)
{
    // Degenerate/inverted radii describe no real blast volume → no damage, even at
    // the centre. Checked first so it wins over the full-damage inner case.
    if (OuterRadius <= InnerRadius)
    {
        return 0.0;
    }
    if (Distance <= InnerRadius)
    {
        return MaxDamage;
    }
    if (Distance >= OuterRadius)
    {
        return 0.0;
    }
    const double T = (Distance - InnerRadius) / (OuterRadius - InnerRadius);
    return MaxDamage * (1.0 - T);
}
