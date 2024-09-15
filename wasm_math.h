#pragma once
#include "base.h"
#include <wasm_simd128.h>

union v128 {
    f32 Float;
    v2 Vector2;
    v3 Vector3;
    v4 Vector4;
    f32x4 Float4;
    u32x4 Uint4;
    v128_t Register;

    inline v128() { Register = wasm_f32x4_const_splat(0.0f); };
    inline v128(f32 Value) : Float(Value) { };
    inline v128(const v2 &Value) : Vector2(Value) { };
    inline v128(const v3 &Value) : Vector3(Value) { };
    inline v128(const f32x4 &Value) : Float4(Value) { };
    inline v128(const u32x4 &Value) : Uint4(Value) { };
    inline v128(const v4 &Value) : Vector4(Value) { };
    inline v128(const v128_t &SIMDLane) : Register(SIMDLane) { };

    explicit operator f32() const { return Float; }
    explicit operator v2() const { return Vector2; }
    explicit operator v3() const { return Vector3; }
    explicit operator v4() const { return Vector4; }
    explicit operator f32x4() const { return Float4; }
    explicit operator u32x4() const { return Uint4; }
    operator v128_t() const { return Register; }

    MATHCALL v128 CreateMask(bool Value) {
        v128 Result = wasm_u32x4_splat((u32)Value * -1);
        return Result;
    }
};

inline f32x4::f32x4(const u32x4 &V) {
    v128 Result = wasm_f32x4_make(V[0], V[1], V[2], V[3]); // wasm_convert ?
    *this = (f32x4)Result;
}
static inline f32 SquareRoot(f32 Value) {
    return __builtin_sqrt(Value);
}
MATHCALL f32 Abs(f32 Value) {
    if (Value < 0.0f) return -Value;
    return Value;
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
    return A*B + C;
}

inline v2::v2(f32 X) {
    v128 Value = wasm_f32x4_splat(X);
    *this = (v2)Value;
}
inline v3::v3(const f32 &X) {
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
inline v3::v3(const f32 &X, const f32 &Y, const f32 &Z) {
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

MATHCALL u32 RotateRight32(u32 Value, s32 Rotation) {
    return __builtin_rotateright32(Value, Rotation);
}
MATHCALL u64 RotateRight64(u64 Value, s32 Rotation) {
    return __builtin_rotateright64(Value, Rotation);
}
MATHCALL u32 RotateLeft32(u32 Value, s32 Rotation) {
    return __builtin_rotateleft32(Value, Rotation);
}
MATHCALL u64 RotateLeft64(u64 Value, s32 Rotation) {
    return __builtin_rotateleft64(Value, Rotation);
}

MATHCALL v2 operator+(const v2 &A, const v2 &B) {
    v128 Result = wasm_f32x4_add(v128(A), v128(B));
    return (v2)Result;
}
MATHCALL v2 operator-(const v2 &A, const v2 &B) {
    v128 Result = wasm_f32x4_sub(v128(A), v128(B));
    return (v2)Result;
}
MATHCALL v2 operator*(const v2 &A, const v2 &B) {
    v128 Result = wasm_f32x4_mul(v128(A), v128(B));
    return (v2)Result;
}
MATHCALL v2 operator/(const v2 &A, const v2 &B) {
    v128 Result = wasm_f32x4_div(v128(A), v128(B));
    return (v2)Result;
}
inline f32 v3::Dot(const v3 &A, const v3 &B) {
    v3 Mul = A * B;
    return Mul.x + Mul.y + Mul.z;
}
inline f32 v3::LengthSquared(const v3 &Value) {
    return v3::Dot(Value, Value);
}
inline f32 v3::Length(const v3 &Value) {
    return SquareRoot(v3::LengthSquared(Value));
}
inline v3 v3::Normalize(const v3 &Value) {
    f32 LengthSquared = v3::LengthSquared(Value);

    bool LengthGreaterThanZero = LengthSquared > F32Epsilon;
    v128 Mask = v128::CreateMask(LengthGreaterThanZero);

    f32 Length = SquareRoot(LengthSquared);
    v3 Result = Value / Length;

    v128 MaskedResult = wasm_v128_and(v128(Result), Mask);
    return (v3)MaskedResult;
}
inline v3 v3::NormalizeFast(const v3 &Value) {
    return v3::Normalize(Value);
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
MATHCALL v3 operator-(const v3 &A) {
    v128 Result = wasm_f32x4_neg(v128(A));
    return (v3)Result;
}

MATHCALL v4 operator+(const v4 &A, const v4 &B) {
    v128 Result = wasm_f32x4_add(v128(A), v128(B));
    return (v4)Result;
}
MATHCALL v4 operator-(const v4 &A, const v4 &B) {
    v128 Result = wasm_f32x4_sub(v128(A), v128(B));
    return (v4)Result;
}
MATHCALL v4 operator*(const v4 &A, const v4 &B) {
    v128 Result = wasm_f32x4_mul(v128(A), v128(B));
    return (v4)Result;
}
MATHCALL v4 operator/(const v4 &A, const v4 &B) {
    v128 Result = wasm_f32x4_div(v128(A), v128(B));
    return (v4)Result;
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
    v3x Result = Value / Length;

    v3x MaskedResult = Result & LengthGreaterThanZeroMask;
    return MaskedResult;
}
inline v3x v3x::NormalizeFast(const v3x &Value) {
    return v3x::Normalize(Value);
}

inline void v3x::ConditionalMove(v3x *A, const v3x &B, const f32x &MoveMask) {
    f32x::ConditionalMove(&A->x, B.x, MoveMask);
    f32x::ConditionalMove(&A->y, B.y, MoveMask);
    f32x::ConditionalMove(&A->z, B.z, MoveMask);
}

inline f32 f32x4::HorizontalMin(const f32x4 &Value) {
    v128 UpperHalf = wasm_f32x4_make(Value[2], Value[3], 0, 0);
    v128 Result = wasm_f32x4_min(v128(Value), UpperHalf);
    Result = wasm_f32x4_min(Result, wasm_f32x4_make(Result.Float4[1], 0, 0, 0));
    return (f32)Result;
}
inline u32 f32x4::HorizontalMinIndex(const f32x4 &Value) {
    f32 MinValue = f32x4::HorizontalMin(Value);
    v128 Comparison = Value == f32x4(MinValue);
    u32 MoveMask = wasm_i32x4_bitmask(Comparison);
    u32 Result = __builtin_ctz(MoveMask);
    return Result;
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
    return f32x4(1.0f) / A;
}
inline void f32x4::ConditionalMove(f32x4 *A, const f32x4 &B, const f32x4 &MoveMask) {
    v128 Result = wasm_v128_bitselect(v128(B), v128(*A), v128(MoveMask));
    *A = (f32x4)Result;
}
MATHCALL f32x4 operator==(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_eq(v128(A), v128(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator!=(const f32x4 &A, const f32x4 &B) {
    v128 Result = wasm_f32x4_ne(v128(A), v128(B));
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

MATHCALL u32x4 operator+(const u32x4 &A, const u32x4 &B) {
	v128 Result = wasm_i32x4_add(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator-(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_i32x4_sub(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator*(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_i32x4_mul(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator==(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_i32x4_eq(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator!=(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_i32x4_ne(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator>(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_u32x4_gt(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator<(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_u32x4_lt(v128(A), v128(B));
    return (u32x4)Result;
}

MATHCALL u32x4 operator&(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_v128_and(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator|(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_v128_or(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator^(const u32x4 &A, const u32x4 &B) {
    v128 Result = wasm_v128_xor(v128(A), v128(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator~(const u32x4 &A) {
    v128 Result = wasm_i32x4_neg(v128(A));
    return (u32x4)Result;
}
MATHCALL u32x4 operator>>(const u32x4 &A, const u32x4 &B) {
	u32x4 Result;
	Result[0] = A[0] >> B[0];
	Result[1] = A[1] >> B[1];
	Result[2] = A[2] >> B[2];
	Result[3] = A[3] >> B[3];
    return (u32x4)Result;
}
MATHCALL u32x4 operator<<(const u32x4 &A, const u32x4 &B) {
	u32x4 Result;
	Result[0] = A[0] >> B[0];
	Result[1] = A[1] >> B[1];
	Result[2] = A[2] >> B[2];
	Result[3] = A[3] >> B[3];
    return (u32x4)Result;
}

MATHCALL u32x4 operator>>(const u32x4 &A, const u32 &&B) {
    v128 Result = wasm_u32x4_shr(v128(A), B);
    return (u32x4)Result;
}
MATHCALL u32x4 operator<<(const u32x4 &A, const u32 &&B) {
    v128 Result = wasm_i32x4_shr(v128(A), B);
    return (u32x4)Result;
}

inline void u32x4::ConditionalMove(u32x4 *A, const u32x4 &B, const u32x4 &MoveMask) {
    v128 Result = wasm_v128_bitselect(v128(B), v128(*A), v128(MoveMask));
    *A = (u32x4)Result;
}

#include <math.h>

static inline f32 Cosine(f32 radians) {
	return cosf(radians);
}

static inline f32 Sin(f32 radians) {
	return sinf(radians);
}
