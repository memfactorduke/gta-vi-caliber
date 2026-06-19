#include "GTCPhoneIcons.h"

#include "Engine/Texture2D.h"
#include "TextureResource.h"

// A tiny software rasterizer. Every icon is composited in a float (sRGB-space) canvas at
// a supersampled resolution, then box-downsampled to the final size so the crude analytic
// shapes come out cleanly anti-aliased. Shapes are built from three primitives — rounded
// box, circle/ring, and round-capped segment — plus triangles, which is enough to evoke
// each app without ever copying a real-world icon.

namespace
{
	struct FCanvas
	{
		int32 W = 0;
		int32 H = 0;
		TArray<FLinearColor> P; // sRGB-space, straight alpha

		void Init(int32 InW, int32 InH, const FLinearColor& Clear)
		{
			W = InW; H = InH;
			P.Init(Clear, W * H);
		}

		FORCEINLINE void Over(int32 X, int32 Y, const FLinearColor& C, float A)
		{
			if (A <= 0.f || X < 0 || Y < 0 || X >= W || Y >= H) return;
			A = FMath::Clamp(A, 0.f, 1.f);
			FLinearColor& D = P[Y * W + X];
			const float OutA = A + D.A * (1.f - A);
			if (OutA <= KINDA_SMALL_NUMBER) { D = FLinearColor(0,0,0,0); return; }
			D.R = (C.R * A + D.R * D.A * (1.f - A)) / OutA;
			D.G = (C.G * A + D.G * D.A * (1.f - A)) / OutA;
			D.B = (C.B * A + D.B * D.A * (1.f - A)) / OutA;
			D.A = OutA;
		}
	};

	FORCEINLINE float SdRoundBox(float px, float py, float cx, float cy, float halfW, float halfH, float r)
	{
		const float qx = FMath::Abs(px - cx) - (halfW - r);
		const float qy = FMath::Abs(py - cy) - (halfH - r);
		const float ax = FMath::Max(qx, 0.f);
		const float ay = FMath::Max(qy, 0.f);
		return FMath::Sqrt(ax * ax + ay * ay) + FMath::Min(FMath::Max(qx, qy), 0.f) - r;
	}

	// Coverage from a signed distance: 1 inside, 0 outside, ~1px feather across the edge.
	FORCEINLINE float Cov(float sd) { return FMath::Clamp(0.5f - sd, 0.f, 1.f); }

	void RoundBox(FCanvas& C, float cx, float cy, float halfW, float halfH, float r, const FLinearColor& Col)
	{
		const int32 x0 = FMath::FloorToInt(cx - halfW - 1), x1 = FMath::CeilToInt(cx + halfW + 1);
		const int32 y0 = FMath::FloorToInt(cy - halfH - 1), y1 = FMath::CeilToInt(cy + halfH + 1);
		for (int32 y = y0; y <= y1; ++y)
			for (int32 x = x0; x <= x1; ++x)
				C.Over(x, y, Col, Cov(SdRoundBox(x + 0.5f, y + 0.5f, cx, cy, halfW, halfH, r)) * Col.A);
	}

	// Rounded box with a vertical top->bottom gradient.
	void RoundBoxGrad(FCanvas& C, float cx, float cy, float halfW, float halfH, float r,
		const FLinearColor& Top, const FLinearColor& Bot)
	{
		const int32 x0 = FMath::FloorToInt(cx - halfW - 1), x1 = FMath::CeilToInt(cx + halfW + 1);
		const int32 y0 = FMath::FloorToInt(cy - halfH - 1), y1 = FMath::CeilToInt(cy + halfH + 1);
		for (int32 y = y0; y <= y1; ++y)
		{
			const float t = FMath::Clamp(((y + 0.5f) - (cy - halfH)) / (2.f * halfH), 0.f, 1.f);
			const FLinearColor Col = FMath::Lerp(Top, Bot, t);
			for (int32 x = x0; x <= x1; ++x)
				C.Over(x, y, Col, Cov(SdRoundBox(x + 0.5f, y + 0.5f, cx, cy, halfW, halfH, r)));
		}
	}

	void Circle(FCanvas& C, float cx, float cy, float r, const FLinearColor& Col)
	{
		RoundBox(C, cx, cy, r, r, r, Col);
	}

	void Ring(FCanvas& C, float cx, float cy, float rOuter, float thick, const FLinearColor& Col)
	{
		const float rInner = rOuter - thick;
		const int32 x0 = FMath::FloorToInt(cx - rOuter - 1), x1 = FMath::CeilToInt(cx + rOuter + 1);
		const int32 y0 = FMath::FloorToInt(cy - rOuter - 1), y1 = FMath::CeilToInt(cy + rOuter + 1);
		for (int32 y = y0; y <= y1; ++y)
			for (int32 x = x0; x <= x1; ++x)
			{
				const float d = FMath::Sqrt(FMath::Square(x + 0.5f - cx) + FMath::Square(y + 0.5f - cy));
				const float cov = FMath::Min(Cov(d - rOuter), Cov(rInner - d));
				C.Over(x, y, Col, cov * Col.A);
			}
	}

	// Filled circular-arc sweep (for activity rings). Angles in degrees, 0 = up, clockwise.
	void Arc(FCanvas& C, float cx, float cy, float rOuter, float thick, float startDeg, float endDeg, const FLinearColor& Col)
	{
		const float rInner = rOuter - thick;
		const int32 x0 = FMath::FloorToInt(cx - rOuter - 1), x1 = FMath::CeilToInt(cx + rOuter + 1);
		const int32 y0 = FMath::FloorToInt(cy - rOuter - 1), y1 = FMath::CeilToInt(cy + rOuter + 1);
		for (int32 y = y0; y <= y1; ++y)
			for (int32 x = x0; x <= x1; ++x)
			{
				const float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
				const float d = FMath::Sqrt(dx * dx + dy * dy);
				float ang = FMath::RadiansToDegrees(FMath::Atan2(dx, -dy)); // 0=up, CW
				if (ang < 0.f) ang += 360.f;
				const bool bIn = (startDeg <= endDeg) ? (ang >= startDeg && ang <= endDeg)
												  : (ang >= startDeg || ang <= endDeg);
				if (!bIn) continue;
				const float cov = FMath::Min(Cov(d - rOuter), Cov(rInner - d));
				C.Over(x, y, Col, cov * Col.A);
			}
	}

	// Round-capped thick segment.
	void Seg(FCanvas& C, float x0, float y0, float x1, float y1, float halfW, const FLinearColor& Col)
	{
		const int32 minx = FMath::FloorToInt(FMath::Min(x0, x1) - halfW - 1);
		const int32 maxx = FMath::CeilToInt(FMath::Max(x0, x1) + halfW + 1);
		const int32 miny = FMath::FloorToInt(FMath::Min(y0, y1) - halfW - 1);
		const int32 maxy = FMath::CeilToInt(FMath::Max(y0, y1) + halfW + 1);
		const float vx = x1 - x0, vy = y1 - y0;
		const float len2 = FMath::Max(vx * vx + vy * vy, KINDA_SMALL_NUMBER);
		for (int32 y = miny; y <= maxy; ++y)
			for (int32 x = minx; x <= maxx; ++x)
			{
				const float wx = x + 0.5f - x0, wy = y + 0.5f - y0;
				const float t = FMath::Clamp((wx * vx + wy * vy) / len2, 0.f, 1.f);
				const float px = x0 + t * vx, py = y0 + t * vy;
				const float d = FMath::Sqrt(FMath::Square(x + 0.5f - px) + FMath::Square(y + 0.5f - py));
				C.Over(x, y, Col, Cov(d - halfW) * Col.A);
			}
	}

	void Tri(FCanvas& C, float ax, float ay, float bx, float by, float cx, float cy, const FLinearColor& Col)
	{
		const int32 x0 = FMath::FloorToInt(FMath::Min3(ax, bx, cx) - 1), x1 = FMath::CeilToInt(FMath::Max3(ax, bx, cx) + 1);
		const int32 y0 = FMath::FloorToInt(FMath::Min3(ay, by, cy) - 1), y1 = FMath::CeilToInt(FMath::Max3(ay, by, cy) + 1);
		auto Edge = [](float px, float py, float x0_, float y0_, float x1_, float y1_)
		{ return (px - x0_) * (y1_ - y0_) - (py - y0_) * (x1_ - x0_); };
		const float area = Edge(ax, ay, bx, by, cx, cy);
		const float s = area < 0 ? -1.f : 1.f;
		for (int32 y = y0; y <= y1; ++y)
			for (int32 x = x0; x <= x1; ++x)
			{
				// 2x2 subsample for clean diagonals.
				float cov = 0.f;
				for (int32 sy = 0; sy < 2; ++sy)
					for (int32 sx = 0; sx < 2; ++sx)
					{
						const float px = x + 0.25f + 0.5f * sx, py = y + 0.25f + 0.5f * sy;
						const float w0 = Edge(px, py, bx, by, cx, cy) * s;
						const float w1 = Edge(px, py, cx, cy, ax, ay) * s;
						const float w2 = Edge(px, py, ax, ay, bx, by) * s;
						if (w0 >= 0 && w1 >= 0 && w2 >= 0) cov += 0.25f;
					}
				C.Over(x, y, Col, cov * Col.A);
			}
	}

	// ---- texture upload -----------------------------------------------------

	UTexture2D* Finalize(const FCanvas& Hi, int32 OutW, int32 OutH)
	{
		const int32 ssx = Hi.W / OutW;
		const int32 ssy = Hi.H / OutH;
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(OutW * OutH);
		const float inv = 1.f / float(ssx * ssy);
		for (int32 y = 0; y < OutH; ++y)
			for (int32 x = 0; x < OutW; ++x)
			{
				FLinearColor acc(0, 0, 0, 0);
				for (int32 sy = 0; sy < ssy; ++sy)
					for (int32 sx = 0; sx < ssx; ++sx)
						acc += Hi.P[(y * ssy + sy) * Hi.W + (x * ssx + sx)];
				acc *= inv;
				Pixels[y * OutW + x] = acc.ToFColor(false);
			}

		UTexture2D* Tex = UTexture2D::CreateTransient(OutW, OutH, PF_B8G8R8A8);
		if (!Tex) return nullptr;
		Tex->SRGB = true;
		Tex->Filter = TF_Trilinear;
		Tex->AddressX = TA_Clamp;
		Tex->AddressY = TA_Clamp;
		Tex->NeverStream = true;
		FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Tex->UpdateResource();
		return Tex;
	}

	// ---- palette ------------------------------------------------------------

	FLinearColor Hex(uint32 RGB, float A = 1.f)
	{
		return FLinearColor(((RGB >> 16) & 0xFF) / 255.f, ((RGB >> 8) & 0xFF) / 255.f, (RGB & 0xFF) / 255.f, A);
	}
	const FLinearColor White = FLinearColor(1, 1, 1, 1);

	// A soft glossy highlight over the top of the squircle — the subtle sheen that makes a
	// flat colour read as a physical iOS icon. Masked to the squircle so it never leaks past
	// the rounded corners. Kept gentle so it reads as depth, not gloss.
	void TopSheen(FCanvas& C)
	{
		const float S = C.W;
		const float cx = S * 0.5f, cy = S * 0.5f, half = S * 0.5f, r = S * 0.2237f;
		for (int32 y = 0; y < C.H; ++y)
		{
			const float ny = (y + 0.5f) / S;                 // 0 at top .. 1 at bottom
			float a = FMath::Clamp(1.f - ny / 0.55f, 0.f, 1.f);
			a = a * a * 0.17f;                                // ease-in, subtle peak alpha
			if (a <= 0.f) continue;
			for (int32 x = 0; x < C.W; ++x)
				C.Over(x, y, White, a * Cov(SdRoundBox(x + 0.5f, y + 0.5f, cx, cy, half, half, r)));
		}
	}

	// Draw the colored squircle background; returns nothing, leaves room for the glyph.
	void IconBG(FCanvas& C, uint32 top, uint32 bot)
	{
		const float S = C.W;
		RoundBoxGrad(C, S * 0.5f, S * 0.5f, S * 0.5f, S * 0.5f, S * 0.2237f, Hex(top), Hex(bot));
		TopSheen(C);
	}

	// ---- per-app glyphs (drawn in a canvas of size S x S) -------------------

	void DrawGlyph(FCanvas& C, EGTCApp App)
	{
		const float S = C.W;
		const float M = S * 0.5f; // center

		switch (App)
		{
		case EGTCApp::Phone:
		{
			IconBG(C, 0x35E15A, 0x16B23F);
			// Handset: a fat round-capped diagonal with two end pads.
			Seg(C, S * 0.34f, S * 0.34f, S * 0.66f, S * 0.66f, S * 0.115f, White);
			Circle(C, S * 0.34f, S * 0.34f, S * 0.135f, White);
			Circle(C, S * 0.66f, S * 0.66f, S * 0.135f, White);
			// Knock the inner corner out so it reads as a receiver, not a bar.
			Circle(C, S * 0.5f, S * 0.5f, S * 0.135f, Hex(0x22A53A));
			break;
		}
		case EGTCApp::Messages:
		{
			IconBG(C, 0x4CE06B, 0x1FBE4C);
			RoundBox(C, M, S * 0.46f, S * 0.30f, S * 0.225f, S * 0.135f, White);
			Tri(C, S * 0.34f, S * 0.64f, S * 0.46f, S * 0.64f, S * 0.30f, S * 0.80f, White);
			Circle(C, S * 0.40f, S * 0.46f, S * 0.035f, Hex(0x1FBE4C));
			Circle(C, M, S * 0.46f, S * 0.035f, Hex(0x1FBE4C));
			Circle(C, S * 0.60f, S * 0.46f, S * 0.035f, Hex(0x1FBE4C));
			break;
		}
		case EGTCApp::Camera:
		{
			IconBG(C, 0x6E7782, 0x3B4148);
			RoundBox(C, M, S * 0.54f, S * 0.32f, S * 0.20f, S * 0.075f, White);
			RoundBox(C, S * 0.40f, S * 0.36f, S * 0.085f, S * 0.045f, S * 0.03f, White); // hump
			Circle(C, M, S * 0.55f, S * 0.135f, Hex(0x3B4148));
			Ring(C, M, S * 0.55f, S * 0.135f, S * 0.03f, White);
			Circle(C, S * 0.62f, S * 0.40f, S * 0.022f, Hex(0xFFD84A)); // flash
			break;
		}
		case EGTCApp::Music:
		{
			IconBG(C, 0xFF5E7E, 0xF02D6A);
			Circle(C, S * 0.36f, S * 0.66f, S * 0.085f, White);
			Circle(C, S * 0.64f, S * 0.60f, S * 0.085f, White);
			Seg(C, S * 0.435f, S * 0.66f, S * 0.435f, S * 0.34f, S * 0.028f, White);
			Seg(C, S * 0.715f, S * 0.60f, S * 0.715f, S * 0.30f, S * 0.028f, White);
			Seg(C, S * 0.435f, S * 0.34f, S * 0.715f, S * 0.30f, S * 0.05f, White); // beam
			break;
		}
		case EGTCApp::Maps:
		{
			IconBG(C, 0x49D6B0, 0x1FA9C7);
			// Location pin.
			Circle(C, M, S * 0.42f, S * 0.165f, White);
			Tri(C, S * 0.355f, S * 0.50f, S * 0.645f, S * 0.50f, M, S * 0.74f, White);
			Circle(C, M, S * 0.42f, S * 0.07f, Hex(0x1FA9C7));
			break;
		}
		case EGTCApp::Weather:
		{
			IconBG(C, 0x47A8FF, 0x1E6FE0);
			// Sun behind a cloud.
			Circle(C, S * 0.62f, S * 0.36f, S * 0.105f, Hex(0xFFD23F));
			for (int i = 0; i < 8; ++i)
			{
				const float a = i * (PI / 4.f);
				const float sx = S * 0.62f + FMath::Cos(a) * S * 0.155f;
				const float sy = S * 0.36f + FMath::Sin(a) * S * 0.155f;
				Seg(C, S * 0.62f + FMath::Cos(a) * S * 0.125f, S * 0.36f + FMath::Sin(a) * S * 0.125f, sx, sy, S * 0.016f, Hex(0xFFD23F));
			}
			Circle(C, S * 0.40f, S * 0.58f, S * 0.115f, White);
			Circle(C, S * 0.55f, S * 0.55f, S * 0.135f, White);
			RoundBox(C, S * 0.50f, S * 0.66f, S * 0.20f, S * 0.075f, S * 0.075f, White);
			break;
		}
		case EGTCApp::Clock:
		{
			IconBG(C, 0xF7F7FA, 0xDADCE2);
			Circle(C, M, M, S * 0.36f, Hex(0x16181D));
			Ring(C, M, M, S * 0.36f, S * 0.022f, White);
			Seg(C, M, M, M, S * 0.30f, S * 0.018f, White);        // minute (up)
			Seg(C, M, M, S * 0.64f, S * 0.42f, S * 0.022f, White); // hour
			Circle(C, M, M, S * 0.028f, Hex(0xFF7A1A));
			break;
		}
		case EGTCApp::Wallet:
		{
			IconBG(C, 0x3A3F47, 0x1C1F24);
			// Card with a stripe and a contactless mark.
			RoundBox(C, M, M, S * 0.32f, S * 0.205f, S * 0.06f, White);
			RoundBox(C, M, S * 0.42f, S * 0.32f, S * 0.045f, S * 0.0f, Hex(0x2A2E34));
			Arc(C, S * 0.42f, S * 0.58f, S * 0.07f, S * 0.022f, 70, 160, Hex(0x2A2E34));
			Arc(C, S * 0.42f, S * 0.58f, S * 0.11f, S * 0.022f, 70, 160, Hex(0x2A2E34));
			break;
		}
		case EGTCApp::Photos:
		{
			IconBG(C, 0xFCFCFE, 0xEDEFF3);
			// Picture frame with sun + mountains.
			RoundBox(C, M, M, S * 0.30f, S * 0.30f, S * 0.06f, Hex(0xBFC6D0));
			RoundBox(C, M, M, S * 0.265f, S * 0.265f, S * 0.045f, Hex(0xEAF6FF));
			Circle(C, S * 0.40f, S * 0.40f, S * 0.05f, Hex(0xFFC83A));
			Tri(C, S * 0.30f, S * 0.66f, S * 0.55f, S * 0.66f, S * 0.42f, S * 0.46f, Hex(0x57C77A));
			Tri(C, S * 0.45f, S * 0.66f, S * 0.72f, S * 0.66f, S * 0.60f, S * 0.42f, Hex(0x3DA86A));
			break;
		}
		case EGTCApp::Settings:
		{
			IconBG(C, 0x9AA1AC, 0x5E646D);
			// Gear: ring + radial teeth + hub.
			for (int i = 0; i < 8; ++i)
			{
				const float a = i * (PI / 4.f);
				Seg(C, M + FMath::Cos(a) * S * 0.20f, M + FMath::Sin(a) * S * 0.20f,
					   M + FMath::Cos(a) * S * 0.33f, M + FMath::Sin(a) * S * 0.33f, S * 0.05f, White);
			}
			Circle(C, M, M, S * 0.235f, White);
			Circle(C, M, M, S * 0.10f, Hex(0x5E646D));
			break;
		}
		case EGTCApp::Calculator:
		{
			IconBG(C, 0x2C2F36, 0x16181C);
			// Keypad: a grid with one accent column.
			const float gx[3] = { S * 0.36f, M, S * 0.64f };
			const float gy[3] = { S * 0.44f, S * 0.58f, S * 0.72f };
			RoundBox(C, M, S * 0.30f, S * 0.235f, S * 0.06f, S * 0.03f, Hex(0x3A3E45)); // display
			for (int yy = 0; yy < 3; ++yy)
				for (int xx = 0; xx < 3; ++xx)
				{
					const FLinearColor col = (xx == 2) ? Hex(0xFF8F1F) : White;
					RoundBox(C, gx[xx], gy[yy], S * 0.055f, S * 0.045f, S * 0.025f, col);
				}
			break;
		}
		case EGTCApp::Notes:
		{
			IconBG(C, 0xFFD23B, 0xF7B500);
			RoundBox(C, M, M, S * 0.275f, S * 0.32f, S * 0.05f, White);
			RoundBox(C, M, S * 0.34f, S * 0.20f, S * 0.018f, S * 0.018f, Hex(0xF7B500));
			Seg(C, S * 0.34f, S * 0.48f, S * 0.66f, S * 0.48f, S * 0.018f, Hex(0xCBA300));
			Seg(C, S * 0.34f, S * 0.58f, S * 0.66f, S * 0.58f, S * 0.018f, Hex(0xCBA300));
			Seg(C, S * 0.34f, S * 0.68f, S * 0.56f, S * 0.68f, S * 0.018f, Hex(0xCBA300));
			break;
		}
		case EGTCApp::Browser:
		{
			IconBG(C, 0x36B7FF, 0x1577E8);
			// Compass.
			Circle(C, M, M, S * 0.345f, Hex(0xEAF6FF));
			Circle(C, M, M, S * 0.305f, Hex(0x2C9BF0));
			Tri(C, S * 0.42f, S * 0.58f, M, M, S * 0.62f, S * 0.40f, Hex(0xFF4D4D)); // N needle
			Tri(C, S * 0.58f, S * 0.42f, M, M, S * 0.38f, S * 0.60f, White);          // S needle
			Circle(C, M, M, S * 0.03f, Hex(0x16181D));
			break;
		}
		case EGTCApp::Fitness:
		{
			IconBG(C, 0x1A1B1F, 0x0C0D10);
			Ring(C, M, M, S * 0.345f, S * 0.07f, Hex(0x2A2B30));
			Ring(C, M, M, S * 0.255f, S * 0.07f, Hex(0x2A2B30));
			Ring(C, M, M, S * 0.165f, S * 0.07f, Hex(0x2A2B30));
			Arc(C, M, M, S * 0.345f, S * 0.07f, 0, 290, Hex(0xFF2D55));
			Arc(C, M, M, S * 0.255f, S * 0.07f, 0, 230, Hex(0x7CFF3D));
			Arc(C, M, M, S * 0.165f, S * 0.07f, 0, 170, Hex(0x14D9E6));
			break;
		}
		case EGTCApp::Stocks:
		{
			IconBG(C, 0x232732, 0x12141B);
			// Rising line chart.
			const FLinearColor g = Hex(0x4ED88A);
			Seg(C, S * 0.30f, S * 0.64f, S * 0.44f, S * 0.52f, S * 0.026f, g);
			Seg(C, S * 0.44f, S * 0.52f, S * 0.56f, S * 0.58f, S * 0.026f, g);
			Seg(C, S * 0.56f, S * 0.58f, S * 0.72f, S * 0.36f, S * 0.026f, g);
			Tri(C, S * 0.72f, S * 0.36f, S * 0.70f, S * 0.30f, S * 0.78f, S * 0.34f, g); // arrowhead
			Seg(C, S * 0.28f, S * 0.74f, S * 0.74f, S * 0.74f, S * 0.014f, Hex(0x3A4150)); // axis
			break;
		}
		case EGTCApp::Contacts:
		{
			IconBG(C, 0x6C7480, 0x474D57);
			Circle(C, M, S * 0.40f, S * 0.135f, White);
			// Shoulders: a capsule clipped to the lower half.
			RoundBox(C, M, S * 0.78f, S * 0.225f, S * 0.16f, S * 0.16f, White);
			break;
		}
		default: break;
		}
	}

	UTexture2D* MakeIcon(EGTCApp App)
	{
		const int32 Out = 144;
		const int32 SS = 4;
		FCanvas C;
		C.Init(Out * SS, Out * SS, FLinearColor(0, 0, 0, 0));
		DrawGlyph(C, App);
		return Finalize(C, Out, Out);
	}

	UTexture2D* MakeWallpaper()
	{
		// Miami / Caliber-Isles dusk: deep indigo at the top falling to magenta then a hot
		// orange horizon glow, with a soft sun. Generated at native res (smooth gradient).
		const int32 W = 540, H = 1170;
		FCanvas C; C.Init(W, H, FLinearColor(0, 0, 0, 0));
		const FLinearColor c0 = Hex(0x0B1030); // top
		const FLinearColor c1 = Hex(0x3C1A6B);
		const FLinearColor c2 = Hex(0x9B2C7A);
		const FLinearColor c3 = Hex(0xE85A5A);
		const FLinearColor c4 = Hex(0xFFB25A); // horizon
		const float sunX = W * 0.5f, sunY = H * 0.66f;
		for (int32 y = 0; y < H; ++y)
		{
			const float t = float(y) / float(H - 1);
			FLinearColor base;
			if (t < 0.35f)       base = FMath::Lerp(c0, c1, t / 0.35f);
			else if (t < 0.58f)  base = FMath::Lerp(c1, c2, (t - 0.35f) / 0.23f);
			else if (t < 0.72f)  base = FMath::Lerp(c2, c3, (t - 0.58f) / 0.14f);
			else                 base = FMath::Lerp(c3, c4, FMath::Min((t - 0.72f) / 0.18f, 1.f));
			for (int32 x = 0; x < W; ++x)
			{
				FLinearColor col = base;
				// Soft sun glow.
				const float d = FMath::Sqrt(FMath::Square(x - sunX) + FMath::Square(y - sunY));
				const float glow = FMath::Exp(-FMath::Square(d / (W * 0.42f)));
				col = col + Hex(0xFFD08A) * glow * 0.55f;
				// Sun disc.
				if (d < W * 0.16f) col = FMath::Lerp(col, Hex(0xFFE8B0), FMath::Clamp((W * 0.16f - d) / 6.f, 0.f, 1.f) * 0.9f);
				col.A = 1.f;
				C.P[y * W + x] = col.GetClamped();
			}
		}
		// A couple of faint horizon scanlines for a synthwave feel.
		for (int i = 0; i < 5; ++i)
		{
			const float yy = sunY + i * 14.f + 6.f;
			Seg(C, sunX - W * 0.16f, yy, sunX + W * 0.16f, yy, 2.2f, Hex(0x3C1A6B, 0.55f));
		}
		// Round the corners into the alpha so the screen reads as a real display over the
		// black bezel behind it.
		const float r = W * 0.135f;
		for (int32 y = 0; y < H; ++y)
			for (int32 x = 0; x < W; ++x)
			{
				const float a = Cov(SdRoundBox(x + 0.5f, y + 0.5f, W * 0.5f, H * 0.5f, W * 0.5f, H * 0.5f, r));
				C.P[y * W + x].A *= a;
			}
		return Finalize(C, W, H);
	}

	// A stylised top-down city map — Vice-City/Miami flavour: teal water, a sandy beach strip,
	// green land with a road grid + a diagonal boulevard, lighter building blocks, and a park.
	// Drawn once and panned under the player blip by the Maps app. SS=2 for clean roads.
	UTexture2D* MakeMapTexture()
	{
		const int32 OutW = 560, OutH = 380, SS = 2;
		FCanvas C; C.Init(OutW * SS, OutH * SS, Hex(0x123A4A)); // deep water base
		const float W = C.W, H = C.H;

		// Water gradient (lighter near the top).
		for (int32 y = 0; y < C.H; ++y)
		{
			const float t = float(y) / float(C.H - 1);
			const FLinearColor wc = FMath::Lerp(Hex(0x1C5566), Hex(0x0E2E3C), t);
			for (int32 x = 0; x < C.W; ++x) C.P[y * C.W + x] = wc;
		}

		// Main landmass: a big soft rounded block off-centre, with a beach rim.
		RoundBox(C, W * 0.52f, H * 0.54f, W * 0.40f, H * 0.40f, W * 0.05f, Hex(0xE9D9A6)); // beach sand
		RoundBox(C, W * 0.52f, H * 0.54f, W * 0.385f, H * 0.385f, W * 0.045f, Hex(0x3E7A4E)); // green land
		// A second island / peninsula.
		RoundBox(C, W * 0.14f, H * 0.30f, W * 0.13f, H * 0.16f, W * 0.04f, Hex(0xE9D9A6));
		RoundBox(C, W * 0.14f, H * 0.30f, W * 0.115f, H * 0.135f, W * 0.03f, Hex(0x3E7A4E));

		// Park (darker green) + a small lake.
		RoundBox(C, W * 0.40f, H * 0.40f, W * 0.085f, H * 0.10f, W * 0.03f, Hex(0x2E6B41));
		Circle(C, W * 0.40f, H * 0.40f, H * 0.035f, Hex(0x2C7E93)); // pond

		// Road grid over the land (light asphalt), then thin centre lines.
		auto Road = [&](float x0, float y0, float x1, float y1, float w, const FLinearColor& col){ Seg(C, x0, y0, x1, y1, w, col); };
		const FLinearColor RoadCol = Hex(0xC9CDce);
		for (int i = 0; i < 6; ++i)
		{
			const float gx = W * (0.20f + i * 0.13f);
			Road(gx, H * 0.16f, gx, H * 0.92f, W * 0.012f, RoadCol);
		}
		for (int j = 0; j < 5; ++j)
		{
			const float gy = H * (0.22f + j * 0.16f);
			Road(W * 0.15f, gy, W * 0.92f, gy, W * 0.012f, RoadCol);
		}
		// Diagonal boulevard.
		Road(W * 0.16f, H * 0.90f, W * 0.90f, H * 0.20f, W * 0.018f, Hex(0xDDE2E6));

		// A few building blocks (subtle light rects) between roads.
		for (int i = 0; i < 5; ++i)
			for (int j = 0; j < 4; ++j)
			{
				if (((i + j) % 2) == 0) continue;
				const float bx = W * (0.255f + i * 0.13f);
				const float by = H * (0.30f + j * 0.16f);
				RoundBox(C, bx, by, W * 0.028f, H * 0.035f, W * 0.006f, Hex(0x4C8A5E));
			}

		// Beach strip along the bottom-right shoreline (extra sand band).
		RoundBox(C, W * 0.80f, H * 0.86f, W * 0.18f, H * 0.05f, W * 0.02f, Hex(0xEAD9A0));

		return Finalize(C, OutW, OutH);
	}
}

namespace GTCPhoneArt
{
	FGTCPhoneArt Generate()
	{
		FGTCPhoneArt Art;
		Art.AppIcons.SetNum((int32)EGTCApp::MAX);
		for (int32 i = 0; i < (int32)EGTCApp::MAX; ++i)
		{
			Art.AppIcons[i] = MakeIcon((EGTCApp)i);
		}
		Art.Wallpaper = MakeWallpaper();
		Art.MapBackdrop = MakeMapTexture();
		return Art;
	}
}
