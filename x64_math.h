#pragma once

#include <immintrin.h>

#define MATHCALL static inline constexpr

struct alignas(8) v2 {
    f32 x = 0.0f, y = 0.0f;
    constexpr v2(f32 X);
    constexpr v2(f32 X, f32 Y);
};
struct alignas(16) v3 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    constexpr v3(f32 X);
    constexpr v3(f32 X, f32 Y, f32 Z);
};
struct alignas(16) v4 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    constexpr v4(f32 X);
    constexpr v4(f32 X, f32 Y, f32 Z, f32 W);
};

union xmm {
    f32 Float;
    v2 Vector2;
    v3 Vector3;
    v4 Vector4;
    __m128 Register;

    constexpr xmm(f32 Value) : Float(Value) { };
    constexpr xmm(const v4 &Value) : Vector4(Value) { };
    constexpr xmm(const __m128 &XMM) : Register(XMM) { };

    explicit operator f32() const { return Float; }
    explicit operator v2() const { return Vector2; }
    explicit operator v3() const { return Vector3; }
    explicit operator v4() const { return Vector4; }
    operator __m128() const { return Register; }
};

constexpr inline v2::v2(f32 X) {
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v2)xmm0;
}
constexpr inline v3::v3(f32 X) {
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v3)xmm0;
}
constexpr inline v4::v4(f32 X) {
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v4)xmm0;
}
constexpr inline v2::v2(f32 X, f32 Y) {
    this->x = X;
    this->y = Y;
}
constexpr inline v3::v3(f32 X, f32 Y, f32 Z) {
    this->x = X;
    this->y = Y;
    this->z = Z;
}
constexpr inline v4::v4(f32 X, f32 Y, f32 Z, f32 W) {
    this->x = X;
    this->y = Y;
    this->z = Z;
    this->w = W;
}


MATHCALL u32 PopCount(u32 a) {
	u32 Result = _mm_popcnt_u32(a);
	return Result;
}
MATHCALL u64 PopCount(u64 a) {
	u64 Result = _mm_popcnt_u64(a);
	return Result;
}

MATHCALL u32 RoundUpPowerOf2(u32 Value, u32 Power2) {
    Assert(PopCount(Power2) == 1);
    u32 Result = Value;
    u32 Mask = Power2 - 1;
    Result += Mask;
    Result &= ~Mask;
    return Result;
}

MATHCALL u64 RoundUpPowerOf2(u64 Value, u64 Power2) {
    Assert(PopCount(Power2) == 1);
    u64 Result = Value;
    u64 Mask = Power2 - 1;
    Result += Mask;
    Result &= ~Mask;
    return Result;
}