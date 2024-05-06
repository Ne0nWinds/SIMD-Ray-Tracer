
#include "base.h"

static v3 CameraPosition;

struct scalar_sphere {
    v3 Position;
    f32 Radius;
    v3 Color;
    f32 Specular;
};
struct sphere_group {
    v3x Positions;
    f32x Radii;
};
struct material {
    v3 Color;
    f32 Specular;
};

static scalar_sphere ScalarSpheres[8];
static sphere_group Spheres[array_len(ScalarSpheres) / SIMD_WIDTH];
static material Materials[array_len(ScalarSpheres)];
static u32_random_state RandomState = { 0x4d595df4d0f33173ULL };

constexpr inline void CreateScalarSphere(const v3 &Position, f32 Radius, const v3 &Color, f32 Specular, scalar_sphere *Sphere) {
    Sphere->Position = Position;
    Sphere->Radius = Radius;
    Sphere->Color = Color;
    Sphere->Specular = Specular;
}

static constexpr inline void InitScalarSpheres(scalar_sphere *ScalarSpheres) {
    CreateScalarSphere(v3(0.0f, 0.0f, -15.0f), 2.0f, v3(1.0f, 0.125f, 0.0f), 1.0f, ScalarSpheres + 0);
    CreateScalarSphere(v3(0.0f, -130.0f, -15.0f), 128.0f, v3(0.2f), 0.0f, ScalarSpheres + 1);
    CreateScalarSphere(v3(5.0f, 2.0f, -25.0f), 2.0f, v3(0.0f, 0.0f, 1.0f), 1.0f, ScalarSpheres + 2);
    CreateScalarSphere(v3(6.0f, 6.0f, -18.0f), 2.0f, v3(0.75f, 0.85f, 0.125f), 1.0f, ScalarSpheres + 3);
    CreateScalarSphere(v3(-7.0f, -0.5f, -25.0f), 1.25f, v3(1.0f, 0.5f, 0.0f), 0.95f, ScalarSpheres + 4);
    CreateScalarSphere(v3(7.0f, 6.0f, -30.0f), 3.0f, v3(0.125f, 0.5f, 0.2f), 1.0f, ScalarSpheres + 5);
    CreateScalarSphere(v3(-3.0f, 3.0f, -30.0f), 2.5f, v3(0.25f, 0.15f, 0.12f), 1.0f, ScalarSpheres + 6);
    CreateScalarSphere(v3(-12.0f, 3.0f, -45.0f), 1.0f, v3(0.65f, 0.25f, 0.42f), 0.0f, ScalarSpheres + 7);
}

static void constexpr ConvertScalarSpheresToSIMDSpheres(const scalar_sphere * const Spheres, u32 ScalarLength, sphere_group *SIMDSpheres) {
    for (u32 i = 0; i < ScalarLength; i += SIMD_WIDTH) {
        sphere_group &SphereGroup = SIMDSpheres[i / SIMD_WIDTH];
        for (u32 j = 0; j < SIMD_WIDTH; ++j) {
            f32 R = Spheres[i + j].Radius;
            SphereGroup.Radii[j] = R;
        }
        for (u32 j = 0; j < SIMD_WIDTH; ++j) {
            const v3 &Position = Spheres[i + j].Position;
            SphereGroup.Positions[j] = Position;
        }
        for (u32 j = 0; j < SIMD_WIDTH; ++j) {
            const v3 &Color = Spheres[i + j].Color;
            const f32 &Specular = Spheres[i + j].Specular;
            Materials[i + j].Color = Color;
            Materials[i + j].Specular = Specular;
        }
    }
}

static memory_arena RenderData;

void OnInit(init_params *Params) {
    constexpr string8 WindowTitle = u8"Raytracing In One Weekend";
    Params->WindowTitle = WindowTitle;
    Params->WindowWidth = 1280;
    Params->WindowHeight = 720;

    InitScalarSpheres(ScalarSpheres);
    ConvertScalarSpheresToSIMDSpheres(ScalarSpheres, array_len(ScalarSpheres), Spheres);

    RenderData = AllocateArenaFromOS(MB(256));
}

static inline constexpr u32 ColorFromV4(const v4 &Value) {
    u8 r = Saturate(Value.x) * 255.0f;
    u8 g = Saturate(Value.y) * 255.0f;
    u8 b = Saturate(Value.z) * 255.0f;
    u8 a = 255;
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

static inline u32& GetPixel(const image &Image, u32 X, u32 Y) {
    u32 *ImageData = (u32 *)Image.Data;
#if defined(PLATFORM_WASM)
    Y = Image.Height - Y - 1;
    return ImageData[Y * Image.Width + X];
#else
    return ImageData[Y * Image.Width + X];
#endif
}

static inline v4& GetPixelV4(const image &Image, u32 X, u32 Y) {
    v4 *ImageData = (v4 *)Image.Data;
    return ImageData[Y * Image.Width + X];
}

static f32 LinearToSRGB(f32 L) {
	f32 Result; 

	L = Saturate(L);

	if (L < 0.0031308f) {
		Result = L * 12.92f;
	} else {
#if 0
		Result = 1.055f * powf(L, 1.0f / 2.4f) - 0.055f;
#else
		// bad but fast code
		Result = 1.02f * SquareRoot(L);
#endif
	}

	return Result;
}

static v4 LinearToSRGB(v4 L) {
    v4 Result;
    Result.x = LinearToSRGB(L.x);
    Result.y = LinearToSRGB(L.y);
    Result.z = LinearToSRGB(L.z);
    Result.w = 1.0f;
    return Result;
}


static u32 PreviousRayCount = 0;
static image PreviousImage = {};

void OnRender(const image &Image) {

    v3 Movement = 0.0f;
    {
        if (IsDown(key::W) || IsDown(key::ArrowUp)) {
            Movement.z -= 0.5f;
        }
        if (IsDown(key::S) || IsDown(key::ArrowDown)) {
            Movement.z += 0.5f;
        }
        if (IsDown(key::D) || IsDown(key::ArrowRight)) {
            Movement.x += 0.5f;
        }
        if (IsDown(key::A) || IsDown(key::ArrowLeft)) {
            Movement.x -= 0.5f;
        }
        if (IsDown(key::Space)) {
            Movement.y += 0.5f;
        }

        bool FlyDown = IsDown(key::C);
#ifndef PLATFORM_WASM
        // Ctrl+W will close the browser window
        FlyDown |= IsDown(key::LeftControl);
#endif
        if (FlyDown) {
            Movement.y -= 0.5f;
        }
        CameraPosition += Movement;
    }

    if (Image.Width != PreviousImage.Width || Image.Height != PreviousImage.Height || v3::Length(Movement) > 0.0f || IsDown(key::R)) {
        PreviousRayCount = 0;
        RenderData.Reset();
        PreviousImage.Width = Image.Width;
        PreviousImage.Height = Image.Height;
        PreviousImage.Data = (v4 *)RenderData.Push(Image.Width * Image.Height * sizeof(v4));
    }

    f64 StartTime = QueryTimestampInMilliseconds();

    v3 CameraZ = v3(0.0f, 0.0f, 1.0f);
    v3 CameraX = v3::Normalize(v3::Cross(v3(0.0f, 1.0f, 0.0f), CameraZ));
    v3 CameraY = v3::Normalize(v3::Cross(CameraZ, CameraX));
    v3 FilmCenter = CameraPosition - CameraZ;

    f32 FilmW = 1.0f;
    f32 FilmH = 1.0f;
    if (Image.Width > Image.Height) {
        FilmH = (f32)Image.Height / (f32)Image.Width;
    } else {
        FilmW = (f32)Image.Width / (f32)Image.Height;
    }

#if 1
    for (u32 y = 0; y < Image.Height; ++y) {
        for (u32 x = 0; x < Image.Width; ++x) {

            v3 Attenuation = 1.0f;
            v3 OutputColor = 0.0f;

            f32 JitterX = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 JitterY = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 FilmX = -1.0f + ((x + JitterX) * 2.0f) / (f32)Image.Width;
            f32 FilmY = -1.0f + ((y + JitterY) * 2.0f) / (f32)Image.Height;

            v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
            v3 RayOrigin = CameraPosition;
            v3 RayDirection = v3::Normalize(FilmP - RayOrigin);

            u32 MaxRayBounce = 6;
            for (u32 i = 0; i < MaxRayBounce; ++i) {
                f32 A = (RayDirection.y + 1.0f) * 0.5f;
                const v3 DefaultColor = (1.0f - A) * v3(1.0f) + A * v3(0.5, 0.7, 1.0);
                v3x HitEmissive = DefaultColor;
                v3x HitNormal = 0.0f;
                v3x NextRayOrigin = 0.0f;
                f32x MinT = F32Max;

                u32x MaterialIndex = 0;

                for (u32 s = 0; s < array_len(Spheres); ++s) {
                    const sphere_group &SphereGroup = Spheres[s];
                    v3x SphereCenter = SphereGroup.Positions - RayOrigin;
                    f32x T = v3x::Dot(SphereCenter, RayDirection);
                    v3x ProjectedPoint = v3x(RayDirection) * T;

                    const f32x &Radius = SphereGroup.Radii;
                    f32x DistanceFromCenter = v3x::Length(SphereCenter - ProjectedPoint);

                    f32x HitMask = DistanceFromCenter < Radius;

                    if (IsZero(HitMask)) continue;

                    f32x X = f32x::SquareRoot(Radius * Radius - DistanceFromCenter * DistanceFromCenter);

                    f32x IntersectionT = T - X;
                    // f32x::ConditionalMove(&IntersectionT, T + X, IntersectionT < 0);

                    v3x IntersectionPoint = RayDirection * IntersectionT;
                    f32x MinMask = (IntersectionT < MinT) & (IntersectionT > F32Epsilon);
                    f32x MoveMask = MinMask & HitMask;

                    v3x Normal = (IntersectionPoint - SphereCenter) * f32x::Reciprocal(Radius);
                    f32x::ConditionalMove(&MinT, IntersectionT, MoveMask);
                    v3x::ConditionalMove(&HitNormal, Normal, MoveMask);
                    v3x::ConditionalMove(&HitEmissive, 0, MoveMask);
                    v3x::ConditionalMove(&NextRayOrigin, RayOrigin + IntersectionPoint, MoveMask);
                }

                u32 Index = f32x::HorizontalMinIndex(MinT);
                OutputColor += v3(HitEmissive[Index]) * Attenuation;

                const material &Material = Materials[Index];
                Attenuation *= Material.Color;
                RayOrigin = v3(NextRayOrigin[Index]);

                f32 Specular = Material.Specular;
                v3 Normal = v3(HitNormal[Index]);
                v3 PureBounce = RayDirection - 2.0f * v3::Dot(RayDirection, Normal) * Normal;
                v3 RandomV3 = v3(RandomState.RandomFloat(), RandomState.RandomFloat(), RandomState.RandomFloat());
                v3 RandomBounce = Normal + RandomV3;
                RayDirection = (1.0 - Specular) * RandomBounce + (Specular * PureBounce);
                RayDirection = v3::Normalize(RayDirection);

                if (MinT[Index] == F32Max) break;
            }

            u32 TotalRayCount = PreviousRayCount + 1;
            v4 &PreviousColor = GetPixelV4(PreviousImage, x, y);
            v4 OutputColorV4 = v4(OutputColor.x, OutputColor.y, OutputColor.z, 1.0f);
            v4 FinalColor = OutputColorV4 * (1.0f / (f32)TotalRayCount) + PreviousColor * ((f32)PreviousRayCount / (f32)TotalRayCount);
            FinalColor.w = 1.0f;
            PreviousColor = FinalColor;

            u32 &Pixel = GetPixel(Image, x, y);
            Pixel = ColorFromV4(LinearToSRGB(FinalColor));
        }
    }

    PreviousRayCount += 1;
#else
        for (u32 y = 0; y < Image.Height; ++y) {
            for (u32 x = 0; x < Image.Width; ++x) {

                u32 &Pixel = GetPixel(Image, x, y);
                Pixel = ColorFromV4(v4(0.0f, 0.0f, 0.0f, 1.0f));

                f32 FilmX = -1.0f + (x * 2.0f) / (f32)Image.Width;
                f32 FilmY = -1.0f + (y * 2.0f) / (f32)Image.Height;

                v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
                v3 RayDirection = v3::Normalize(FilmP - Origin);
                v3 RayOrigin = Origin;

                v3 Attenuation = 1.0f;
                v3 OutputColor = v3(0.0f);

                u32 MaxRayBounce = 2;
                for (u32 i = 0; i < MaxRayBounce; ++i) {
                    v3 HitNormal = 0.0f;
                    v3 HitColor = 0.0f;
                    v3 Emissive = 1.0f;
                    f32 MinT = F32Max;
                    for (const scalar_sphere &Sphere : ScalarSpheres) {
                        v3 SphereCenter = Sphere.Position - RayOrigin;
                        f32 T = v3::Dot(SphereCenter, RayDirection);
                        v3 ProjectedPoint = RayDirection * T;

                        f32 Radius = Sphere.Radius;
                        f32 DistanceFromCenter = v3::Length(SphereCenter - ProjectedPoint);
                        if (DistanceFromCenter > Radius) continue;
                        if (T < F32Epsilon) continue;
                        f32 X = SquareRoot(Radius*Radius - DistanceFromCenter*DistanceFromCenter);

                        f32 T1 = T - X;
                        // f32 T2 = T + X;
                        // T = (T1 > 0) ? T1 : T2;

                        if (T > MinT) continue;
                        MinT = T;

                        v3 IntersectionPoint = RayDirection * T1;
                        v3 Normal = v3::Normalize(IntersectionPoint - SphereCenter);
                        HitNormal = Normal;
                        HitColor = Sphere.Color;
                        Emissive = 0.0f;
                        RayOrigin = RayOrigin + IntersectionPoint;
                        RayDirection = HitNormal;
                    }
                    OutputColor += Attenuation * Emissive;
                    Attenuation *= HitColor;

                    if (MinT == F32Max) break;
                }

                v4 Color = v4(OutputColor.x, OutputColor.y, OutputColor.z, 1.0f);
                Pixel = ColorFromV4(Color);
            }
        }
#endif

    f64 EndTime = QueryTimestampInMilliseconds();
    volatile f64 TimeElapsed = EndTime - StartTime;
    (void)TimeElapsed;
}