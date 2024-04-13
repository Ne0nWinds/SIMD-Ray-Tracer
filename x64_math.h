#pragma once

#include <immintrin.h>

#define MATHCALL static inline constexpr

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