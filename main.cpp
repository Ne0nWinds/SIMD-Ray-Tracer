
#include "base.h"

static v3 Origin;

struct scalar_sphere {
    v3 Position;
    f32 Radius;
};
struct sphere_group {
    v3x Positions;
    f32x Radii;
};
static scalar_sphere ScalarSpheres[8];

static sphere_group Spheres[array_len(ScalarSpheres) / SIMD_WIDTH];

static constexpr inline void InitScalarSpheres(scalar_sphere *ScalarSpheres) {
    ScalarSpheres[0].Position.x = 0.0f;
    ScalarSpheres[0].Position.y = 0.0f;
    ScalarSpheres[0].Position.z = -15.0f;
    ScalarSpheres[0].Radius = 2.0f;

    ScalarSpheres[1].Position.x = -5.0f;
    ScalarSpheres[1].Position.y = 0.0f;
    ScalarSpheres[1].Position.z = -25.0f;
    ScalarSpheres[1].Radius = 2.0f;

    ScalarSpheres[2].Position.x = 5.0f;
    ScalarSpheres[2].Position.y = 2.0f;
    ScalarSpheres[2].Position.z = -25.0f;
    ScalarSpheres[2].Radius = 2.0f;

    ScalarSpheres[3].Position.x = 6.0f;
    ScalarSpheres[3].Position.y = -6.0f;
    ScalarSpheres[3].Position.z = -18.0f;
    ScalarSpheres[3].Radius = 2.0f;
}

static void ConvertScalarSpheresToSIMDSpheres(const scalar_sphere * const Spheres, u32 ScalarLength, sphere_group *SIMDSpheres) {
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
    }
}

void OnInit(init_params *Params) {
    constexpr string8 WindowTitle = u8"Raytracing In One Weekend";
    Params->WindowTitle = WindowTitle;
    Params->WindowWidth = 1280;
    Params->WindowHeight = 720;

    InitScalarSpheres(ScalarSpheres);
    ConvertScalarSpheresToSIMDSpheres(ScalarSpheres, array_len(ScalarSpheres), Spheres);
}

static inline u32 U32FromV4(v4 Value) {
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

__declspec(noinline)
void OnRender(const image &Image) {

    v3 CameraZ = v3(0.0f, 0.0f, 1.0f);
    v3 CameraX = v3::Normalize(v3::Cross(v3(0.0f, 1.0f, 0.0f), CameraZ));
    v3 CameraY = v3::Normalize(v3::Cross(CameraZ, CameraX));
    v3 FilmCenter = Origin - CameraZ;

    f32 FilmW = 1.0f;
    f32 FilmH = 1.0f;
    if (Image.Width > Image.Height) {
        FilmH = (f32)Image.Height / (f32)Image.Width;
    } else {
        FilmW = (f32)Image.Width / (f32)Image.Height;
    }

    for (u32 y = 0; y < Image.Height; ++y) {
        for (u32 x = 0; x < Image.Width; ++x) {

            u32 &Pixel = GetPixel(Image, x, y);
            Pixel = U32FromV4(v4(0.0f, 0.0f, 0.0f, 1.0f));

            f32 FilmX = -1.0f + (x * 2.0f) / (f32)Image.Width;
            f32 FilmY = -1.0f + (y * 2.0f) / (f32)Image.Height;

            v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
            v3 RayDirection = v3::Normalize(FilmP - Origin);

            v3 C = v3();
            f32 MinT = F32Max;

            for (u32 i = 0; i < array_len(Spheres); ++i) {
                const sphere_group &SphereGroup = Spheres[i];

                f32x T = v3x::Dot(SphereGroup.Positions, RayDirection);
                v3x ProjectedPoint = v3x(RayDirection) * T;

                const f32x &Radius = SphereGroup.Radii;
                f32x DistanceFromCenter = v3x::Length(SphereGroup.Positions - ProjectedPoint);

                f32x HitMask = DistanceFromCenter < Radius;

                if (IsZero(HitMask)) continue;
                C = v3(0.5);
            }

            v4 Color = v4(C.x, C.y, C.z, 1.0f);
            Pixel = U32FromV4(Color);

/*
            for (u32 i = 0; i < array_len(Spheres); ++i) {
                sphere Sphere = Spheres[i];
                f32 T = v3::Dot(Sphere.Position, RayDirection);
                v3 ProjectedPoint = RayDirection * T;

                f32 Radius = Sphere.Radius;
                f32 DistanceFromCenter = v3::Length(Sphere.Position - ProjectedPoint);
                if (DistanceFromCenter > Radius) continue;
                if (T > MinT) continue;
                MinT = T;
                f32 X = Sqrt(Radius*Radius - DistanceFromCenter*DistanceFromCenter);

                v3 IntersectionPoint = RayDirection * (T - X);
                v3 Normal = v3::Normalize(IntersectionPoint - Sphere.Position);
                Normal += 1.0f;
                Normal *= 0.5f;
                C = Normal;
            }
*/
        }
    }
}