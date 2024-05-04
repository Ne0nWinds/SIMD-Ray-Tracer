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

struct v2;
struct v3;
struct v4;

struct v2_reference;
struct v3_reference;
struct v4_reference;

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

/* == Debugging == */
#if defined(_DEBUG)
    #if defined(PLATFORM_WIN32)
        #define Break() __debugbreak()
        #define Assert(Expr) if (!(Expr)) Break()
    #elif defined(PLATFORM_WASM)
        void __attribute__((import_name("__break"))) __break();
        #define Break() __break()
        #define Assert(Expr) if (!(Expr)) Break()
    #endif
#endif

#ifndef _DEBUG
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
    void Reset();
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

[[maybe_unused]]
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

enum class key : u32 {
    Escape = 0x1,
	One,
	Two,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Zero,
	Minus,
	Plus,
	Backspace,
	Tab,
	Q,
	W,
	E,
	R,
	T,
	Y,
	U,
	I,
	O,
	P,
	LeftBracket,
	RightBracket,
	Enter,
	LeftControl,
	A,
	S,
	D,
	F,
	G,
	H,
	J,
	K,
	L,
	Semicolon,
	Quote,
	GraveAccent,
	LeftShift,
	Pipe,
	Z,
	X,
	C,
	V,
	B,
	N,
	M,
	Comma,
	Period,
	QuestionMark,
	RightShift,
	NumpadMultiply,
	LeftAlt,
	Space,
	CapsLock,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	Pause,
	ScrollLock,
	Numpad7,
	Numpad8,
	Numpad9,
	NumpadMinus,
	Numpad4,
	Numpad5,
	Numpad6,
	NumpadPlus,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad0,
	NumpadPeriod,
	AltPrintScreen,
	_Unused,
	OEM10,
	F11,
	F12,
	LeftWindows,
	RightAlt,
	RightWindows,
	Menu,
	RightControl,
	Insert,
	Home,
	PageUp,
	Delete,
	End,
	PageDown,
	ArrowUp,
	ArrowLeft,
	ArrowDown,
	ArrowRight,
	NumLock,
	NumpadForwardSlash,
	NumpadEnter,
	Count
};

enum class button : u32 {
	DPadUp         = 1 << 0,
	DPadDown       = 1 << 1,
	DPadLeft       = 1 << 2,
	DPadRight      = 1 << 3,
	Start          = 1 << 4,
	Back           = 1 << 5,
	LeftThumb      = 1 << 6,
	RightThumb     = 1 << 7,
	LeftShoulder   = 1 << 8,
	RightShoulder  = 1 << 9,
	A              = 1 << 10,
	B              = 1 << 11,
	X              = 1 << 12,
	Y              = 1 << 13,
	Count
};

enum class mouse_button : u32 {
	LeftMouseButton   = 1 << 0,
	RightMouseButton  = 1 << 1,
	MiddleMouseButton = 1 << 2,
	XButton1          = 1 << 3,
	XButton2          = 1 << 4,
	Count
};

bool IsDown(key Key);
bool IsUp(key Key);
bool WasReleased(key Key);
bool WasPressed(key Key);

bool IsDown(button Button);
bool IsUp(button Button);
bool WasReleased(button Button);
bool WasPressed(button Button);

bool IsDown(mouse_button Button);
bool IsUp(mouse_button Button);
bool WasReleased(mouse_button Button);
bool WasPressed(mouse_button Button);

v2 GetAnalogInput(u32 StickIndex);
v2 GetMouseDelta();
v2 GetMouseWheelDelta();

/* == Math == */
#define MATHCALL static inline


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
    explicit inline v3(const v3_reference &V3);

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

struct u32x4;
struct u32x8;

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
using u32x = u32x4;
#elif SIMD_WIDTH == 8
using f32x = f32x8;
using v2x = v2x8;
using v3x = v3x8;
using v4x = v4x8;
using u32x = u32x8;
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

inline v3::v3(const v3_reference &V3) {
    this->x = V3.x;
    this->y = V3.y;
    this->z = V3.z;
}

struct f32x4 {
    f32 Value[4];

    inline f32x4() { }
    inline f32x4(f32 V) {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }
    explicit inline f32x4(const u32x4 &V);

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

struct u32x4 {
    u32 Value[4];

    inline u32x4() { }
    inline constexpr u32x4(u32 V) : Value() {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }

    inline u32 &operator[](u32 Index) {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    inline const u32 &operator[](u32 Index) const {
        Assert(Index < array_len(Value));
        return Value[Index];
    }
} __attribute__((__vector_size__(16), __aligned__(16)));

MATHCALL u32x4 operator+(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator-(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator*(const u32x4 &A, const u32x4 &B);

MATHCALL u32x4 operator==(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator!=(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator>(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator<(const u32x4 &A, const u32x4 &B);

MATHCALL u32x4 operator&(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator|(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator^(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator~(const u32x4 &A);
MATHCALL u32x4 operator>>(const u32x4 &A, const u32x4 &B);
MATHCALL u32x4 operator<<(const u32x4 &A, const u32x4 &B);

MATHCALL u32x4 operator>>(const u32x4 &A, const u32 &&B);
MATHCALL u32x4 operator<<(const u32x4 &A, const u32 &&B);

MATHCALL void operator+=(u32x4 &A, const u32x4 &B) { A = A + B; }
MATHCALL void operator-=(u32x4 &A, const u32x4 &B) { A = A - B; }
MATHCALL void operator*=(u32x4 &A, const u32x4 &B) { A = A * B; }
MATHCALL void operator&=(u32x4 &A, const u32x4 &B) { A = A & B; }
MATHCALL void operator|=(u32x4 &A, const u32x4 &B) { A = A | B; }
MATHCALL void operator^=(u32x4 &A, const u32x4 &B) { A = A ^ B; }
MATHCALL void operator>>=(u32x4 &A, const u32x4 &B) { A = A >> B; }
MATHCALL void operator<<=(u32x4 &A, const u32x4 &B) { A = A << B; };
MATHCALL void operator>>=(u32x4 &A, const u32 &&B) { A = A >> B; }
MATHCALL void operator<<=(u32x4 &A, const u32 &&B) { A = A << B; };


#if SIMD_WIDTH >= 8

struct f32x8 {
    f32 Value[8];

    inline f32x8() { }
    inline f32x8(const f32 V) {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }
    explicit inline f32x8(const u32x8 &V);

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

MATHCALL void operator+=(f32x8 &A, const f32x8 &B) { A = A + B; }
MATHCALL void operator-=(f32x8 &A, const f32x8 &B) { A = A - B; }
MATHCALL void operator*=(f32x8 &A, const f32x8 &B) { A = A * B; }
MATHCALL void operator/=(f32x8 &A, const f32x8 &B) { A = A / B; }
MATHCALL void operator&=(f32x8 &A, const f32x8 &B) { A = A & B; }
MATHCALL void operator|=(f32x8 &A, const f32x8 &B) { A = A | B; }
MATHCALL void operator^=(f32x8 &A, const f32x8 &B) { A = A ^ B; }

struct u32x8 {
    u32 Value[8];

    inline u32x8() { }
    inline constexpr u32x8(u32 V) : Value() {
        for (u32 i = 0; i < array_len(Value); ++i) {
            Value[i] = V;
        }
    }

    inline u32 &operator[](u32 Index) {
        Assert(Index < array_len(Value));
        return Value[Index];
    }

    inline const u32 &operator[](u32 Index) const {
        Assert(Index < array_len(Value));
        return Value[Index];
    }
} __attribute__((__vector_size__(32), __aligned__(32)));

MATHCALL u32x8 operator+(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator-(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator*(const u32x8 &A, const u32x8 &B);

MATHCALL u32x8 operator==(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator!=(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator>(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator<(const u32x8 &A, const u32x8 &B);

MATHCALL u32x8 operator&(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator|(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator^(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator~(const u32x8 &A);
MATHCALL u32x8 operator>>(const u32x8 &A, const u32x8 &B);
MATHCALL u32x8 operator<<(const u32x8 &A, const u32x8 &B);

MATHCALL u32x8 operator>>(const u32x8 &A, const u32 &&B);
MATHCALL u32x8 operator<<(const u32x8 &A, const u32 &&B);

MATHCALL void operator+=(u32x8 &A, const u32x8 &B) { A = A + B; }
MATHCALL void operator-=(u32x8 &A, const u32x8 &B) { A = A - B; }
MATHCALL void operator*=(u32x8 &A, const u32x8 &B) { A = A * B; }
MATHCALL void operator&=(u32x8 &A, const u32x8 &B) { A = A & B; }
MATHCALL void operator|=(u32x8 &A, const u32x8 &B) { A = A | B; }
MATHCALL void operator^=(u32x8 &A, const u32x8 &B) { A = A ^ B; }
MATHCALL void operator>>=(u32x8 &A, const u32x8 &B) { A = A >> B; }
MATHCALL void operator<<=(u32x8 &A, const u32x8 &B) { A = A << B; };
MATHCALL void operator>>=(u32x8 &A, const u32 &&B) { A = A >> B; }
MATHCALL void operator<<=(u32x8 &A, const u32 &&B) { A = A << B; };

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
    inline v3x8(const f32 &&X, const f32 &&Y, const f32 &&Z) : x(X), y(Y), z(Z) { }
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
MATHCALL void operator+=(v3x8 &A, const v3x8 &B) {
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
}
MATHCALL v3x8 operator-(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return Result;
}
MATHCALL void operator-=(v3x8 &A, const v3x8 &B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;
}
MATHCALL v3x8 operator*(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    return Result;
}
MATHCALL void operator*=(v3x8 &A, const v3x8 &B) {
    A.x *= B.x;
    A.y *= B.y;
    A.z *= B.z;
}
MATHCALL v3x8 operator/(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x / B.x;
    Result.y = A.y / B.y;
    Result.z = A.z / B.z;
    return Result;
}
MATHCALL void operator/=(v3x8 &A, const v3x8 &B) {
    A.x /= B.x;
    A.y /= B.y;
    A.z /= B.z;
}
MATHCALL v3x8 operator&(const v3x8 &A, const v3x8 &B) {
    v3x8 Result;
    Result.x = A.x & B.x;
    Result.y = A.y & B.y;
    Result.z = A.z & B.z;
    return Result;
}
MATHCALL void operator&=(v3x8 &A, const v3x8 &B) {
    A.x &= B.x;
    A.y &= B.y;
    A.z &= B.z;
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

/* == Pseudo-Random Number Generators == */

#define RANDOM_ALGORITHM_PCG 1
#define RANDOM_ALGORITHM_XORSHIFT 2
#define RANDOM_ALGORITHM_LCG 3

static constexpr u32 DefaultRandomAlgorithm = RANDOM_ALGORITHM_PCG;

struct u32x_random_state {
    u32x Seed;

    inline u32x PCG() {
        u32x State = this->Seed;
        u32x Result = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
        this->Seed = State * 747796405U;
        return (Result >> 22u) ^ Result;
    }
    inline u32x XorShift() {
        u32x Result = this->Seed;
        Seed ^= Seed << 13;
        Seed ^= Seed >> 17;
        Seed ^= Seed << 5;
        return Result * 0x4F6CDD1DU + 2891336453U;
    }
    inline u32x LCG() {
        u32x Result = this->Seed;
        this->Seed *= 747796405U;
        this->Seed += 2891336453U;
        return Result;
    }
    inline u32x RandomInt() {
        switch (DefaultRandomAlgorithm) {
            case RANDOM_ALGORITHM_PCG: return this->PCG();
            case RANDOM_ALGORITHM_XORSHIFT: return this->XorShift();
            case RANDOM_ALGORITHM_LCG: return this->LCG();
        }
    }
    inline f32x RandomFloat(f32 Min = -1.0f, f32 Max = 1.0f) {
        u32x N = this->RandomInt();
        constexpr f32 InverseMaxInt = 1.0 / (f64)((u32)-1);
        f32x RandomFloat = f32x(N) * InverseMaxInt;
        f32x Result = RandomFloat * (Max - Min) + Min;
        return Result;
    }
};

struct u32_random_state {
    u64 Seed;

    inline u32 PCG() {
        u64 OldSeed = Seed;
        Seed = Seed * 6364136223846793005ULL + 1442695040888963407ULL;
        u32 Result = RotateRight32((u32)(OldSeed >> 32) ^ (u32)OldSeed, OldSeed >> 59);
        return Result;
    }
    inline u32 XorShift() {
        u64 Result = Seed;
        Seed ^= Seed >> 12;
        Seed ^= Seed << 25;
        Seed ^= Seed >> 27;
        return (Result * 0x2545F4914F6CDD1DULL) >> 32;
    }
    inline u32 LCG() {
        u32 Result = this->Seed;
        Seed = Seed * 6364136223846793005ULL + 1442695040888963407ULL;
        return Result;
    }
    inline u32 RandomInt() {
        switch (DefaultRandomAlgorithm) {
            case RANDOM_ALGORITHM_PCG: return this->PCG();
            case RANDOM_ALGORITHM_XORSHIFT: return this->XorShift();
            case RANDOM_ALGORITHM_LCG: return this->LCG();
        }
    }
    inline f32 RandomFloat(f32 Min = -1.0f, f32 Max = 1.0f) {
        u32 N = this->RandomInt();
        f32 InverseMaxInt = (Max - Min) / (f64)((u32)-1);
        f32 RandomFloat = (f32)N * InverseMaxInt;
        f32 Result = RandomFloat + Min;
        return Result;
    }
};

/* == Profiling == */
f64 QueryTimestampInMilliseconds();