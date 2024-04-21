#pragma once

#include <immintrin.h>

union xmm {
    f32 Float;
    v2 Vector2;
    v3 Vector3;
    v4 Vector4;
    __m128 Register;

    inline xmm() : Register() { Register = _mm_xor_ps(Register, Register); }
    inline xmm(f32 Value) : Float(Value) { };
    inline xmm(v2 Value) : Vector2(Value) { };
    inline xmm(v3 Value) : Vector3(Value) { };
    inline xmm(const v4 &Value) : Vector4(Value) { };
    inline xmm(const __m128 &XMM) : Register(XMM) { };

    explicit operator f32() const { return Float; }
    explicit operator v2() const { return Vector2; }
    explicit operator v3() const { return Vector3; }
    explicit operator v4() const { return Vector4; }
    operator __m128() const { return Register; }

    static inline xmm CreateMask(bool Value) {
        xmm Zero = xmm();
        xmm Result = (Value) ? xmm(_mm_cmpeq_ps(Zero, Zero)) : Zero;
        return Result;
    }
};

MATHCALL f32 SquareRoot(f32 Value) {
    xmm Result = _mm_sqrt_ss(xmm(Value));
    return (f32)Result;
}
MATHCALL f32 Max(f32 A, f32 B) {
    xmm Result = _mm_max_ss(xmm(A), xmm(B));
    return (f32)Result;
}
MATHCALL f32 Min(f32 A, f32 B) {
    xmm Result = _mm_min_ss(xmm(A), xmm(B));
    return (f32)Result;
}
MATHCALL f32 Negate(f32 Value) {
    xmm SignBit = xmm(F32SignBit);
    xmm Result = _mm_xor_ps(SignBit, xmm(Value));
    return (f32)Result;
}
MATHCALL f32 Sign(f32 Value) {
    xmm Result = 1.0f;
    Result = _mm_or_ps(Result, xmm(F32SignBit));
    return (f32)Result;
}
MATHCALL f32 Reciprocal(f32 Value) {
    xmm Result = _mm_rcp_ss(xmm(Value));
    return (f32)Result;
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
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v2)xmm0;
}
inline v3::v3(f32 X) {
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v3)xmm0;
}
inline v4::v4(f32 X) {
    xmm xmm0 = _mm_broadcastss_ps(xmm(X));
    *this = (v4)xmm0;
}
inline v2::v2(f32 X, f32 Y) {
    xmm xmm0 = _mm_set_ps(0.0f, 0.0f, Y, X);
    *this = (v2)xmm0;
}
inline v3::v3(f32 X, f32 Y, f32 Z) {
    xmm xmm0 = _mm_set_ps(0.0f, Z, Y, X);
    *this = (v3)xmm0;
}
inline v4::v4(f32 X, f32 Y, f32 Z, f32 W) {
    xmm xmm0 = _mm_set_ps(W, Z, Y, X);
    *this = (v4)xmm0;
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

inline f32 v2::Dot(const v2 &A, const v2 &B) {
    v2 Mul = A * B;
    return Mul.x + Mul.y;
}
inline f32 v2::LengthSquared(const v2 &Value) {
    return v2::Dot(Value, Value);
}
inline f32 v2::Length(const v2 &Value) {
    return SquareRoot(v2::LengthSquared(Value));
}
inline v2 v2::Normalize(const v2 &Value) {
    f32 LengthSquared = v2::LengthSquared(Value);

    bool LengthGreaterThanZero = LengthSquared > F32Epsilon;
    xmm Mask = xmm::CreateMask(LengthGreaterThanZero);

    f32 Length = SquareRoot(LengthSquared);
    v2 Result = Value * Reciprocal(Length);

    xmm MaskedResult = _mm_and_ps(xmm(Result), Mask);
    return (v2)MaskedResult;
}
MATHCALL v2 operator+(const v2 &A, const v2 &B) {
    xmm Result = _mm_add_ps(xmm(A), xmm(B));
    return (v2)Result;
}
MATHCALL v2 operator-(const v2 &A, const v2 &B) {
    xmm Result = _mm_sub_ps(xmm(A), xmm(B));
    return (v2)Result;
}
MATHCALL v2 operator*(const v2 &A, const v2 &B) {
    xmm Result = _mm_mul_ps(xmm(A), xmm(B));
    return (v2)Result;
}
MATHCALL v2 operator/(const v2 &A, const v2 &B) {
    xmm Result = _mm_div_ps(xmm(A), xmm(B));
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
    xmm Mask = xmm::CreateMask(LengthGreaterThanZero);

    f32 Length = SquareRoot(LengthSquared);
    v3 Result = Value * Reciprocal(Length);

    xmm MaskedResult = _mm_and_ps(xmm(Result), Mask);
    return (v3)MaskedResult;
}
inline v3 v3::Cross(const v3 &A, const v3 &B) {
    v3 Result;
    Result.x = A.y * B.z - A.z * B.y;
    Result.y = A.z * B.x - A.x * B.z;
    Result.z = A.x * B.y - A.y * B.x;
    return Result;
}
MATHCALL v3 operator+(const v3 &A, const v3 &B) {
    xmm Result = _mm_add_ps(xmm(A), xmm(B));
    return (v3)Result;
}
MATHCALL v3 operator-(const v3 &A, const v3 &B) {
    xmm Result = _mm_sub_ps(xmm(A), xmm(B));
    return (v3)Result;
}
MATHCALL v3 operator*(const v3 &A, const v3 &B) {
    xmm Result = _mm_mul_ps(xmm(A), xmm(B));
    return (v3)Result;
}
MATHCALL v3 operator/(const v3 &A, const v3 &B) {
    xmm Result = _mm_div_ps(xmm(A), xmm(B));
    return (v3)Result;
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

#if SIMD_WIDTH >= 8
union ymm {
    f32x8 Vector8;
    __m256 Register;
    inline ymm() { Register = _mm256_xor_ps(Register, Register); };
    inline ymm(const f32x8 &Value) : Vector8(Value) { };
    inline ymm(const __m256 &YMM) : Register(YMM) { };

    explicit operator f32x8() const { return Vector8; }
    operator __m256() const { return Register; }
};

MATHCALL f32x8 operator+(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_add_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator-(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_sub_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator*(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_mul_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator/(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_div_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
inline f32x8 f32x8::SquareRoot(const f32x8 &A) {
    ymm Result = _mm256_sqrt_ps(ymm(A));
    return (f32x8)Result;
}
MATHCALL f32x8 operator==(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_cmp_ps(ymm(A), ymm(B), _CMP_EQ_OQ);
    return (f32x8)Result;
}
MATHCALL f32x8 operator!=(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_cmp_ps(ymm(A), ymm(B), _CMP_NEQ_OQ);
    return (f32x8)Result;
}
MATHCALL f32x8 operator>(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_cmp_ps(ymm(A), ymm(B), _CMP_GT_OQ);
    return (f32x8)Result;
}
MATHCALL f32x8 operator<(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_cmp_ps(ymm(A), ymm(B), _CMP_LT_OQ);
    return (f32x8)Result;
}
MATHCALL bool IsZero(const f32x8 &Value) {
    ymm Zero = ymm();
    ymm ComparisonResult = _mm256_cmp_ps(ymm(Value), Zero, _CMP_NEQ_OQ);
    s32 MoveMask = _mm256_movemask_ps(ComparisonResult);
    return MoveMask == 0;
}
#endif