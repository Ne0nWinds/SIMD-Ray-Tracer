#pragma once

/* == Base Types == */
#include <stdint.h>
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char char8;

typedef float f32;
typedef double f64;

struct string8 {
    char8 *Data;
    u32 Size;

    constexpr string8() : Data(0), Size(0) { };

    constexpr string8(const char8 *Str) :
        Data(const_cast<char8 *>(Str)),
        Size(0) {
            u32 Size = 0;
            char8 *Char = const_cast<char8 *>(Str);

            while (*Char) {
                Char += 1;
                Size += 1;
            }

            this->Size = Size;
        }
};

/* == Macros == */

#define array_len(arr) (sizeof(arr) / sizeof(*arr)) 

#ifndef SIMD_WIDTH
    #if defined(__AVX2__)
        #define SIMD_WIDTH 8
    #elif defined(__SSE2__)
        #define SIMD_WIDTH 4
    #elif defined(CPU_WASM)
        #define SIMD_WIDTH 4
    #endif
    #ifndef SIMD_WIDTH
        #error "SIMD_WIDTH not defined for this platform"
    #endif
#endif

#if defined(__amd64__) | defined(_M_AMD64)
    #define CPU_X64
#elif defined(__arm__) | defined(_M_ARM)
    #define CPU_ARM
#endif
// NOTE: CPU_WASM must be defined manually

#if defined(_WIN32)
    #define PLATFORM_WIN32
#elif defined(__gnu_linux__)
    #define PLATFORM_LINUX
#elif defined(__WASM__) | defined(CPU_WASM)
    #define PLATFORM_WASM
#endif

#if defined(PLATFORM_WIN32)
    #define Break() __debugbreak()
    #define Assert(Expr) if (!(Expr)) Break()
#else
    #define Break()
    #define Assert(Expr)
#endif

/* == OS / Memory == */
#define KB(b) (b * 1024)
#define MB(b) (KB(b) * 1024)
#define GB(b) (MB(b) * 1024LLU)
#define TB(b) (GB(b) * 1024LLU)

struct memory_arena {
    void *Start;
    void *Offset;
    u32 Size;

    void *Push(u64 Size, u32 Alignment = 16);
    void Pop(void *Ptr);
    memory_arena CreateScratch();
};
memory_arena AllocateArenaFromOS(u32 Size, u64 StartingAddress = 0);

enum class format : u32 {
    R32B32G32A32_F32 = 0x1,
    R8G8B8A8_U32,
};
static u32 GetFormatSizeInBytes(format Format) {
    switch (Format) {
        case format::R32B32G32A32_F32: {
            return 16;
        }
        case format::R8G8B8A8_U32: {
            return 4;
        }
    }
}

struct image {
    void *Data;
    u32 Width, Height;
    format Format;
};

static image CreateImage(memory_arena *Arena, u32 Width, u32 Height, format Format) {
    image Result = {0};
    Result.Data = Arena->Push(Width * Height * GetFormatSizeInBytes(Format));
    Result.Width = Width;
    Result.Height = Height;
    Result.Format = Format;
    return Result;
}

struct window_handle {
    u64 Handle;
};

struct init_params {
    u32 WindowWidth, WindowHeight;
    string8 WindowTitle;
};

void OnInit(init_params *Params);
void OnRender(const image &Image);

#define MATHCALL static inline

/* == Math == */
struct v2 {
    f32 x = 0.0f, y = 0.0f;
    inline v2() { };
    inline v2(f32 X);
    inline v2(f32 X, f32 Y);

    MATHCALL f32 Dot(const v2 &A, const v2 &B);
    MATHCALL f32 LengthSquared(const v2 &Value);
    MATHCALL f32 Length(const v2 &Value);
    MATHCALL v2 Normalize(const v2 &Value);
} __attribute__((__vector_size__(8), __aligned__(8)));
MATHCALL v2 operator+(const v2 &A, const v2 &B);
MATHCALL v2 operator-(const v2 &A, const v2 &B);
MATHCALL v2 operator*(const v2 &A, const v2 &B);
MATHCALL v2 operator/(const v2 &A, const v2 &B);

struct v3 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    inline v3() { };
    inline v3(f32 X);
    inline v3(f32 X, f32 Y, f32 Z);

    static inline f32 Dot(const v3 &A, const v3 &B);
    static inline f32 LengthSquared(const v3 &Value);
    static inline f32 Length(const v3 &Value);
    static inline v3 Normalize(const v3 &Value);
    static inline v3 Cross(const v3 &A, const v3 &B);
} __attribute__((__vector_size__(12), __aligned__(16)));
MATHCALL v3 operator+(const v3 &A, const v3 &B);
MATHCALL v3 operator-(const v3 &A, const v3 &B);
MATHCALL v3 operator*(const v3 &A, const v3 &B);
MATHCALL v3 operator/(const v3 &A, const v3 &B);

MATHCALL void operator+=(v3 &A, const v3 &B) {
    A = A + B;
}
MATHCALL void operator-=(v3 &A, const v3 &B) {
    A = A - B;
}
MATHCALL void operator*=(v3 &A, const v3 &B) {
    A = A * B;
}
MATHCALL void operator/=(v3 &A, const v3 &B) {
    A = A / B;
}

struct v4 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    inline v4() { };
    inline v4(f32 X);
    inline v4(f32 X, f32 Y, f32 Z, f32 W);

    MATHCALL f32 Dot(const v4 &A, const v4 &B);
    MATHCALL f32 LengthSquared(const v4 &Value);
    MATHCALL f32 Length(const v4 &Value);
    MATHCALL v4 Normalize(const v4 &Value);
} __attribute__((__vector_size__(16), __aligned__(16)));
MATHCALL v4 operator+(const v4 &A, const v4 &B);
MATHCALL v4 operator-(const v4 &A, const v4 &B);
MATHCALL v4 operator*(const v4 &A, const v4 &B);
MATHCALL v4 operator/(const v4 &A, const v4 &B);

struct f32x4;
struct f32x8;
struct v2x4;
struct v2x8;
struct v3x4;
struct v3x8;
struct v4x4;
struct v4x8;

#if SIMD_WIDTH == 1
typedef f32 f32x;
typedef v2 v2x;
typedef v3 v3x;
typedef v4 v4x;
#elif SIMD_WIDTH == 4
using f32x = f32x4;
using v2x = v2x4;
using v3x = v3x4;
using v4x = v4x4;
#elif SIMD_WIDTH == 8
using f32x = f32x8;
using v2x = v2x8;
using v3x = v3x8;
using v4x = v4x8;
#endif

struct v2_reference {
    f32 &x, &y;
    constexpr inline void operator=(const v2 &Value) {
        this->x = Value.x;
        this->y = Value.y;
    }
};
struct v3_reference {
    f32 &x, &y, &z;
    constexpr inline void operator=(const v3 &Value) {
        this->x = Value.x;
        this->y = Value.y;
        this->z = Value.z;
    }
};
struct v4_reference {
    f32 &x, &y, &z, &w;
    constexpr inline void operator=(const v4 &Value) {
        this->x = Value.x;
        this->y = Value.y;
        this->z = Value.z;
        this->w = Value.w;
    }
};

struct f32x4 {
    f32 Value[4];

    inline f32x4() { }
    inline constexpr f32x4(f32 V) : Value() {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }

    inline f32 &operator[](u32 Index) {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    inline const f32 &operator[](u32 Index) const {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    static inline f32x4 SquareRoot(const f32x4 &A);
    static inline f32x4 Min(const f32x4 &A, const f32x4 &B);
    static inline f32x4 Max(const f32x4 &A, const f32x4 &B);
    static inline f32x4 Reciprocal(const f32x4 &A);
    static inline void ConditionalMove(f32x4 *A, const f32x4 &B, const f32x4 &MoveMask);
    static inline f32 HorizontalMin(const f32x4 &A);
    static inline u32 HorizontalMinIndex(const f32x4 &A);
} __attribute__((__vector_size__(16), __aligned__(16)));
MATHCALL f32x4 operator+(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator-(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator*(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator/(const f32x4 &A, const f32x4 &B);

MATHCALL f32x4 operator==(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator!=(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator>(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator<(const f32x4 &A, const f32x4 &B);

MATHCALL f32x4 operator&(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator|(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator^(const f32x4 &A, const f32x4 &B);
MATHCALL f32x4 operator~(const f32x4 &A);

#if SIMD_WIDTH >= 8

struct f32x8 {
    f32 Value[8];

    inline f32x8() { }
    inline constexpr f32x8(f32 V) : Value() {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }

    inline f32 &operator[](u32 Index) {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    inline const f32 &operator[](u32 Index) const {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    static inline f32x8 SquareRoot(const f32x8 &A);
    static inline f32x8 Min(const f32x8 &A, const f32x8 &B);
    static inline f32x8 Max(const f32x8 &A, const f32x8 &B);
    static inline f32x8 Reciprocal(const f32x8 &A);
    static inline void ConditionalMove(f32x8 *A, const f32x8 &B, const f32x8 &MoveMask);
    static inline f32 HorizontalMin(const f32x &A);
    static inline u32 HorizontalMinIndex(const f32x &A);
} __attribute__((__vector_size__(32), __aligned__(32)));
MATHCALL f32x8 operator+(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator-(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator*(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator/(const f32x8 &A, const f32x8 &B);

MATHCALL f32x8 operator==(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator!=(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator>(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator<(const f32x8 &A, const f32x8 &B);

MATHCALL f32x8 operator&(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator|(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator^(const f32x8 &A, const f32x8 &B);
MATHCALL f32x8 operator~(const f32x8 &A);

struct v2x8 {
    f32x8 x, y;
    inline v2_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v2_reference Result = {
            .x = x[Index],
            .y = y[Index],
        };
        return Result;
    }
};
struct v3x8 {
    f32x8 x, y, z;

    inline v3x8(const f32 &&Value = 0.0f) : x(Value), y(Value), z(Value) { }
    inline v3x8(const f32x8 &Value) : x(Value), y(Value), z(Value) { }
    inline v3x8(const v3 &Value) : x(Value.x), y(Value.y), z(Value.z) { }

    inline v3_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v3_reference Result = {
            .x = x[Index],
            .y = y[Index],
            .z = z[Index],
        };
        return Result;
    }
    static inline f32x Dot(const v3x8 &A, const v3x8 &B);
    static inline f32x Length(const v3x8 &A);
    static inline f32x LengthSquared(const v3x8 &A);
    static inline v3x8 Normalize(const v3x8 &A);
    static inline void ConditionalMove(v3x8 *A, const v3x8 &B, const f32x8 &MoveMask);
};
MATHCALL v3x8 operator+(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return Result;
}
MATHCALL v3x8 operator-(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return Result;
}
MATHCALL v3x8 operator*(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    return Result;
}
MATHCALL v3x8 operator/(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x / B.x;
    Result.y = A.y / B.y;
    Result.z = A.z / B.z;
    return Result;
}
MATHCALL v3x8 operator&(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x & B.x;
    Result.y = A.y & B.y;
    Result.z = A.z & B.z;
    return Result;
}
struct v4x8 {
    f32x8 x, y, z, w;
    inline v4_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v4_reference Result = {
            .x = x[Index],
            .y = y[Index],
            .z = z[Index],
            .w = w[Index]
        };
        return Result;
    }
};
#endif

struct v2x4 {
    f32x4 x, y;

    inline v2_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v2_reference Result = {
            .x = x[Index],
            .y = y[Index],
        };
        return Result;
    }
};
struct v3x4 {
    f32x4 x, y, z;

    inline v3x4() { }
    inline v3x4(const f32 &&Value) : x(Value), y(Value), z(Value) { }
    inline v3x4(const f32x4 &Value) : x(Value), y(Value), z(Value) { }
    inline v3x4(const v3 &Value) : x(Value.x), y(Value.y), z(Value.z) { }

    inline v3_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v3_reference Result = {
            .x = x[Index],
            .y = y[Index],
            .z = z[Index],
        };
        return Result;
    }

    static inline f32x4 Dot(const v3x4 &A, const v3x4 &B);
    static inline f32x4 Length(const v3x4 &A);
    static inline f32x4 LengthSquared(const v3x4 &A);
    static inline v3x4 Normalize(const v3x4 &A);
    static inline void ConditionalMove(v3x4 *A, const v3x4 &B, const f32x4 &MoveMask);
};
MATHCALL v3x4 operator+(const v3x4 &A, const v3x4 &B) {
    v3x4 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return Result;
}
MATHCALL v3x4 operator-(const v3x4 &A, const v3x4 &B) {
    v3x4 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return Result;
}
MATHCALL v3x4 operator*(const v3x4 &A, const v3x4 &B) {
    v3x4 Result;
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    return Result;
}
MATHCALL v3x4 operator/(const v3x4 &A, const v3x4 &B) {
    v3x4 Result;
    Result.x = A.x / B.x;
    Result.y = A.y / B.y;
    Result.z = A.z / B.z;
    return Result;
}
MATHCALL v3x4 operator&(const v3x4 &A, const v3x4 &B) {
    v3x4 Result;
    Result.x = A.x & B.x;
    Result.y = A.y & B.y;
    Result.z = A.z & B.z;
    return Result;
}

struct v4x4 {
    f32x4 x, y, z, w;
    inline v4_reference operator[](u32 Index) {
        Assert(Index < array_len(x.Value));
        v4_reference Result = {
            .x = x[Index],
            .y = y[Index],
            .z = z[Index],
            .w = w[Index]
        };
        return Result;
    }
};

constexpr static f32 F32Epsilon = 1e-5f;
constexpr static f32 F32Min = 1e-30f;
constexpr static f32 F32Max = 1e30f;
constexpr static f32 PI32 = 3.14159265358979323846f;
constexpr static u32 F32SignBit = 0x8000'0000;

#if defined(CPU_X64)
    #include "x64_math.h"
#elif defined(CPU_ARM)
    #include "arm_math.h"
#elif defined(CPU_WASM)
    #include "wasm_math.h"
#else
    #error "Platform not supported"
#endif