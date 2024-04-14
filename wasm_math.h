#pragma once
#include "base.h"
#include <wasm_simd128.h>

#define MATHCALL static inline constexpr

union v128 {
    f32 Float;
    v2 Vector2;
    v3 Vector3;
    v4 Vector4;
    v128_t Register;

    constexpr v128(f32 Value) : Float(Value) { };
    constexpr v128(const v4 &Value) : Vector4(Value) { };
    constexpr v128(const v128_t &SIMDLane) : Register(SIMDLane) { };

    explicit operator f32() const { return Float; }
    explicit operator v2() const { return Vector2; }
    explicit operator v3() const { return Vector3; }
    explicit operator v4() const { return Vector4; }
    operator v128_t() const { return Register; }
};

constexpr inline v2::v2(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v2)Value;
}
constexpr inline v3::v3(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v3)Value;
}
constexpr inline v4::v4(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v4)Value;
}
constexpr inline v2::v2(f32 X, f32 Y) {
    v128 Value = wasm_f32x4_make(X, Y, 0.0f, 0.0f);
    *this = (v2)Value;
}
constexpr inline v3::v3(f32 X, f32 Y, f32 Z) {
    v128 Value = wasm_f32x4_make(X, Y, Z, 0.0f);
    *this = (v3)Value;
}
constexpr inline v4::v4(f32 X, f32 Y, f32 Z, f32 W) {
    v128 Value = wasm_f32x4_make(X, Y, Z, W);
    *this = (v4)Value;
}

MATHCALL u32 PopCount(u32 a) {
    return __builtin_popcount(a);
}
MATHCALL u64 PopCount(u64 a) {
    return __builtin_popcount(a);
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