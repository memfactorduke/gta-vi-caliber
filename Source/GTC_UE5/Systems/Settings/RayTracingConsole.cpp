// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/IConsoleManager.h"
#include "Logging/LogMacros.h"

// Dev toggle for hardware ray-tracing effects (RT shadows/reflections + Lumen
// hardware RT). r.RayTracing itself is a read-only startup CVar — it allocates
// the ray-tracing scene at init and cannot be flipped at runtime — so this drives
// the runtime master switch r.RayTracing.ForceAllRayTracingEffects instead:
//   -1 = honour each effect's own CVar (engine default / "auto")
//    0 = force every RT effect OFF
//    1 = force every RT effect ON
//
//   gtc.RT [0|1]   toggle RT effects; no arg flips the current state.
//   gtc.RT.Auto    restore per-effect defaults (ForceAllRayTracingEffects = -1).
//   gtc.RT.Status  log the current force state.
//
// The project ships with r.RayTracing=True (Config/DefaultEngine.ini), so the RT
// subsystem is always initialised and these can turn effects on/off freely. Turn
// RT off here when you want to skip RT PSO compiles / GPU cost during dev without
// editing config and relaunching. Dev/test only — compiled out of shipping builds.
#if !UE_BUILD_SHIPPING

namespace
{
    IConsoleVariable* GTCForceRTCVar()
    {
        return IConsoleManager::Get().FindConsoleVariable(
            TEXT("r.RayTracing.ForceAllRayTracingEffects"));
    }

    void GTCSetForceRT(int32 Value)
    {
        if (IConsoleVariable* CVar = GTCForceRTCVar())
        {
            CVar->Set(Value, ECVF_SetByConsole);
            UE_LOG(LogTemp, Log, TEXT("[gtc.RT] ForceAllRayTracingEffects = %d (%s)"),
                Value,
                Value == 0 ? TEXT("forced OFF") : (Value == 1 ? TEXT("forced ON") : TEXT("per-effect defaults")));
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[gtc.RT] r.RayTracing.ForceAllRayTracingEffects not found (ray tracing unavailable on this RHI?)"));
        }
    }

    FAutoConsoleCommand GRTToggleCmd(
        TEXT("gtc.RT"),
        TEXT("Toggle hardware ray-tracing effects. Arg: 0 = off, 1 = on; no arg flips current state."),
        FConsoleCommandWithArgsDelegate::CreateLambda(
            [](const TArray<FString>& Args)
            {
                IConsoleVariable* CVar = GTCForceRTCVar();
                if (!CVar)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[gtc.RT] ray tracing CVar unavailable on this RHI"));
                    return;
                }

                int32 Target;
                if (Args.Num() > 0)
                {
                    Target = (Args[0] == TEXT("0")) ? 0 : 1;
                }
                else
                {
                    // Flip: only an explicit "off" (0) turns back on; otherwise
                    // (default -1 or forced-on 1) the first toggle turns RT off.
                    Target = (CVar->GetInt() == 0) ? 1 : 0;
                }
                GTCSetForceRT(Target);
            }));

    FAutoConsoleCommand GRTAutoCmd(
        TEXT("gtc.RT.Auto"),
        TEXT("Restore per-effect ray-tracing defaults (r.RayTracing.ForceAllRayTracingEffects = -1)."),
        FConsoleCommandDelegate::CreateLambda([]() { GTCSetForceRT(-1); }));

    FAutoConsoleCommand GRTStatusCmd(
        TEXT("gtc.RT.Status"),
        TEXT("Log the current ray-tracing force state."),
        FConsoleCommandDelegate::CreateLambda(
            []()
            {
                IConsoleVariable* CVar = GTCForceRTCVar();
                if (!CVar)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[gtc.RT.Status] ray tracing CVar unavailable on this RHI"));
                    return;
                }
                const int32 V = CVar->GetInt();
                UE_LOG(LogTemp, Log, TEXT("[gtc.RT.Status] ForceAllRayTracingEffects=%d (%s)"),
                    V, V == 0 ? TEXT("forced OFF") : (V == 1 ? TEXT("forced ON") : TEXT("per-effect defaults")));
            }));
}

#endif // !UE_BUILD_SHIPPING
