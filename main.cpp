
#include "base.h"

static u32 PreviousRayCount = 0;
static constexpr u32 TileSize = 128;

struct material {
    v3 Color;
    v3 Emissive;
    f32 Specular;
    f32 IndexOfRefraction;
};
struct scalar_sphere {
    v3 Position;
    f32 Radius;
    material Material;
};

struct sphere_group {
    v3x Positions;
    f32x Radii;
};

static scalar_sphere ScalarSpheres[128];
static sphere_group Spheres[array_len(ScalarSpheres) / SIMD_WIDTH];
static material Materials[array_len(ScalarSpheres) + 1];

static constexpr f32 WorldScale = 1.0f / 16.0f;
constexpr inline void CreateScalarSphere(const v3 &Position, f32 Radius, const v3 &Color, f32 Specular, f32 IndexOfRefraction, const v3 &Emissive, scalar_sphere *Sphere) {
    Sphere->Position.x = Position.x * WorldScale;
    Sphere->Position.y = Position.y * WorldScale;
    Sphere->Position.z = Position.z * WorldScale;
    Sphere->Radius = Radius * WorldScale;
    Sphere->Material.Color = Color;
    Sphere->Material.Specular = Specular;
    Sphere->Material.Emissive = Emissive;
    Sphere->Material.IndexOfRefraction = IndexOfRefraction;
}

static constexpr inline void InitScalarSpheres(scalar_sphere *SpheresArray) {
#if 0
    CreateScalarSphere(v3(0.0f, 0.0f, -15.0f), 2.0f, v3(1.0f), 1.0f, 1.5f, 0.0f, ScalarSpheres + 0);
    CreateScalarSphere(v3(0.0f, -130.0f, -15.0f), 128.0f, v3(0.2f), 0.0f, 0.0f, 0.0f, ScalarSpheres + 1);
    CreateScalarSphere(v3(5.0f, 2.0f, -25.0f), 2.0f, v3(0.0f, 0.0f, 1.0f), 1.0f, 0.0f, 0.0f, ScalarSpheres + 2);
    CreateScalarSphere(v3(6.0f, 6.0f, -18.0f), 2.0f, v3(0.75f, 0.85f, 0.125f), 1.0f, 0.0f, 0.0f, ScalarSpheres + 3);
    CreateScalarSphere(v3(-7.0f, -0.5f, -25.0f), 1.25f, v3(1.0f, 0.5f, 0.0f), 0.95f, 0.0f, 0.0f, ScalarSpheres + 4);
    CreateScalarSphere(v3(7.0f, 6.0f, -30.0f), 3.0f, v3(0.125f, 0.5f, 0.2f), 1.0f, 0.0f, 0.0f, ScalarSpheres + 5);
    CreateScalarSphere(v3(-3.0f, 3.0f, -30.0f), 2.5f, v3(0.25f, 0.15f, 0.12f), 1.0f, 0.0f, 0.0f, ScalarSpheres + 6);
    CreateScalarSphere(v3(-12.0f, 3.0f, -45.0f), 1.0f, v3(0.65f, 0.25f, 0.42f), 0.0f, 0.0f, 0.0f, ScalarSpheres + 7);
    
#elif 1
    u32_random_state RandomState = { 0xCD46749A57ACB371 };
    constexpr u32 Length = array_len(ScalarSpheres);
    for (u32 i = 0; i < Length; ++i) {
        v3 Position = 0.0f;
        Position.x = RandomState.RandomFloat(-36.0f, 36.0f);
        Position.y = RandomState.RandomFloat(-36.0f, 36.0f);
        Position.z = RandomState.RandomFloat(-36.0f, 36.0f);

        f32 Radius = RandomState.RandomFloat(0.25f, 5.0f);

        v3 Color = 0.0f;
        Color.x = RandomState.RandomFloat(0.1f, 1.0f);
        Color.y = RandomState.RandomFloat(0.1f, 1.0f);
        Color.z = RandomState.RandomFloat(0.1f, 1.0f);

        v3 Emissive = 0.0f;
        f32 Specular = 0.0f;
        f32 IndexOfRefraction = 0.0f;
        if (RandomState.RandomFloat(0.0f) < 0.25f) {
            Emissive = RandomState.RandomFloat(1.0f, 5.0f) * Color;
        } else {
            f32 Random = RandomState.RandomFloat(0.0f);
            if (Random < 0.25f) {
                Color = 1.0f;
                IndexOfRefraction = 1.5f;
            } else {
                Specular = Random > (0.75f / 2.0f) ? 1.0f : 0.0f;
            }
        }

        CreateScalarSphere(Position, Radius, Color, Specular, IndexOfRefraction, Emissive, SpheresArray + i);
    }
#else
    CreateScalarSphere(v3(0.0f, -256 - 2.0f, -15.0f), 256.0f, v3(0.2f), 0.0f, 0.0f, 0.0f, ScalarSpheres + 0);
    CreateScalarSphere(v3(0.0f, 0, -10.0f), 2.0f, v3(1.0f), 0.0f, 1.5f, 0.0f, ScalarSpheres + 1);

    CreateScalarSphere(v3(-4.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(8.0f, 0.0f, 0.0f), ScalarSpheres + 2);
    CreateScalarSphere(v3(0.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(0.0f, 8.0f, 0.0f), ScalarSpheres + 3);
    CreateScalarSphere(v3(4.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(0.0f, 0.0f, 8.0f), ScalarSpheres + 4);
#endif
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
            scalar_sphere Sphere = Spheres[i + j];
            Materials[i + j + 1] = Sphere.Material;
        }
    }
}

struct camera_info {
    v3 CameraPosition;
    v3 CameraZ;
    v3 CameraX;
    v3 CameraY;
    v3 FilmCenter;
    f32 FilmW;
    f32 FilmH;
    u32 TilesX;

    image CurrentImage;
    image PreviousImage;
};

struct thread_context {
    u32_random_state RandomState;
    // camera_info *CameraInfo;
}__attribute__((__aligned__(64)));

static camera_info CameraInfo = {};
static thread_context *ThreadContexts = 0;

static f32 Reflectance(f32 CosTheta, f32 RefractionIndex) {
    f32 R0 = (1.0f - RefractionIndex) / (1.0f + RefractionIndex);
    R0 *= R0;

    f32 R1 = 1.0f - CosTheta;
    R1 = R1 * R1 * R1 * R1 * R1;

    return R0 + (1.0f - R0) * R1;
}

static inline v4& GetPixelV4(const image &Image, u32 X, u32 Y) {
    v4 *ImageData = (v4 *)Image.Data;
    return ImageData[Y * Image.Width + X];
}
static inline u32& GetPixel(const image &Image, u32 X, u32 Y) {
    u32 *ImageData = (u32 *)Image.Data;
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
        Result = SquareRoot(L);
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

static inline constexpr u32 ColorFromV4(const v4 &Value) {
    u8 r = Saturate(Value.x) * 255.0f;
    u8 g = Saturate(Value.y) * 255.0f;
    u8 b = Saturate(Value.z) * 255.0f;
    u8 a = 255;
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

static void RenderTile(work_queue_context *WorkQueueContext) {

    u32_random_state &RandomState = ThreadContexts[WorkQueueContext->ThreadIndex].RandomState;

    const image &CurrentImage = CameraInfo.CurrentImage;
    const image &PreviousImage = CameraInfo.PreviousImage; 
    const v3 &FilmCenter = CameraInfo.FilmCenter;
    const v3 &FilmW = CameraInfo.FilmW;
    const v3 &FilmH = CameraInfo.FilmH;
    const v3 &CameraX = CameraInfo.CameraX;
    const v3 &CameraY = CameraInfo.CameraY;
    // const v3 &CameraZ = CameraInfo.CameraZ;

    u32 Tile = WorkQueueContext->WorkEntry;
    u32 TileX = Tile % CameraInfo.TilesX;
    u32 TileY = Tile / CameraInfo.TilesX;
    u32 TileTop = TileY * TileSize;
    u32 TileLeft = TileX * TileSize;
    u32 TileBottom = TileTop + Min(TileSize, CurrentImage.Height - TileTop);
    u32 TileRight = TileLeft + Min(TileSize, CurrentImage.Width - TileLeft);

    for (u32 y = TileTop; y < TileBottom; ++y) {
        for (u32 x = TileLeft; x < TileRight; ++x) {

            v3 Attenuation = 1.0f;
            v3 OutputColor = 0.0f;

            f32 JitterX = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 JitterY = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 FilmX = -1.0f + ((x + JitterX) * 2.0f) / (f32)CurrentImage.Width;
            f32 FilmY = -1.0f + ((y + JitterY) * 2.0f) / (f32)CurrentImage.Height;

            v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
            v3 RayOrigin = CameraInfo.CameraPosition;
            v3 RayDirection = v3::Normalize(FilmP - RayOrigin);

            u32 MaxRayBounce = 5;
            for (u32 i = 0; i < MaxRayBounce; ++i) {
                f32 A = (RayDirection.y + 1.0f) * 0.5f;
                // Materials[0].Emissive = (1.0f - A) * v3(1.0f) + A * v3(0.5, 0.7, 1.0);
                (void)A;

                v3x HitNormal = 0.0f;
                v3x NextRayOrigin = 0.0f;
                f32x MinT = F32Max;
                u32x MaterialIndex = 0;
                f32x InsideSphere = 0;

                for (u32 s = 0; s < array_len(Spheres); ++s) {
                    const sphere_group &SphereGroup = Spheres[s];
                    v3x SphereCenter = SphereGroup.Positions - RayOrigin;
                    f32x T = v3x::Dot(SphereCenter, RayDirection);
                    v3x ProjectedPoint = v3x(RayDirection) * T;

                    const f32x &Radius = SphereGroup.Radii;
                    const f32x RadiusSquared = Radius * Radius;
                    f32x DistanceFromCenter = v3x::LengthSquared(SphereCenter - ProjectedPoint);

                    f32x HitMask = DistanceFromCenter < RadiusSquared;

                    if (IsZero(HitMask)) continue;

                    f32x X = f32x::SquareRoot(RadiusSquared - DistanceFromCenter);

                    f32x IntersectionT = T - X;
                    f32x IntersectionTest = IntersectionT < F32Epsilon;
                    f32x::ConditionalMove(&IntersectionT, T + X, IntersectionTest);

                    f32x MinMask = (IntersectionT < MinT) & (IntersectionT > F32Epsilon);
                    f32x MoveMask = MinMask & HitMask;
                    if (IsZero(MoveMask)) continue;

                    v3x IntersectionPoint = RayDirection * IntersectionT;
                    v3x Normal = (IntersectionPoint - SphereCenter);
                    f32x::ConditionalMove(&InsideSphere, 1.0f, IntersectionTest & MoveMask);
                    u32x::ConditionalMove(&MaterialIndex, s, u32x(MoveMask));
                    f32x::ConditionalMove(&MinT, IntersectionT, MoveMask);
                    v3x::ConditionalMove(&HitNormal, Normal, MoveMask);
                    v3x::ConditionalMove(&NextRayOrigin, RayOrigin + IntersectionPoint, MoveMask);
                }

                u32 Index = f32x::HorizontalMinIndex(MinT);
                if (MinT[Index] == F32Max) break;

                const u32 AbsoluteIndex = MaterialIndex[Index] * SIMD_WIDTH + Index + 1;
                const material &Material = Materials[AbsoluteIndex];

                OutputColor += Material.Emissive * Attenuation;
                Attenuation *= Material.Color;
                RayOrigin = v3(NextRayOrigin[Index]);

                f32 Specular = Material.Specular;
                v3 Normal = v3::Normalize(v3(HitNormal[Index]));

                v3 PureBounce = RayDirection - 2.0f * v3::Dot(RayDirection, Normal) * Normal;

                bool RayOriginInSphere = InsideSphere[Index] != 0.0f;
                if (RayOriginInSphere) {
                    Normal = -Normal;
                }

                if (Material.IndexOfRefraction == 0.0f) {
                    v3 RandomV3 = v3::NormalizeFast(v3(RandomState.RandomFloat(), RandomState.RandomFloat(), RandomState.RandomFloat()));
                    v3 RandomBounce = Normal + RandomV3;
                    RayDirection = (1.0 - Specular) * RandomBounce + (Specular * PureBounce);
                    RayDirection = v3::Normalize(RayDirection);
                } else {
                    f32 RefractionIndex = (RayOriginInSphere) ? Material.IndexOfRefraction : 1.0f / Material.IndexOfRefraction;

                    f32 CosTheta = Min(v3::Dot(-RayDirection, Normal), 1.0f);
                    f32 SinTheta = SquareRoot(1.0 - CosTheta * CosTheta);

                    bool CantRefract = RefractionIndex * SinTheta > 1.0f;
                    v3 Perpendicular = RefractionIndex * (RayDirection + CosTheta * Normal);
                    v3 Parallel = -SquareRoot(Abs(1.0f - v3::Dot(Perpendicular, Perpendicular))) * Normal;
                    v3 RefractedRay = v3::Normalize(Perpendicular + Parallel);

                    if ((CantRefract || Reflectance(CosTheta, RefractionIndex) > RandomState.RandomFloat(0.0f)) && !RayOriginInSphere) {
                        RayDirection = PureBounce;
                    } else {
                        RayDirection = RefractedRay;
                    }
                }
            }

            u32 TotalRayCount = PreviousRayCount + 1;
            v4 &PreviousColor = GetPixelV4(PreviousImage, x, y);
            v4 OutputColorV4 = v4(OutputColor.x, OutputColor.y, OutputColor.z, 1.0f);
            v4 FinalColor = OutputColorV4 * (1.0f / (f32)TotalRayCount) + PreviousColor * ((f32)PreviousRayCount / (f32)TotalRayCount);
            FinalColor.w = 1.0f;
            PreviousColor = FinalColor;

            u32 &Pixel = GetPixel(CurrentImage, x, y);
            Pixel = ColorFromV4(LinearToSRGB(FinalColor));
        }
    }

}

static void RenderTileScalar(work_queue_context *WorkQueueContext) {
    u32_random_state &RandomState = ThreadContexts[WorkQueueContext->ThreadIndex].RandomState;

    const image &CurrentImage = CameraInfo.CurrentImage;
    const image &PreviousImage = CameraInfo.PreviousImage;
    const v3 &FilmCenter = CameraInfo.FilmCenter;
    const v3 &FilmW = CameraInfo.FilmW;
    const v3 &FilmH = CameraInfo.FilmH;
    const v3 &CameraX = CameraInfo.CameraX;
    const v3 &CameraY = CameraInfo.CameraY;
    // const v3 &CameraZ = CameraInfo.CameraZ;

    u32 Tile = WorkQueueContext->WorkEntry;
    u32 TileX = Tile % CameraInfo.TilesX;
    u32 TileY = Tile / CameraInfo.TilesX;
    u32 TileTop = TileY * TileSize;
    u32 TileLeft = TileX * TileSize;
    u32 TileBottom = TileTop + Min(TileSize, CurrentImage.Height - TileTop);
    u32 TileRight = TileLeft + Min(TileSize, CurrentImage.Width - TileLeft);

    for (u32 y = TileTop; y < TileBottom; ++y) {
        for (u32 x = TileLeft; x < TileRight; ++x) {

            v3 Attenuation = 1.0f;
            v3 OutputColor = 0.0f;

            f32 JitterX = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 JitterY = RandomState.RandomFloat(-0.5f, 0.5f);
            f32 FilmX = -1.0f + ((x + JitterX) * 2.0f) / (f32)CurrentImage.Width;
            f32 FilmY = -1.0f + ((y + JitterY) * 2.0f) / (f32)CurrentImage.Height;

            v3 FilmP = FilmCenter + (FilmX * FilmW * 0.5f * CameraX) + (FilmY * FilmH * 0.5f * CameraY);
            v3 RayOrigin = CameraInfo.CameraPosition;
            v3 RayDirection = v3::Normalize(FilmP - RayOrigin);

            u32 MaxRayBounce = 5;
            for (u32 i = 0; i < MaxRayBounce; ++i) {
                f32 A = (RayDirection.y + 1.0f) * 0.5f;
                // Materials[0].Emissive = (1.0f - A) * v3(1.0f) + A * v3(0.5, 0.7, 1.0);
                (void)A;

                v3 HitNormal = 0.0f;
                v3 NextRayOrigin = 0.0f;
                f32 MinT = F32Max;
                u32 SphereIndex = 0;
                bool RayOriginInSphere = false;

                for (u32 s = 0; s < array_len(ScalarSpheres); ++s) {
                    const scalar_sphere &Sphere = ScalarSpheres[s];
                    v3 SphereCenter = Sphere.Position - RayOrigin;
                    f32 T = v3::Dot(SphereCenter, RayDirection);
                    v3 ProjectedPoint = RayDirection * T;

                    const f32 &Radius = Sphere.Radius;
                    const f32 RadiusSquared = Radius * Radius;
                    f32 DistanceFromCenter = v3::LengthSquared(SphereCenter - ProjectedPoint);

                    if (DistanceFromCenter > RadiusSquared) {
                        continue;
                    }

                    f32 X = SquareRoot(RadiusSquared - DistanceFromCenter);
                    f32 IntersectionT = T - X;
                    bool IntersectionTest = IntersectionT < F32Epsilon;
                    if (IntersectionTest) {
                        IntersectionT = T + X;
                    }

                    if (IntersectionT > MinT) continue;
                    if (IntersectionT < F32Epsilon) continue;

                    v3 IntersectionPoint = RayDirection * IntersectionT;
                    v3 Normal = (IntersectionPoint - SphereCenter);

                    RayOriginInSphere = IntersectionTest;
                    SphereIndex = s;
                    MinT = IntersectionT;
                    HitNormal = Normal;
                    NextRayOrigin = RayOrigin + IntersectionPoint;
                }

                if (MinT == F32Max) break;

                const material &Material = ScalarSpheres[SphereIndex].Material;

                OutputColor += Material.Emissive * Attenuation;
                Attenuation *= Material.Color;
                RayOrigin = NextRayOrigin;

                f32 Specular = Material.Specular;
                v3 Normal = v3::Normalize(HitNormal);

                v3 PureBounce = RayDirection - 2.0f * v3::Dot(RayDirection, Normal) * Normal;

                if (RayOriginInSphere) {
                    Normal = -Normal;
                }

                if (Material.IndexOfRefraction == 0.0f) {
                    v3 RandomV3 = v3::NormalizeFast(v3(RandomState.RandomFloat(), RandomState.RandomFloat(), RandomState.RandomFloat()));
                    v3 RandomBounce = Normal + RandomV3;
                    RayDirection = (1.0 - Specular) * RandomBounce + (Specular * PureBounce);
                    RayDirection = v3::Normalize(RayDirection);
                } else {
                    f32 RefractionIndex = (RayOriginInSphere) ? Material.IndexOfRefraction : 1.0f / Material.IndexOfRefraction;

                    f32 CosTheta = Min(v3::Dot(-RayDirection, Normal), 1.0f);
                    f32 SinTheta = SquareRoot(1.0 - CosTheta * CosTheta);

                    bool CantRefract = RefractionIndex * SinTheta > 1.0f;
                    v3 Perpendicular = RefractionIndex * (RayDirection + CosTheta * Normal);
                    v3 Parallel = -SquareRoot(Abs(1.0f - v3::Dot(Perpendicular, Perpendicular))) * Normal;
                    v3 RefractedRay = v3::Normalize(Perpendicular + Parallel);

                    if ((CantRefract || Reflectance(CosTheta, RefractionIndex) > RandomState.RandomFloat(0.0f)) && !RayOriginInSphere) {
                        RayDirection = PureBounce;
                    } else {
                        RayDirection = RefractedRay;
                    }
                }
            }

            u32 TotalRayCount = PreviousRayCount + 1;
            v4 &PreviousColor = GetPixelV4(PreviousImage, x, y);
            v4 OutputColorV4 = v4(OutputColor.x, OutputColor.y, OutputColor.z, 1.0f);
            v4 FinalColor = OutputColorV4 * (1.0f / (f32)TotalRayCount) + PreviousColor * ((f32)PreviousRayCount / (f32)TotalRayCount);
            FinalColor.w = 1.0f;
            PreviousColor = FinalColor;

            u32 &Pixel = GetPixel(CurrentImage, x, y);
            Pixel = ColorFromV4(LinearToSRGB(FinalColor));
        }
    }
}

static memory_arena RenderData;
static memory_arena ThreadData;

void OnInit(init_params *Params) {
    constexpr string8 WindowTitle = u8"SIMD Ray Tracer";
    Params->WindowTitle = WindowTitle;

    Params->WindowWidth = 1280;
    Params->WindowHeight = 720;

    InitScalarSpheres(ScalarSpheres);
    ConvertScalarSpheresToSIMDSpheres(ScalarSpheres, array_len(ScalarSpheres), Spheres);

    RenderData = AllocateArenaFromOS(MB(512));

#if 1
	WorkQueueCreate(RenderTile);
#else
	WorkQueueCreate(RenderTileScalar);
#endif

    u32 ThreadCount = GetProcessorThreadCount();
	u32 RequiredThreadDataSize = sizeof(thread_context) * ThreadCount;
    ThreadData = AllocateArenaFromOS(RequiredThreadDataSize);
	ThreadContexts = (thread_context *)ThreadData.Push(RequiredThreadDataSize);

    for (u32 i = 0; i < ThreadCount; ++i) {
        u64 InitialSeed = 0x420247153476526ULL * (u64)i;
        InitialSeed += 0x8442885C91A5C8DULL;
        InitialSeed ^= InitialSeed >> ((7 + i) % 64);
        InitialSeed ^= InitialSeed << 23;
        InitialSeed ^= InitialSeed >> ((0x29 ^ i) % 64);
        InitialSeed = (InitialSeed * 0x11C19226CEB4769AULL) + 0x1105404122082911ULL;
        InitialSeed ^= InitialSeed << 19;
        InitialSeed ^= InitialSeed >> 13;
        u32_random_state RandomState = { InitialSeed };
        ThreadContexts[i].RandomState = RandomState;
    }
}


static image CurrentImage = {};
static image PreviousImage = {};

static inline void CopyImage(image DstImage, image SrcImage) {
	Assert(DstImage.Width == SrcImage.Width);
	Assert(DstImage.Height == SrcImage.Height);
	u32 Length = DstImage.Width * DstImage.Height;
	u32 *Dst = (u32 *)DstImage.Data;
	u32 *Src = (u32 *)SrcImage.Data;
	for (u32 i = 0; i < Length; ++i) {
		Dst[i] = Src[i];
	}
}

bool OnRender(const image &Image, render_params RenderParams) {

	if (!WorkQueueIsReady()) {
		return false;
	}

    v3 LookAt = v3(Spheres[3].Positions[1]);

    static f32 DistanceFromLookAt = 2.0f;
    static f32 XAngle = PI32 / 3.0;
    static f32 YHeight = 0.0f;
	static v3 CameraPosition;

    bool Moved = false;
    {
        constexpr f32 MovementSpeed = 0.125f * WorldScale;
        if (IsDown(key::W) || IsDown(key::ArrowUp)) {
            DistanceFromLookAt -= MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::S) || IsDown(key::ArrowDown)) {
            DistanceFromLookAt += MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::D) || IsDown(key::ArrowRight)) {
            XAngle -= MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::A) || IsDown(key::ArrowLeft)) {
            XAngle += MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::Space)) {
            YHeight += MovementSpeed;
            Moved = true;
        }

        bool FlyDown = IsDown(key::C);
#ifndef PLATFORM_WASM
        // Ctrl+W will close the browser window
        FlyDown |= IsDown(key::LeftControl);
#endif
        if (FlyDown) {
            YHeight -= MovementSpeed;
            Moved = true;
        }

        if (XAngle > PI32 * 2.0f) {
            XAngle -= PI32 * 4.0f;
        }
        if (XAngle < -(PI32 * 2.0f)) {
            XAngle += PI32 * 4.0f;
        }
        if (DistanceFromLookAt < 0.5f) {
            DistanceFromLookAt = 0.5f;
        }
        if (YHeight > DistanceFromLookAt) {
            YHeight = DistanceFromLookAt;
        }

        v2 XYPosition = v2(Cosine(XAngle), Sin(XAngle)) * DistanceFromLookAt;
        CameraPosition.x = XYPosition.x;
        CameraPosition.y = YHeight;
        CameraPosition.z = XYPosition.y;
        CameraPosition += LookAt;
    }

	bool WorkQueueComplete = WorkQueueHasCompleted();
	bool Resize = Image.Width != PreviousImage.Width || Image.Height != PreviousImage.Height;
	bool CopyToOutput = WorkQueueComplete && !Resize;
	static f64 StartTime;

	if (CopyToOutput) {
		CopyImage(Image, CurrentImage);
	}

    if (Resize || Moved || IsDown(key::R)) {
		WorkQueueWaitUntilCompletion();
		if (!Resize) {
			CopyImage(Image, CurrentImage);
			CopyToOutput = true;
		}
        PreviousRayCount = 0;
        RenderData.Reset();
        PreviousImage.Width = Image.Width;
        PreviousImage.Height = Image.Height;
        PreviousImage.Data = RenderData.Push(Image.Width * Image.Height * sizeof(v4));
        CurrentImage.Width = Image.Width;
        CurrentImage.Height = Image.Height;
        CurrentImage.Data = RenderData.Push(Image.Width * Image.Height * sizeof(u32));
    } else if (WorkQueueComplete) {
		PreviousRayCount += 1;
	} else {
		return false;
	}

    v3 CameraZ = v3::Normalize(CameraPosition - LookAt);
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

    u32 TilesX = Image.Width / TileSize;
    u32 TilesY = Image.Height / TileSize;
    if (Image.Width & (TileSize - 1)) TilesX += 1;
    if (Image.Height & (TileSize - 1)) TilesY += 1;

    CameraInfo.CameraX = CameraX;
    CameraInfo.CameraY = CameraY;
    CameraInfo.CameraZ = CameraZ;
    CameraInfo.FilmCenter = FilmCenter;
    CameraInfo.FilmW = FilmW;
    CameraInfo.FilmH = FilmH;
    CameraInfo.CurrentImage = CurrentImage;
    CameraInfo.PreviousImage = PreviousImage;
    CameraInfo.CameraPosition = CameraPosition;
    CameraInfo.TilesX = TilesX;

    u32 WorkItemCount = TilesX * TilesY;
    WorkQueueStart(WorkItemCount, RenderParams.ThreadCount);

	return CopyToOutput;
}
