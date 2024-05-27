
#include "base.h"

static v3 CameraPosition;

struct scalar_sphere {
    v3 Position;
    f32 Radius;
    v3 Color;
    f32 Specular;
};
struct sphere_group {
    v3x4 Positions;
    f32x4 Radii;
    v3x4 Color;
    f32x4 Specular;
};
static scalar_sphere ScalarSpheres[8];
static sphere_group SphereGroups[array_len(ScalarSpheres) / SIMD_WIDTH];

static constexpr inline void InitScalarSpheres(scalar_sphere *ScalarSpheres);
static void constexpr ConvertScalarSpheresToSIMDSpheres(const scalar_sphere * const Spheres, u32 ScalarLength, sphere_group *SIMDSpheres);

void OnInit(init_params *Params) {
    constexpr string8 WindowTitle = u8"Raytracing In One Weekend";
    Params->WindowTitle = WindowTitle;
    Params->WindowWidth = 640;
    Params->WindowHeight = 640;

    InitScalarSpheres(ScalarSpheres);
    ConvertScalarSpheresToSIMDSpheres(ScalarSpheres, array_len(ScalarSpheres), SphereGroups);
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

    f64 StartTime = QueryTimestampInMilliseconds();

    v3 CameraZ = v3(0.0f, 0.0f, 1.0f);
    v3 CameraX = v3(1.0f, 0.0f, 0.0f);
    v3 CameraY = v3(0.0f, 1.0f, 0.0f);
    v3 FilmCenter = CameraPosition - CameraZ;

    f32 FilmW = 1.0f;
    f32 FilmH = 1.0f;
    if (Image.Width > Image.Height) {
        FilmH = (f32)Image.Height / (f32)Image.Width;
    } else {
        FilmW = (f32)Image.Width / (f32)Image.Height;
    }

    for (u32 y = 0; y < Image.Height; ++y) {
        for (u32 x = 0; x < Image.Width; ++x) {
            f32 FilmX = -1.0f + (x * 2.0f) / (f32)Image.Width;
            f32 FilmY = -1.0f + (y * 2.0f) / (f32)Image.Height;

            v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
            v3 RayOrigin = CameraPosition;
            v3 RayDirection = v3::Normalize(FilmP - RayOrigin);

            v3 DefaultColor = v3(0.0);

#if 1
            // SIMD version
            v3x4 Color = v3x4(DefaultColor);
            f32x4 MinT = F32Max;

            for (const sphere_group &SphereGroup : SphereGroups) {
                v3x4 SphereCenter = SphereGroup.Positions - RayOrigin;
                f32x4 T = v3x4::Dot(SphereCenter, RayDirection);
                v3x4 ProjectedPoint = RayDirection * T;

                f32x4 Radius = SphereGroup.Radii;
                f32x4 DistanceFromCenter = v3x4::Length(SphereCenter - ProjectedPoint);
                f32x4 HitMask = DistanceFromCenter < Radius;
                if (IsZero(HitMask)) continue;
                
                f32x4 X = f32x4::SquareRoot(Radius*Radius - DistanceFromCenter*DistanceFromCenter);
                T = T - X;
                
                f32x4 MinMask = (T < MinT) & (T > 0);
                f32x4 MoveMask = MinMask & HitMask;
                if (IsZero(MoveMask)) continue;

                v3x4 IntersectionPoint = RayDirection * T;
                v3x4 Normal = v3x4::Normalize(IntersectionPoint - SphereCenter);

                f32x4::ConditionalMove(&MinT, T, MoveMask);
                v3x4::ConditionalMove(&Color, (Normal + 1.0f) * 0.5f, MoveMask);
            }

            u32 Index = f32x4::HorizontalMinIndex(MinT);

            v4 OutputColor;
            OutputColor.x = Color.x[Index];
            OutputColor.y = Color.y[Index];
            OutputColor.z = Color.z[Index];
            OutputColor.w = 1.0;
#else
            // Scalar version
            v3 Color = DefaultColor;
            f32 MinT = F32Max;

            for (const scalar_sphere &Sphere : ScalarSpheres) {
                v3 SphereCenter = Sphere.Position - RayOrigin;
                f32 T = v3::Dot(SphereCenter, RayDirection);
                v3 ProjectedPoint = RayDirection * T;

                f32 Radius = Sphere.Radius;
                f32 DistanceFromCenter = v3::Length(SphereCenter - ProjectedPoint);
                if (DistanceFromCenter > Radius) continue;
                
                f32 X = SquareRoot(Radius*Radius - DistanceFromCenter*DistanceFromCenter);
                T = T - X;
                
                if (T > MinT || T < 0) continue;
                MinT = T;

                v3 IntersectionPoint = RayDirection * T;

                // Calculate Color Value
                v3 Normal = v3::Normalize(IntersectionPoint - SphereCenter);
                Color = (Normal + 1.0f) * 0.5f;
            }

            v4 OutputColor;
            OutputColor.x = Color.x;
            OutputColor.y = Color.y;
            OutputColor.z = Color.z;
            OutputColor.w = 1.0;
#endif
            u32 &Pixel = GetPixel(Image, x, y);
            Pixel = ColorFromV4(OutputColor);
        }
    }

    f64 EndTime = QueryTimestampInMilliseconds();
    volatile f64 TimeElapsed = EndTime - StartTime;
    (void)TimeElapsed;
}

static constexpr inline void InitScalarSpheres(scalar_sphere *ScalarSpheres) {
    ScalarSpheres[0].Position.x = 0.0f;
    ScalarSpheres[0].Position.y = 0.0f;
    ScalarSpheres[0].Position.z = -15.0f;
    ScalarSpheres[0].Radius = 2.0f;
    ScalarSpheres[0].Color.x = 1.0f;
    ScalarSpheres[0].Color.y = 0.125f;
    ScalarSpheres[0].Color.z = 0.0f;
    ScalarSpheres[0].Specular = 1.0f;

    ScalarSpheres[1].Position.x = 0.0f;
    ScalarSpheres[1].Position.y = -130.0f;
    ScalarSpheres[1].Position.z = -15.0f;
    ScalarSpheres[1].Radius = 128.0f;
    ScalarSpheres[1].Color.x = 0.2f;
    ScalarSpheres[1].Color.y = 0.2f;
    ScalarSpheres[1].Color.z = 0.2f;

    ScalarSpheres[2].Position.x = 5.0f;
    ScalarSpheres[2].Position.y = 2.0f;
    ScalarSpheres[2].Position.z = -25.0f;
    ScalarSpheres[2].Radius = 2.0f;
    ScalarSpheres[2].Color.x = 0.0f;
    ScalarSpheres[2].Color.y = 0.0f;
    ScalarSpheres[2].Color.z = 1.0f;

    ScalarSpheres[3].Position.x = 6.0f;
    ScalarSpheres[3].Position.y = 6.0f;
    ScalarSpheres[3].Position.z = -18.0f;
    ScalarSpheres[3].Radius = 2.0f;
    ScalarSpheres[3].Color.x = 0.75f;
    ScalarSpheres[3].Color.y = 0.85f;
    ScalarSpheres[3].Color.z = 0.125f;

    ScalarSpheres[4].Position.x = -7.0f;
    ScalarSpheres[4].Position.y = -0.5f;
    ScalarSpheres[4].Position.z = -25.0f;
    ScalarSpheres[4].Radius = 1.25f;
    ScalarSpheres[4].Color.x = 1.0f;
    ScalarSpheres[4].Color.y = 0.5f;
    ScalarSpheres[4].Color.z = 0.0f;
    ScalarSpheres[4].Specular = 0.95f;
    
    ScalarSpheres[5].Position.x = 7.0f;
    ScalarSpheres[5].Position.y = 6.0f;
    ScalarSpheres[5].Position.z = -30.0f;
    ScalarSpheres[5].Radius = 3.0f;
    ScalarSpheres[5].Color.x = 0.125f;
    ScalarSpheres[5].Color.y = 0.5f;
    ScalarSpheres[5].Color.z = 0.2f;

    ScalarSpheres[6].Position.x = -3.0f;
    ScalarSpheres[6].Position.y = 3.0f;
    ScalarSpheres[6].Position.z = -30.0f;
    ScalarSpheres[6].Radius = 2.5f;
    ScalarSpheres[6].Color.x = 0.25f;
    ScalarSpheres[6].Color.y = 0.15f;
    ScalarSpheres[6].Color.z = 0.12f;

    ScalarSpheres[7].Position.x = -12.0f;
    ScalarSpheres[7].Position.y = 3.0f;
    ScalarSpheres[7].Position.z = -45.0f;
    ScalarSpheres[7].Radius = 1.0f;
    ScalarSpheres[7].Color.x = 0.65f;
    ScalarSpheres[7].Color.y = 0.25f;
    ScalarSpheres[7].Color.z = 0.42f;
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
            SphereGroup.Color[j] = Color;
        }
        for (u32 j = 0; j < SIMD_WIDTH; ++j) {
            const f32 &Specular = Spheres[i + j].Specular;
            SphereGroup.Specular[j] = Specular;
        }
    }
}
