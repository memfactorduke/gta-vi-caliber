#pragma once
struct FMath {
    template<class T> static T Clamp(T v, T lo, T hi){ return v<lo?lo:(v>hi?hi:v); }
    template<class T> static T Max(T a, T b){ return a>b?a:b; }
    template<class T> static T Lerp(T a, T b, T t){ return (T)(a + (b - a) * t); }
    // UE SmoothStep: 0 below lo, 1 above hi, cubic Hermite (3t^2-2t^3) between.
    static float SmoothStep(float lo, float hi, float x){
        if (hi <= lo) return x < lo ? 0.f : 1.f;
        float t = (x - lo) / (hi - lo); if (t < 0) t = 0; if (t > 1) t = 1;
        return t * t * (3.f - 2.f * t);
    }
};
