
#include "base.h"

static u32 PreviousRayCount = 0;
static constexpr u32 TileSize = 64;

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

template<typename T>
struct array {
	T *Data;
	u32 Count;
	inline constexpr T &operator[](u32 Index) {
		Assert(Index < Count);
		return Data[Index];
	}
	inline constexpr const T &operator[](u32 Index) const {
		Assert(Index < Count);
		return Data[Index];
	}
};

struct scene {
	v3 LookAt;
	bool UseSkyColor;
    f32 DefaultDistanceFromLookAt;
    f32 DefaultXAngle;
    f32 DefaultYHeight;
	array<scalar_sphere> ScalarSpheres;
	array<sphere_group> SIMDSpheres;
	array<material> Materials;
};

static u32 SceneIndex = -1;
static scene Scenes[3] = { };

static constexpr f32 WorldScale = 1.0f;
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

static void constexpr ConvertScalarSpheresToSIMDSpheres(scene *Scene) {
    for (u32 i = 0; i < Scene->ScalarSpheres.Count; i += SIMD_WIDTH) {
        sphere_group &SphereGroup = Scene->SIMDSpheres[i / SIMD_WIDTH];
		const u32 RemainingIterations = Scene->ScalarSpheres.Count - i;
		u32 InnerLoopCount =  RemainingIterations > SIMD_WIDTH ? SIMD_WIDTH : RemainingIterations;
        for (u32 j = 0; j < InnerLoopCount; ++j) {
            f32 R = Scene->ScalarSpheres[i + j].Radius;
            SphereGroup.Radii[j] = R;
        }
        for (u32 j = 0; j < InnerLoopCount; ++j) {
            const v3 &Position = Scene->ScalarSpheres[i + j].Position;
            SphereGroup.Positions[j] = Position;
        }
        for (u32 j = 0; j < InnerLoopCount; ++j) {
            scalar_sphere Sphere = Scene->ScalarSpheres[i + j];
            Scene->Materials[i + j] = Sphere.Material;
        }
    }
}

static scalar_sphere RandomizedScalarSpheres[256];
static sphere_group RandomizedSIMDSpheres[(array_len(RandomizedScalarSpheres) + (SIMD_WIDTH - 1)) / SIMD_WIDTH];
static material RandomizedMaterials[array_len(RandomizedScalarSpheres) + 1];
static inline void InitRandomizedSphereScene(scene *Scene) {
	Scene->ScalarSpheres.Data = RandomizedScalarSpheres;
	Scene->ScalarSpheres.Count = array_len(RandomizedScalarSpheres);
	Scene->SIMDSpheres.Data = RandomizedSIMDSpheres;
	Scene->SIMDSpheres.Count = array_len(RandomizedSIMDSpheres);
	Scene->Materials.Data = RandomizedMaterials;
	Scene->Materials.Count = array_len(RandomizedMaterials);

    Scene->DefaultDistanceFromLookAt = 32.0f;

    u32_random_state RandomState = { 0x29D7A0A514F22432LLU };
    constexpr u32 Length = array_len(RandomizedScalarSpheres);

	material Materials[28];
	for (u32 i = 0; i < array_len(Materials); ++i) {
        v3 Color = 0.0f;
        Color.x = RandomState.RandomFloat(0.15f, 1.0f);
        Color.y = RandomState.RandomFloat(0.1f, 0.75f);
        Color.z = RandomState.RandomFloat(0.15f, 1.0f);

        v3 Emissive = 0.0f;
        f32 Specular = 0.0f;
        if (RandomState.RandomFloat(0.0f) < 0.125f) {
            Emissive = RandomState.RandomFloat(2.0f, 5.0f) * Color;
        } else {
            f32 Random = RandomState.RandomFloat(0.0f);
            if (Random < 0.65f) {
                Specular = 1.0f;
            }
        }
		Materials[i].Color = Color;
		Materials[i].Emissive = Emissive;
		Materials[i].IndexOfRefraction = 0.0f;
		Materials[i].Specular = Specular;
	}
	
	f32 Radius = RandomState.RandomFloat(2.0f, 8.0f);
	const material &M = Materials[0];
	CreateScalarSphere(v3(1.0, 0.0, 0.0), Radius, M.Color, M.Specular, M.IndexOfRefraction, M.Emissive, RandomizedScalarSpheres + 0);
	CreateScalarSphere(v3(8.0, -1.0, 8.0), Radius, M.Color, M.Specular, M.IndexOfRefraction, M.Emissive, RandomizedScalarSpheres + 1);
	CreateScalarSphere(v3(-20.0, -4.0, -20.0), Radius, M.Color, M.Specular, M.IndexOfRefraction, M.Emissive, RandomizedScalarSpheres + 2);

    for (u32 i = 3; i < Length; ++i) {
        v3 Vector = 0.0f;
        Vector.x = RandomState.RandomFloat();
        Vector.y = RandomState.RandomFloat();
        Vector.z = RandomState.RandomFloat();
		v3 NormalizedVector = v3::Normalize(Vector);

		u32 RandomIndex = i - 3;
		f32 R = RandomizedScalarSpheres[RandomIndex].Radius;
		v3 P = RandomizedScalarSpheres[RandomIndex].Position;

		f32 Radius = RandomState.RandomFloat(1.0f, 4.0f);
		v3 Position = P + NormalizedVector * (RandomState.RandomFloat(1.0, 8.0) + Radius + R);

		const material &M = Materials[i % array_len(Materials)];
        CreateScalarSphere(Position, Radius, M.Color, M.Specular, M.IndexOfRefraction, M.Emissive, RandomizedScalarSpheres + i);
    }

	ConvertScalarSpheresToSIMDSpheres(Scene);

	Scene->LookAt = v3(2.0, 0.0, 2.0);
}
static scalar_sphere RGBScalarSpheres[5];
static sphere_group RGBSIMDSpheres[(array_len(RGBScalarSpheres) + (SIMD_WIDTH - 1)) / SIMD_WIDTH];
static material RGBMaterials[array_len(RGBScalarSpheres) + 1];
static inline void InitRGBSphereScene(scene *Scene) {
	Scene->ScalarSpheres.Data = RGBScalarSpheres;
	Scene->ScalarSpheres.Count = array_len(RGBScalarSpheres);
	Scene->SIMDSpheres.Data = RGBSIMDSpheres;
	Scene->SIMDSpheres.Count = array_len(RGBSIMDSpheres);
	Scene->Materials.Data = RGBMaterials;
	Scene->Materials.Count = array_len(RGBMaterials);
    Scene->DefaultDistanceFromLookAt = 13.0f;
    Scene->DefaultXAngle = PI32 / 3.0;
    Scene->DefaultYHeight = 2.0f;

    CreateScalarSphere(v3(0.0f, -256 - 2.0f, -15.0f), 256.0f, v3(0.2f), 0.0f, 0.0f, 0.0f, RGBScalarSpheres + 0);
    CreateScalarSphere(v3(0.0f, 0, -10.0f), 2.0f, v3(1.0f), 0.0f, 1.5f, 0.0f, RGBScalarSpheres + 1);
    CreateScalarSphere(v3(-4.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(8.0f, 0.0f, 0.0f), RGBScalarSpheres + 2);
    CreateScalarSphere(v3(0.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(0.0f, 8.0f, 0.0f), RGBScalarSpheres + 3);
    CreateScalarSphere(v3(4.0f, 1.0f, -15.0f), 1.5f, v3(1.0f, 0.0f, 0.0f), 0.0f, 0, v3(0.0f, 0.0f, 8.0f), RGBScalarSpheres + 4);

	ConvertScalarSpheresToSIMDSpheres(Scene);

	Scene->LookAt = RGBScalarSpheres[1].Position;
}

static scalar_sphere RTWeekendSpheres[482];
static sphere_group RTWeekendSIMDSpheres[(array_len(RTWeekendSpheres) + (SIMD_WIDTH - 1)) / SIMD_WIDTH];
static material RTWeekendMaterials[array_len(RTWeekendSpheres) + 1];
static inline void InitRTWeekendSphereScene(scene *Scene) {
	Scene->ScalarSpheres.Data = RTWeekendSpheres;
	Scene->ScalarSpheres.Count = array_len(RTWeekendSpheres);
	Scene->SIMDSpheres.Data = RTWeekendSIMDSpheres;
	Scene->SIMDSpheres.Count = array_len(RTWeekendSIMDSpheres);
	Scene->Materials.Data = RTWeekendMaterials;
	Scene->Materials.Count = array_len(RTWeekendMaterials);
	Scene->UseSkyColor = true;

    Scene->DefaultDistanceFromLookAt = 12.0f;
    Scene->DefaultXAngle = PI32 / 8;
    Scene->DefaultYHeight = 2.0f;

	u32 Index = 0;
	CreateScalarSphere(v3(0, -1000, 0), 1000, v3(0.5), 0.0, 0.0, v3(0.0), RTWeekendSpheres + Index);
	Index += 1;
	CreateScalarSphere(v3(0, 1, 0), 1, v3(1.0), 0.0, 1.5, v3(0.0), RTWeekendSpheres + Index);
	Index += 1;
	CreateScalarSphere(v3(-4, 1, 0), 1, v3(0.4, 0.2, 0.1), 0.0, 0.0, v3(0.0), RTWeekendSpheres + Index);
	Index += 1;
	CreateScalarSphere(v3(4, 1, 0), 1, v3(0.7, 0.6, 0.5), 1.0, 0.0, v3(0.0), RTWeekendSpheres + Index);
	Index += 1;

    u32_random_state RandomState = { 0xCD46749A57ACB371 };

	for (s32 i = -11; i < 11; ++i) {
		for (s32 j = -11; j < 11; ++j) {
			f32 M = RandomState.RandomFloat(0.0, 1.0);
			v3 Center;
			Center.x = i + RandomState.RandomFloat();
			Center.y = 0.2;
			Center.z = j + RandomState.RandomFloat();
			if (v3::Length(Center - v3(4, 0.2, 0)) > 0.9) {
				v3 Color = 0.0f;
				v3 Emissive = 0.0f;
				f32 Radius = 0.2f;
				f32 Specular = 0.0f;
				f32 IndexOfRefraction = 0.0f;

				if (M < 0.8) {
					Color = v3(
						RandomState.RandomFloat(0.0, 1.0),
						RandomState.RandomFloat(0.0, 1.0),
						RandomState.RandomFloat(0.0, 1.0)
					);
				} else if (M < 0.95) {
					Color = v3(
						RandomState.RandomFloat(0.0, 1.0),
						RandomState.RandomFloat(0.0, 1.0),
						RandomState.RandomFloat(0.0, 1.0)
					);
					Specular = RandomState.RandomFloat(0.5, 1.0);
				} else {
					Color = 1.0f;
					IndexOfRefraction = 1.5f;
				}
				CreateScalarSphere(Center, Radius, Color, Specular, IndexOfRefraction, Emissive, RTWeekendSpheres + Index);
				Index += 1;
			}
		}
	}

	Scene->LookAt = RTWeekendSpheres[1].Position;
	ConvertScalarSpheresToSIMDSpheres(Scene);
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

	const scene Scene = Scenes[SceneIndex];

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
                v3x HitNormal = 0.0f;
                v3x NextRayOrigin = 0.0f;
                f32x MinT = F32Max;
                u32x MaterialIndex = 0;
                f32x InsideSphere = 0;

#pragma clang loop unroll_count(2)
                for (u32 s = 0; s < Scene.SIMDSpheres.Count; ++s) {
                    const sphere_group SphereGroup = Scene.SIMDSpheres[s];
                    v3x SphereCenter = SphereGroup.Positions - RayOrigin;
                    f32x T = v3x::Dot(SphereCenter, RayDirection);
                    v3x ProjectedPoint = v3x(RayDirection) * T;

                    const f32x Radius = SphereGroup.Radii;
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
                    f32x::ConditionalMove(&InsideSphere, f32x(u32x4(-1)), IntersectionTest & MoveMask);
                    u32x::ConditionalMove(&MaterialIndex, s, u32x(MoveMask));
                    f32x::ConditionalMove(&MinT, IntersectionT, MoveMask);
                    v3x::ConditionalMove(&HitNormal, Normal, MoveMask);
                    v3x::ConditionalMove(&NextRayOrigin, RayOrigin + IntersectionPoint, MoveMask);
                }

                u32 Index = f32x::HorizontalMinIndex(MinT);
                if (MinT[Index] == F32Max) {
					if (Scene.UseSkyColor) {
						f32 A = (RayDirection.y + 1.0f) * 0.5f;
						v3 SkyEmissive = (1.0f - A) * v3(1.0f) + A * v3(0.5, 0.7, 1.0);
						OutputColor += SkyEmissive * Attenuation;
					}
					break;
				}

                const u32 AbsoluteIndex = MaterialIndex[Index] * SIMD_WIDTH + Index;
                const material Material = Scene.Materials[AbsoluteIndex];

                OutputColor += Material.Emissive * Attenuation;
                Attenuation *= Material.Color;
                RayOrigin = v3(NextRayOrigin.x[Index], NextRayOrigin.y[Index], NextRayOrigin.z[Index]);

                f32 Specular = Material.Specular;
                v3 Normal = v3::Normalize(v3(HitNormal.x[Index], HitNormal.y[Index], HitNormal.z[Index]));

                v3 PureBounce = RayDirection - 2.0f * v3::Dot(RayDirection, Normal) * Normal;

                bool RayOriginInSphere = InsideSphere[Index] != 0;
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

	const scene Scene = Scenes[SceneIndex];

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
                v3 HitNormal = 0.0f;
                v3 NextRayOrigin = 0.0f;
                f32 MinT = F32Max;
                u32 SphereIndex = 0;
                bool RayOriginInSphere = false;

                for (u32 s = 0; s < Scene.ScalarSpheres.Count; ++s) {
                    const scalar_sphere Sphere = Scene.ScalarSpheres[s];
                    v3 SphereCenter = Sphere.Position - RayOrigin;
                    f32 T = v3::Dot(SphereCenter, RayDirection);
                    v3 ProjectedPoint = RayDirection * T;

                    const f32 Radius = Sphere.Radius;
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

                if (MinT == F32Max) {
					if (Scene.UseSkyColor) {
						f32 A = (RayDirection.y + 1.0f) * 0.5f;
						v3 SkyEmissive = (1.0f - A) * v3(1.0f) + A * v3(0.5, 0.7, 1.0);
						OutputColor += SkyEmissive * Attenuation;
					}
					break;
				}

                const material &Material = Scene.ScalarSpheres[SphereIndex].Material;

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

	InitRGBSphereScene(Scenes + 0);
	InitRandomizedSphereScene(Scenes + 1);
	InitRTWeekendSphereScene(Scenes + 2);
    // InitScalarSpheres(ScalarSpheres);
    // ConvertScalarSpheresToSIMDSpheres(ScalarSpheres, array_len(ScalarSpheres), Spheres);

    RenderData = AllocateArenaFromOS(MB(512));

	WorkQueueCreate();

    u32 ThreadCount = GetProcessorThreadCount();
	u32 RequiredThreadDataSize = sizeof(thread_context) * ThreadCount;
    ThreadData = AllocateArenaFromOS(RequiredThreadDataSize + 64);
	ThreadContexts = (thread_context *)ThreadData.Push(RequiredThreadDataSize, 64);

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


    static f32 DistanceFromLookAt;
    static f32 XAngle;
    static f32 YHeight;
	v3 CameraPosition;

    bool Moved = false;
	if (SceneIndex != RenderParams.SceneIndex) {
		if (!WorkQueueHasCompleted()) {
			return false;
		}
		SceneIndex = RenderParams.SceneIndex;
		DistanceFromLookAt = Scenes[SceneIndex].DefaultDistanceFromLookAt;
		XAngle = Scenes[SceneIndex].DefaultXAngle;
		YHeight = Scenes[SceneIndex].DefaultYHeight;
		Moved = true;
	}
    const v3 LookAt = Scenes[SceneIndex].LookAt;

    {
        constexpr f32 MovementSpeed = 1.0f;
        if (IsDown(key::W) || IsDown(key::ArrowUp)) {
            DistanceFromLookAt -= MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::S) || IsDown(key::ArrowDown)) {
            DistanceFromLookAt += MovementSpeed;
            Moved = true;
        }
        if (IsDown(key::D) || IsDown(key::ArrowRight)) {
            XAngle -= MovementSpeed / 16.0f;
            Moved = true;
        }
        if (IsDown(key::A) || IsDown(key::ArrowLeft)) {
            XAngle += MovementSpeed / 16.0f;
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
    WorkQueueStart(
		(RenderParams.EnableSIMD) ? RenderTile : RenderTileScalar,
		WorkItemCount,
		RenderParams.ThreadCount
	);

	return CopyToOutput;
}
