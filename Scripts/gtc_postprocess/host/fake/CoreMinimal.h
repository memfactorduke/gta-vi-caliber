#pragma once
// Minimal UE stubs so the pure-core FSmartSharpen math compiles & runs on host,
// with no engine dependency. Mirrors just the types/precision the math uses.
#define GTC_UE5_API
struct FLinearColor { float R, G, B, A; FLinearColor(float r=0,float g=0,float b=0,float a=1):R(r),G(g),B(b),A(a){} };
struct FVector2D { double X, Y; FVector2D(double x=0,double y=0):X(x),Y(y){} static const FVector2D ZeroVector; };
