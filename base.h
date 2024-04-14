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
#ifndef SIMD_WIDTH
    #if defined(__AVX2__)
        #define SIMD_WIDTH 8
    #else
        #define SIMD_WIDTH 4
    #endif
#endif

#if defined(_WIN32)
    #define PLATFORM_WIN32
#elif defined(__gnu_linux__)
    #define PLATFORM_LINUX
#elif defined(__WASM__)
    #define PLATFORM_WASM
#endif

#if defined(__amd64__) | defined(_M_AMD64)
    #define CPU_X64
#elif defined(__arm__) | defined(_M_ARM)
    #define CPU_ARM
#endif
// NOTE: CPU_WASM must be defined manually

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
static constexpr u32 GetFormatSizeInBytes(format Format) {
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

/* == Math == */
struct v2;
struct v3;
struct v4;
struct v2x;
struct v3x;
struct v4x;
#if defined(CPU_X64)
    #include "x64_math.h"
#elif defined(CPU_ARM)
    #include "arm_math.h"
#elif defined(CPU_WASM)
    #include "wasm_math.h"
#else
    #error "Platform not supported"
#endif