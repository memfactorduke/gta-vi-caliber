// Tiny headless assert harness for the out-of-tree living-city oracles.
// Each oracle is a standalone executable that includes the REAL pure-core
// header, links the REAL .cpp, and re-asserts the GTC.* test's cases — proving
// the algorithm compiles and runs without a live UE editor.
#pragma once

#include <cstdio>
#include <cmath>

static int g_pass = 0;
static int g_fail = 0;

inline void Check(bool Cond, const char* Msg)
{
    if (Cond) { ++g_pass; }
    else { ++g_fail; std::printf("  FAIL: %s\n", Msg); }
}

inline void CheckNear(double A, double B, const char* Msg, double Tol = 1e-4)
{
    Check(std::fabs(A - B) <= Tol, Msg);
}

inline bool VecNear(const FVector& A, const FVector& B, double Tol = 1e-4)
{
    return std::fabs(A.X - B.X) <= Tol && std::fabs(A.Y - B.Y) <= Tol
        && std::fabs(A.Z - B.Z) <= Tol;
}

inline int OracleSummary(const char* Name)
{
    std::printf("[%s] %s: %d passed, %d failed\n", g_fail == 0 ? "PASS" : "FAIL", Name, g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
