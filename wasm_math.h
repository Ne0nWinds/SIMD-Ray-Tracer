#pragma once
#include "base.h"
#include <wasm_simd128.h>

union v128 {
    f32 Float;
    v2 Vector2;
    v3 Vector3;
    v4 Vector4;
    f32x4 Float4;
    v128_t Register;

    inline v128() { Register = wasm_f32x4_const_splat(0.0f); };
    inline v128(f32 Value) : Float(Value) { };
    inline v128(const v2 &Value) : Vector2(Value) { };
    inline v128(const v3 &Value) : Vector3(Value) { };
    inline v128(const f32x4 &Value) : Float4(Value) { };
    inline v128(const v4 &Value) : Vector4(Value) { };
    inline v128(const v128_t &SIMDLane) : Register(SIMDLane) { };

    explicit operator f32() const { return Float; }
    explicit operator v2() const { return Vector2; }
    explicit operator v3() const { return Vector3; }
    explicit operator v4() const { return Vector4; }
    explicit operator f32x4() const { return Float4; }
    operator v128_t() const { return Register; }

    MATHCALL v128 CreateMask(bool Value) {
        v128 Zero = v128();
        v128 Result = (Value) ? v128(wasm_v128_not(Zero)) : Zero;
        return Result;
    }
};

static inline f32 Sqrt(f32 Value) {
    return __builtin_sqrt(Value);
}
static inline f32 Max(f32 A, f32 B) {
    return __builtin_wasm_max_f32(A, B);
}
static inline f32 Min(f32 A, f32 B) {
    return __builtin_wasm_min_f32(A, B);
}
MATHCALL f32 Negate(f32 Value) {
    return Value * -1.0f; // f32.neg
}
static inline f32 Sign(f32 Value) {
    f32 Result = __builtin_copysign(1.0f, Value);
    return Result;
}
MATHCALL f32 Reciprocal(f32 Value) {
    return 1.0f / Value;
}
MATHCALL f32 Saturate(f32 Value) {
    if (Value < 0.0f) return 0.0f;
    if (Value > 1.0f) return 1.0f;
    return Value;
}
MATHCALL f32 FMA(f32 A, f32 B, f32 C) {
    return 0.0f;
}

inline v2::v2(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v2)Value;
}
inline v3::v3(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v3)Value;
}
inline v4::v4(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v4)Value;
}
inline v2::v2(f32 X, f32 Y) {
    v128 Value = wasm_f32x4_make(X, Y, 0.0f, 0.0f);
    *this = (v2)Value;
}
inline v3::v3(f32 X, f32 Y, f32 Z) {
    v128 Value = wasm_f32x4_make(X, Y, Z, 0.0f);
    *this = (v3)Value;
}
inline v4::v4(f32 X, f32 Y, f32 Z, f32 W) {
    v128 Value = wasm_f32x4_make(X, Y, Z, W);
    *this = (v4)Value;
}

MATHCALL u32 PopCount(u32 a) {
    return __builtin_popcount(a);
}
MATHCALL u64 PopCount(u64 a) {
    return __builtin_popcount(a);
}

inline f32 v3::Dot(const v3 &A, const v3 &B) {
    v3 Mul = A * B;
    return Mul.x + Mul.y + Mul.z;
}
inline f32 v3::LengthSquared(const v3 &Value) {
    return v3::Dot(Value, Value);
}
inline f32 v3::Length(const v3 &Value) {
    return Sqrt(v3::LengthSquared(Value));
}
inline v3 v3::Normalize(const v3 &Value) {
    f32 LengthSquared = v3::LengthSquared(Value);

    bool LengthGreaterThanZero = LengthSquared > F32Epsilon;
    v128 Mask = v128::CreateMask(LengthGreaterThanZero);

    f32 Length = Sqrt(LengthSquared);
    v3 Result = Value / Length;

    v128 MaskedResult = wasm_v128_and(v128(Result), Mask);
    return (v3)MaskedResult;
}
MATHCALL v3 operator+(const v3 &A, const v3 &B) {
    v128 Result = wasm_f32x4_add(v128(A), v128(B));
    return (v3)Result;
}
MATHCALL v3 operator-(const v3 &A, const v3 &B) {
    v128 Result = wasm_f32x4_sub(v128(A), v128(B));
    return (v3)Result;
}
MATHCALL v3 operator*(const v3 &A, const v3 &B) {
    v128 Result = wasm_f32x4_mul(v128(A), v128(B));
    return (v3)Result;
}
MATHCALL v3 operator/(const v3 &A, const v3 &B) {
    v128 Result = wasm_f32x4_div(v128(A), v128(B));
    return (v3)Result;
}

inline v3 v3::Cross(const v3 &A, const v3 &B) {
    v3 Result;
    Result.x = A.y * B.z - A.z * B.y;
    Result.y = A.z * B.x - A.x * B.z;
    Result.z = A.x * B.y - A.y * B.x;
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

inline f32x v3x::Dot(const v3x &A, const v3x &B) {
    v3x C = A * B;
    f32x Result = C.x + C.y + C.z;
    return Result;
}
inline f32x v3x::Length(const v3x &A) {
    f32x LengthSquared = v3x::Dot(A, A);
    f32x Length = f32x::SquareRoot(LengthSquared);
    return Length;
}
static inline f32x LengthSquared(const v3x &A) {
    v3x C = A * A;
    f32x Result = C.x + C.y + C.z;
    return Result;
}
inline f32x v3x::LengthSquared(const v3x &A) {
    v3x C = A * A;
    f32x Result = C.x + C.y + C.z;
    return Result;
}
inline v3x v3x::Normalize(const v3x &Value) {
    f32x LengthSquared = v3x::LengthSquared(Value);

    f32x LengthGreaterThanZeroMask = LengthSquared > F32Epsilon;

    f32x Length = f32x::SquareRoot(LengthSquared);
    v3x Result = Value * f32x::Reciprocal(Length);

    v3x MaskedResult = Result & LengthGreaterThanZeroMask;
    return MaskedResult;
}

MATHCALL f32x4 operator+(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_add(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator-(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_sub(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator*(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_mul(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator/(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_div(v128(A), v128(B));
    return (f32x4)Result;
}
inline f32x4 f32x4::SquareRoot(const f32x4 &A) {
    v128 Result = wasm_f32x4_sqrt(v128(A));
    return (f32x4)Result;
}
inline f32x4 f32x4::Min(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_min(v128(A), v128(B));
    return (f32x4)Result;
}
inline f32x4 f32x4::Max(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_max(v128(A), v128(B));
    return (f32x4)Result;
}
inline f32x4 f32x4::Reciprocal(const f32x4 &A) {
    v128 One = wasm_f32x4_const_splat(1.0f);
    v128 Result = wasm_f32x4_div(v128(A), One);
    return (f32x4)Result;
}
MATHCALL f32x4 operator>(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_gt(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator<(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_lt(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator&(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_v128_and(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator|(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_v128_or(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator^(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_v128_xor(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL bool IsZero(const f32x4 &Value) {
    v128 Zero = wasm_f32x4_const_splat(0.0f);
    v128 ComparisonResult = wasm_f32x4_ne(v128(Value), Zero);
    bool Result = wasm_v128_any_true(ComparisonResult);
    return Result == 0;
}