#pragma once

#include <immintrin.h>

union xmm {
    __m128 Register;
    __m128i IntRegister;

    inline xmm() { Register = _mm_setzero_ps(); }
    inline xmm(f32 Value) { Register = _mm_set_ss(Value); };
    inline xmm(const v2 &Value) { Register = _mm_load_sd((f64 *)&Value); };
    inline xmm(const v3 &Value) { Register = _mm_loadu_ps((f32 *)&Value); };
    inline xmm(const v4 &Value) { Register = _mm_loadu_ps((f32 *)&Value); };
    inline xmm(const f32x4 &Value) { Register = _mm_loadu_ps(Value.Value); };
    inline xmm(const u32x4 &Value) { IntRegister = _mm_load_si128((const __m128i *)&Value); };
    inline xmm(const __m128 &XMM) : Register(XMM) { };

    explicit operator f32() const { return _mm_cvtss_f32(Register); }
    explicit operator v2() const { 
        v2 Result;
        _mm_store_sd((f64 *)&Result, Register);
        return Result;
    }
    explicit operator v3() const {
        v3 Result;
        _mm_store_ps((f32 *)&Result, Register);
        return Result;
    }
    explicit operator v4() const {
        v4 Result;
        _mm_store_ps((f32 *)&Result, Register);
        return Result;
    }
    explicit operator f32x4() const {
        f32x4 Result;
        _mm_store_ps(Result.Value, Register);
        return Result;
    }
    explicit operator u32x4() const {
        u32x4 Result;
        _mm_store_si128((__m128i *)Result.Value, IntRegister);
        return Result;
    }
    operator __m128() const { return Register; }

    static inline xmm CreateMask(bool Value) {
        u32 Mask = Value * - 1;
        f32 MaskAsFloat;
        __builtin_memcpy(&MaskAsFloat, &Mask, sizeof(f32));
        xmm Result = _mm_set1_ps(MaskAsFloat);
        return Result;
    }
};

MATHCALL u32 Min(u32 A, u32 B) {
	u32 DOZ = (A - B) & -(A >= B);
	u32 Result = A - DOZ;
	return Result;
}

MATHCALL f32 Abs(f32 Value) {
    constexpr u32 SignBitU32 = ~F32SignBit;
    xmm SignBit = _mm_set1_ps(*(f32 *)&SignBitU32);
    xmm Result = _mm_and_ps(xmm(Value), SignBit);
    return (f32)Result;
}
MATHCALL f32 SquareRoot(f32 Value) {
    xmm Result = _mm_sqrt_ss(xmm(Value));
    return (f32)Result;
}
MATHCALL f32 InverseSquareRoot(f32 Value) {
    xmm Result = _mm_rsqrt_ss(xmm(Value));
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
    xmm SignBit = xmm(-0.0f);
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
    xmm xmm0 = _mm_set1_ps(X);
    *this = (v2)xmm0;
}
inline v3::v3(const f32 &X) {
    xmm xmm0 = _mm_set1_ps(X);
    *this = (v3)xmm0;
}
inline v4::v4(f32 X) {
    xmm xmm0 = _mm_set1_ps(X);
    *this = (v4)xmm0;
}

inline v2::v2(f32 X, f32 Y) {
    xmm xmm0 = _mm_set_ps(0.0f, 0.0f, Y, X);
    *this = (v2)xmm0;
}
inline v3::v3(const f32 &X, const f32 &Y, const f32 &Z) {
    xmm xmm0 = _mm_set_ps(0.0f, Z, Y, X);
    *this = (v3)xmm0;
}
inline v4::v4(f32 X, f32 Y, f32 Z, f32 W) {
    xmm xmm0 = _mm_set_ps(W, Z, Y, X);
    *this = (v4)xmm0;
}

#ifdef __SSE4_2__
MATHCALL u32 PopCount(u32 a) {
	u32 Result = _mm_popcnt_u32(a);
	return Result;
}
MATHCALL u64 PopCount(u64 a) {
	u64 Result = _mm_popcnt_u64(a);
	return Result;
}
#else
MATHCALL u32 PopCount(u32 a) {
    return __builtin_popcount(a);
}
MATHCALL u64 PopCount(u64 a) {
    return __builtin_popcount(a);
}
#endif

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

MATHCALL u32 RotateRight32(u32 Value, s32 Rotation) {
    return _rotr(Value, Rotation);
}
MATHCALL u64 RotateRight64(u64 Value, s32 Rotation) {
    return _rotr64(Value, Rotation);
}
MATHCALL u32 RotateLeft32(u32 Value, s32 Rotation) {
    return _rotl(Value, Rotation);
}
MATHCALL u64 RotateLeft64(u64 Value, s32 Rotation) {
    return _rotl64(Value, Rotation);
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
MATHCALL v3 operator-(const v3 &A) {
    xmm Result = _mm_xor_ps(xmm(A), xmm(_mm_set1_ps(-0.0f)));
    return (v3)Result;
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
    v3 Result = Value / Length;

    xmm MaskedResult = _mm_and_ps(xmm(Result), Mask);
    return (v3)MaskedResult;
}
inline v3 v3::NormalizeFast(const v3 &Value) {
    f32 LengthSquared = v3::LengthSquared(Value);

    bool LengthGreaterThanZero = LengthSquared > F32Epsilon;
    xmm Mask = xmm::CreateMask(LengthGreaterThanZero);

    f32 InvLength = InverseSquareRoot(LengthSquared);
    v3 Result = Value * InvLength;

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

MATHCALL v4 operator+(const v4 &A, const v4 &B) {
    xmm Result = _mm_add_ps(xmm(A), xmm(B));
    return (v4)Result;
}
MATHCALL v4 operator-(const v4 &A, const v4 &B) {
    xmm Result = _mm_sub_ps(xmm(A), xmm(B));
    return (v4)Result;
}
MATHCALL v4 operator*(const v4 &A, const v4 &B) {
    xmm Result = _mm_mul_ps(xmm(A), xmm(B));
    return (v4)Result;
}
MATHCALL v4 operator/(const v4 &A, const v4 &B) {
    xmm Result = _mm_div_ps(xmm(A), xmm(B));
    return (v4)Result;
}

inline f32x4 v3x4::Dot(const v3x4 &A, const v3x4 &B) {
    v3x4 C = A * B;
    f32x4 Result = C.x + C.y + C.z;
    return Result;
}

inline f32x4 v3x4::LengthSquared(const v3x4 &A) {
    v3x4 C = A * A;
    f32x4 Result = C.x + C.y + C.z;
    return Result;
}

inline f32x v3x::Length(const v3x &A) {
    f32x LengthSquared = v3x::Dot(A, A);
    f32x Length = f32x::SquareRoot(LengthSquared);
    return Length;
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
    f32x LengthSquared = v3x::LengthSquared(Value);

    f32x LengthGreaterThanZeroMask = LengthSquared > F32Epsilon;

    f32x InverseLength = f32x::InverseSquareRoot(LengthSquared);
    v3x Result = Value * InverseLength;

    v3x MaskedResult = Result & LengthGreaterThanZeroMask;
    return MaskedResult;
}
inline void v3x::ConditionalMove(v3x *A, const v3x &B, const f32x &MoveMask) {
    f32x::ConditionalMove(&A->x, B.x, MoveMask);
    f32x::ConditionalMove(&A->y, B.y, MoveMask);
    f32x::ConditionalMove(&A->z, B.z, MoveMask);
}

#if SIMD_WIDTH >= 8
union ymm {
    __m256 Register;
    __m256i IntRegister;

    inline ymm() { Register = _mm256_setzero_ps(); };
    inline ymm(const f32x8 &Value) {
        Register = _mm256_load_ps(Value.Value);
    };
    inline ymm(const u32x8 &Value) {
        IntRegister = _mm256_load_si256((__m256i *)Value.Value);
    };
    inline ymm(const __m256 &YMM) : Register(YMM) { };

    explicit operator u32x8() const {
        u32x8 Result;
        _mm256_store_si256((__m256i *)Result.Value, IntRegister);
        return Result;
    }
    explicit operator f32x8() const {
        f32x8 Result;
        _mm256_store_ps(Result.Value, Register);
        return Result;
    }
    operator __m256() const { return Register; }
};

inline f32x8::f32x8(const u32x &V) {
    ymm Result = _mm256_cvtepi32_ps(ymm(V));
    *this = (f32x8)Result;
}
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
MATHCALL f32x8 operator&(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_and_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator|(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_or_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator^(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_xor_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
MATHCALL f32x8 operator~(const f32x8 &A) {
    ymm Ones = _mm256_cmp_ps(ymm(), ymm(), _CMP_EQ_OQ);
    ymm Result = _mm256_xor_ps(ymm(A), Ones);
    return (f32x8)Result;
}
inline f32x8 v3x8::Dot(const v3x8 &A, const v3x8 &B) {
    ymm XMul = _mm256_mul_ps(ymm(A.x), ymm(B.x));
    ymm YMulPlusXMul = _mm256_fmadd_ps(ymm(A.y), ymm(B.y), ymm(XMul));
    ymm Result = _mm256_fmadd_ps(ymm(A.z), ymm(B.z), YMulPlusXMul);
    return (f32x8)Result;
}
inline f32x8 v3x8::LengthSquared(const v3x8 &A) {
    ymm XMul = _mm256_mul_ps(ymm(A.x), ymm(A.x));
    ymm YMulPlusXMul = _mm256_fmadd_ps(ymm(A.y), ymm(A.y), ymm(XMul));
    ymm Result = _mm256_fmadd_ps(ymm(A.z), ymm(A.z), YMulPlusXMul);
    return (f32x8)Result;
}
inline f32x8 f32x8::SquareRoot(const f32x8 &A) {
    ymm Result = _mm256_sqrt_ps(ymm(A));
    return (f32x8)Result;
}
inline f32x8 f32x8::InverseSquareRoot(const f32x8 &A) {
    ymm Result = _mm256_rsqrt_ps(ymm(A));
    return (f32x8)Result;
}
inline f32x8 f32x8::Min(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_min_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
inline f32x8 f32x8::Max(const f32x8 &A, const f32x8 &B) {
    ymm Result = _mm256_max_ps(ymm(A), ymm(B));
    return (f32x8)Result;
}
inline f32x8 f32x8::Reciprocal(const f32x8 &Value) {
    ymm Result = _mm256_rcp_ps(ymm(Value));
    return (f32x8)Result;
}
inline void f32x8::ConditionalMove(f32x8 *A, const f32x8 &B, const f32x8 &MoveMask) {
    // f32x8 BlendedResult = (*A & ~MoveMask) | (B & MoveMask);
    ymm BlendedResult = _mm256_blendv_ps(ymm(*A), ymm(B), ymm(MoveMask));
    *A = (f32x8)BlendedResult;
}
inline f32 f32x8::HorizontalMin(const f32x8 &Value) {
    xmm min = _mm256_extractf128_ps(ymm(Value), 1);
    min = _mm_min_ps(min, _mm256_extractf128_ps(ymm(Value), 0));
    min = _mm_min_ps(min, _mm_movehl_ps(min, min));
    min = _mm_min_ps(min, _mm_movehdup_ps(min));
    return _mm_cvtss_f32(min);
}
inline u32 f32x8::HorizontalMinIndex(const f32x8 &Value) {
    f32 MinValue = f32x8::HorizontalMin(Value);
    ymm Comparison = Value == f32x8(MinValue);
    u32 MoveMask = _mm256_movemask_ps(Comparison);
    u32 MinIndex = _tzcnt_u32(MoveMask);
    return MinIndex;
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
    s32 MoveMask = _mm256_movemask_ps(ymm(Value));
    return MoveMask == 0;
}


MATHCALL u32x8 operator+(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_add_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator-(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_sub_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator*(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_mul_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}

MATHCALL u32x8 operator==(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_cmpeq_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator!=(const u32x8 &A, const u32x8 &B) {
    ymm Ones = _mm256_cmpeq_epi32(ymm(u32x8(0)), ymm(u32x8(0)));
    ymm ComparisonResult = _mm256_cmpeq_epi32(ymm(A), ymm(B));
    ymm Result = _mm256_xor_si256(ComparisonResult, Ones);
    return (u32x8)Result;
}
MATHCALL u32x8 operator>(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_cmpgt_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator<(const u32x8 &A, const u32x8 &B) {
    ymm Ones = _mm256_cmpeq_epi32(ymm(u32x8(0)), ymm(u32x8(0)));
    ymm ComparisonResult = _mm256_cmpgt_epi32(ymm(A - 1), ymm(B));
    ymm Result = _mm256_xor_si256(ComparisonResult, Ones);
    return (u32x8)Result;
}

MATHCALL u32x8 operator&(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_and_si256(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator|(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_or_si256(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator^(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_xor_si256(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator~(const u32x8 &A) {
    ymm Ones = _mm256_cmpeq_epi32(ymm(u32x8(0)), ymm(u32x8(0)));
    ymm Result = _mm256_xor_si256(ymm(A), Ones);
    return (u32x8)Result;
}
MATHCALL u32x8 operator>>(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_srlv_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator<<(const u32x8 &A, const u32x8 &B) {
    ymm Result = _mm256_sllv_epi32(ymm(A), ymm(B));
    return (u32x8)Result;
}
MATHCALL u32x8 operator>>(const u32x8 &A, const u32 &&B) {
    ymm Result = _mm256_srli_epi32(ymm(A), B);
    return (u32x8)Result;
}
MATHCALL u32x8 operator<<(const u32x8 &A, const u32 &&B) {
    ymm Result = _mm256_slli_epi32(ymm(A), B);
    return (u32x8)Result;
}
inline void u32x8::ConditionalMove(u32x8 *A, const u32x8 &B, const u32x8 &MoveMask) {
    ymm BlendedResult = _mm256_blendv_ps(ymm(*A), ymm(B), ymm(MoveMask));
    *A = (u32x8)BlendedResult;
}
#endif

inline f32x4 f32x4::SquareRoot(const f32x4 &A) {
    xmm Result = _mm_sqrt_ps(xmm(A));
    return (f32x4)Result;
}
inline f32x4 f32x4::InverseSquareRoot(const f32x4 &A) {
    xmm Result = _mm_rsqrt_ps(xmm(A));
    return (f32x4)Result;
}
inline f32x4 f32x4::Min(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_min_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
inline f32x4 f32x4::Max(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_max_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
inline f32x4 f32x4::Reciprocal(const f32x4 &A) {
    xmm Result = _mm_rcp_ps(xmm(A));
    return (f32x4)Result;
}
inline void f32x4::ConditionalMove(f32x4 *A, const f32x4 &B, const f32x4 &MoveMask) {
    f32x4 BlendedResult = (*A & ~MoveMask) | (B & MoveMask);
    *A = (f32x4)BlendedResult;
}
inline f32 f32x4::HorizontalMin(const f32x4 &Value) {
    xmm min = Value;
    min = _mm_min_ps(min, _mm_movehl_ps(min, min));
    xmm Shuffled =  _mm_shuffle_ps(min, min, 0b00'01'00'01);
    min = _mm_min_ps(min, Shuffled);
    return _mm_cvtss_f32(min);
}
inline u32 f32x4::HorizontalMinIndex(const f32x4 &Value) {
    f32 MinValue = f32x4::HorizontalMin(Value);
    xmm Comparison = Value == f32x4(MinValue);
    u32 MoveMask = _mm_movemask_ps(Comparison);
    u32 MinIndex = _tzcnt_u32(MoveMask);
    return MinIndex;
}

MATHCALL f32x4 operator+(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_add_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator-(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_sub_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator*(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_mul_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator/(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_div_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}

MATHCALL f32x4 operator==(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_cmpeq_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator!=(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_cmpneq_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator>(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_cmpgt_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator<(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_cmplt_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}

MATHCALL f32x4 operator&(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_and_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator|(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_or_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator^(const f32x4 &A, const f32x4 &B) {
    xmm Result = _mm_xor_ps(xmm(A), xmm(B));
    return (f32x4)Result;
}
MATHCALL f32x4 operator~(const f32x4 &A) {
    xmm Ones = _mm_cmpeq_ps(xmm(), xmm());
    xmm Result = _mm_xor_ps(xmm(A), Ones);
    return (f32x4)Result;
}

MATHCALL bool IsZero(const f32x4 &Value) {
    xmm Zero = xmm();
    xmm ComparisonResult = _mm_cmpneq_ps(xmm(Value), Zero);
    s32 MoveMask = _mm_movemask_ps(ComparisonResult);
    return MoveMask == 0;
}

MATHCALL u32x4 operator+(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_add_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator-(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_sub_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator*(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_mul_epu32(xmm(A), xmm(B));
    return (u32x4)Result;
}

MATHCALL u32x4 operator==(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_cmpeq_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator!=(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_cmpeq_epi32(xmm(A), xmm(B));
    return ~(u32x4)Result;
}
MATHCALL u32x4 operator>(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_cmpgt_epi32(xmm(A), xmm(B)); // not technically correct
    return (u32x4)Result;
}
MATHCALL u32x4 operator<(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_cmplt_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}

MATHCALL u32x4 operator&(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_and_si128(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator|(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_or_si128(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator^(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_xor_si128(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator~(const u32x4 &A) {
    xmm Ones = _mm_cmpeq_epi32(xmm(), xmm());
    xmm Result = _mm_xor_si128(xmm(A), Ones);
    return (u32x4)Result;
}
MATHCALL u32x4 operator>>(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_sll_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}
MATHCALL u32x4 operator<<(const u32x4 &A, const u32x4 &B) {
    xmm Result = _mm_srl_epi32(xmm(A), xmm(B));
    return (u32x4)Result;
}

MATHCALL u32x4 operator>>(const u32x4 &A, const u32 &&B) {
    xmm Result = _mm_slli_epi32(xmm(A), B);
    return (u32x4)Result;
}
MATHCALL u32x4 operator<<(const u32x4 &A, const u32 &&B) {
    xmm Result = _mm_srli_epi32(xmm(A), B);
    return (u32x4)Result;
}

inline void u32x4::ConditionalMove(u32x4 *A, const u32x4 &B, const u32x4 &MoveMask) {
    u32x4 BlendedResult = (*A & ~MoveMask) | (B & MoveMask);
    *A = (u32x4)BlendedResult;
}

inline f32x4::f32x4(const u32x4 &V) {
    xmm Result = _mm_cvtepi32_ps(xmm(V));
    *this = (f32x4)Result;
}