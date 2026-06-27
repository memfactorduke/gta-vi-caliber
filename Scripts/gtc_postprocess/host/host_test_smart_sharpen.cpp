// Host-side unit test for FSmartSharpen (the pure-core sharpening math the
// M_GTC_SmartSharpen material and the UGTCVideoSettings "Sharpness" slider both
// rely on). Compiles the real Source/.../SmartSharpen.cpp against tiny UE stubs
// so the policy can be verified with no engine/editor. Mirrors the assertions in
// Camera/Tests/SmartSharpenTest.cpp.  Build & run:  ./run.sh
#include "SmartSharpen.h"
const FVector2D FVector2D::ZeroVector(0.0, 0.0);
#include "../../../Source/GTC_UE5/Camera/SmartSharpen.cpp"
#include <cstdio>
#include <cmath>

static int g_fail = 0;
static void chk(const char* name, double got, double want, double eps = 1e-4) {
    if (std::fabs(got - want) > eps) { std::printf("FAIL  %-34s got %g  want %g\n", name, got, want); ++g_fail; }
    else std::printf("ok    %-34s = %g\n", name, got);
}
static void chktrue(const char* name, bool c) {
    if (!c) { std::printf("FAIL  %-34s\n", name); ++g_fail; } else std::printf("ok    %-34s\n", name); }

int main() {
    using S = FSmartSharpen;
    chk("luma black", S::LumaRec709(FLinearColor(0,0,0)), 0.0);
    chk("luma white", S::LumaRec709(FLinearColor(1,1,1)), 1.0);
    chk("luma green", S::LumaRec709(FLinearColor(0,1,0)), 0.7152);
    chk("luma red",   S::LumaRec709(FLinearColor(1,0,0)), 0.2126);
    chk("luma blue",  S::LumaRec709(FLinearColor(0,0,1)), 0.0722);
    chk("clamp neg",  S::ClampAmount(-2.0f), 0.0);
    chk("clamp mid",  S::ClampAmount(1.5f), 1.5);
    chk("clamp max",  S::ClampAmount(99.0f), S::MaxAmount);
    chk("dist under near", S::DistanceSharpen(1,0.2f,100,500,5000), 1.0);
    chk("dist near edge",  S::DistanceSharpen(1,0.2f,500,500,5000), 1.0);
    chk("dist over far",   S::DistanceSharpen(1,0.2f,9000,500,5000), 0.2);
    chk("dist midpoint",   S::DistanceSharpen(1,0.2f,2750,500,5000), 0.6);
    chk("dist degenerate", S::DistanceSharpen(1,0.2f,9000,5000,5000), 1.0);
    chk("shadow black",  S::ShadowSafeWeight(0.0f,0.1f), 0.0);
    chk("shadow floor",  S::ShadowSafeWeight(0.1f,0.1f), 1.0);
    chk("shadow above",  S::ShadowSafeWeight(0.8f,0.1f), 1.0);
    chk("shadow half",   S::ShadowSafeWeight(0.05f,0.1f), 0.5);
    chk("shadow nofloor",S::ShadowSafeWeight(0.0f,0.0f), 1.0);
    chk("foliage nomask",  S::FoliageBlendedAmount(1,0.3f,0.0f), 1.0);
    chk("foliage fullmask",S::FoliageBlendedAmount(1,0.3f,1.0f), 0.3);
    chk("foliage halfmask",S::FoliageBlendedAmount(1,0.3f,0.5f), 0.65);
    chk("foliage clamp",   S::FoliageBlendedAmount(1,0.3f,2.0f), 0.3);
    chk("combine zero", S::CombineLocalGlobal(0.4f,1,0.0f), 0.4);
    chk("combine full", S::CombineLocalGlobal(0.4f,1,1.0f), 1.0);
    chk("combine half", S::CombineLocalGlobal(0.4f,1,0.5f), 0.7);
    chk("texel 1080 x", S::TexelStep(1920,1080,1).X, 1.0/1920.0);
    chk("texel 1080 y", S::TexelStep(1920,1080,1).Y, 1.0/1080.0);
    chk("texel 4k half",S::TexelStep(3840,2160,1).X, (1.0/1920.0)*0.5);
    chk("texel 2px",    S::TexelStep(1920,1080,2).X, (1.0/1920.0)*2.0);
    chk("texel zerow",  S::TexelStep(0,1080,1).X, 0.0);
    chk("eff near lit",  S::EffectiveGain(1,0.2f,100,500,5000,0.3f,0.0f,0.8f,0.1f), 1.0);
    chk("eff black gate",S::EffectiveGain(1,0.2f,100,500,5000,0.3f,0.0f,0.0f,0.1f), 0.0);
    chk("eff foliage",   S::EffectiveGain(1,0.2f,100,500,5000,0.3f,1.0f,0.8f,0.1f), 0.3);
    chk("eff far",       S::EffectiveGain(1,0.2f,9000,500,5000,0.3f,0.0f,0.8f,0.1f), 0.2);
    chktrue("eff never negative", S::EffectiveGain(-9,-9,100,500,5000,-9,0.0f,0.8f,0.1f) >= 0.0f);
    // UserSharpnessToAmount — the options-menu "Sharpness" slider bridge.
    chk("user 0",       S::UserSharpnessToAmount(0.0f, 2.0f), 0.0);
    chk("user half",    S::UserSharpnessToAmount(0.5f, 2.0f), 1.0);
    chk("user 1",       S::UserSharpnessToAmount(1.0f, 2.0f), 2.0);
    chk("user over1",   S::UserSharpnessToAmount(9.0f, 2.0f), 2.0);
    chk("user neg",     S::UserSharpnessToAmount(-1.0f, 2.0f), 0.0);
    chk("user clampmax",S::UserSharpnessToAmount(1.0f, 99.0f), S::MaxAmount);
    chk("user negspan", S::UserSharpnessToAmount(1.0f, -3.0f), 0.0);

    std::printf("\n%s  (%d failures)\n", g_fail==0 ? "ALL PASS" : "FAILURES", g_fail);
    return g_fail ? 1 : 0;
}
