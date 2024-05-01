#include "..\base.h"

#define WIN32_LEAN_AND_MEAN 1
#define UNICODE
#include <Windows.h>

#include <gl/GL.h>

static memory_arena Temp;

static u32 WindowWidth = 1280;
static u32 WindowHeight = 720;
static u32 GLTextureHandle;
static HDC DeviceContext;

static u64 AllocationGranularity;
static u64 NumberOfCPUs;
static LARGE_INTEGER PerformanceFrequency;

static u64 KeyboardState[2];
static u64 PrevKeyboardState[2];

static wchar_t *WideStringFromString8(const string8 &String) {
    memory_arena Scratch = Temp.CreateScratch();
    wchar_t *Result = 0;

	s32 Length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS | MB_PRECOMPOSED, (const char *)String.Data, String.Size + 1, 0, 0);
	if (Length == 0) goto error;

	Result = (wchar_t *)Scratch.Push(sizeof(wchar_t));
	Length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS | MB_PRECOMPOSED, (const char *)String.Data, String.Size + 1, Result, Length * sizeof(wchar_t));
	if (Length == 0) goto error;

	return Result;
error:
    Break();
    return 0;
}


memory_arena AllocateArenaFromOS(u32 Size, u64 StartingAddress) {
    memory_arena Result = {0};

    u32 CommitSize = RoundUpPowerOf2(Size, (u32)AllocationGranularity);

    Result.Start = VirtualAlloc((LPVOID)StartingAddress, CommitSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Result.Size = CommitSize;
    Result.Offset = Result.Start;

    Assert(Result.Start != NULL);

    return Result;
}

void *memory_arena::Push(u64 Size, u32 Alignment) {
    Assert(PopCount(Alignment) == 1);

    u8 *AlignedOffset = (u8 *)this->Offset;
    AlignedOffset = (u8 *)RoundUpPowerOf2((u64)AlignedOffset, (u64)Alignment);

    u8 *NewOffset = AlignedOffset + Size;
    Assert((u64)NewOffset - (u64)this->Start < this->Size);
    this->Offset = (void *)NewOffset;

    return AlignedOffset;
}

void memory_arena::Pop(void *Ptr) {
    Assert((u64)Ptr >= (u64)this->Start && (u64)Ptr < (u64)this->Offset);
    this->Offset = Ptr;
}

void memory_arena::Reset() {
    this->Offset = this->Start;
}

memory_arena memory_arena::CreateScratch() {
    memory_arena Result = {0};
    Result.Start = this->Offset;
    Result.Offset = Result.Start;
    Result.Size = this->Size - ((u8 *)this->Offset - (u8 *)this->Start);
    return Result;
}

static HWND WindowHandle;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitializeOpenGL() {
    DeviceContext = GetDC(WindowHandle);
    PIXELFORMATDESCRIPTOR PixelFormatDesc;
    s32 PixelFormat = ChoosePixelFormat(DeviceContext, &PixelFormatDesc);
    SetPixelFormat(DeviceContext, PixelFormat, &PixelFormatDesc);

    HGLRC OpenGLContext = wglCreateContext(DeviceContext);
    Assert(OpenGLContext);
    wglMakeCurrent(DeviceContext, OpenGLContext);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glViewport(0, 0, WindowWidth, WindowHeight);

    glGenTextures(1, &GLTextureHandle);
    glBindTexture(GL_TEXTURE_2D, GLTextureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

}

#undef CreateWindow
void CreateWindow(const string8 &Title, u32 WindowWidth, u32 WindowHeight) {
    HMODULE HInstance = GetModuleHandle(0);
    wchar_t *WindowTitle = WideStringFromString8(Title);

    {
        WNDCLASSW WindowClass = {0};
        WindowClass.lpfnWndProc = WindowProc;
        WindowClass.hInstance = HInstance;
        WindowClass.lpszClassName = WindowTitle;
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassW(&WindowClass);

        DWORD WindowStyle = WS_OVERLAPPEDWINDOW;

        RECT WindowSize = {0};
        WindowSize.right = (LONG)WindowWidth;
        WindowSize.bottom = (LONG)WindowHeight;
        AdjustWindowRectEx(&WindowSize, WindowStyle, FALSE, 0);

        u32 AdjustedWidth = WindowSize.right - WindowSize.left;
        u32 AdjustedHeight = WindowSize.bottom - WindowSize.top;

        WindowHandle = CreateWindowExW(
            0,
            WindowTitle,
            WindowTitle,
            WindowStyle | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, AdjustedWidth, AdjustedHeight,
            NULL, NULL, HInstance, NULL
        );
        Assert(WindowHandle != NULL);
    }
}

static bool ShouldWindowClose = false;
bool WindowShouldClose() {
    PrevKeyboardState[0] = KeyboardState[0];
    PrevKeyboardState[1] = KeyboardState[1];

	MSG Message;
	while (PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE)) {
		DispatchMessage(&Message);
	}

	HWND ActiveWindow = GetActiveWindow();
	if (ActiveWindow != WindowHandle) {
        PrevKeyboardState[0] = PrevKeyboardState[1] = 0;
        KeyboardState[0] = KeyboardState[1] = 0;
    }

    return ShouldWindowClose;
}

static void InitOSProperties() {
    {
        // CPU / OS Info
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);
        AllocationGranularity = SystemInfo.dwAllocationGranularity;
        NumberOfCPUs = SystemInfo.dwNumberOfProcessors;
        Assert(PopCount(AllocationGranularity) == 1);
        Assert(NumberOfCPUs >= 1);
    }

    {
        // Init Memory Allocator
        u64 StartingAddress = 0;
#ifdef _DEBUG
        StartingAddress = TB(0x4); // Deterministic pointers in debug mode
#endif
        Temp = AllocateArenaFromOS(MB(256), StartingAddress);
    }

    {
        // rdtsc / profiling
        BOOL Success = QueryPerformanceFrequency(&PerformanceFrequency);
        Assert(Success);
        (void)Success;
    }
}

/* == Input == */

enum class scan_code : u32 {
    // Based on ANSI Layout
    Escape = 0x1,
	One = 0x2,
	Two = 0x3,
	Three = 0x4,
	Four = 0x5,
	Five = 0x6,
	Six = 0x7,
	Seven = 0x8,
	Eight = 0x9,
	Nine = 0xA,
	Zero = 0xB,
	Minus = 0xC,
	Plus = 0xD,
	Backspace = 0xE,
	Tab = 0xF,
	Q = 0x10,
	W = 0x11,
	E = 0x12,
	R = 0x13,
	T = 0x14,
	Y = 0x15,
	U = 0x16,
	I = 0x17,
	O = 0x18,
	P = 0x19,
	LeftBracket = 0x1A,
	RightBracket = 0x1B,
	Enter = 0x1C,
	LeftControl = 0x1D,
	A = 0x1E,
	S = 0x1F,
	D = 0x20,
	F = 0x21,
	G = 0x22,
	H = 0x23,
	J = 0x24,
	K = 0x25,
	L = 0x26,
	Semicolon = 0x27,
	Quote = 0x28,
	GraveAccent = 0x29,
	LeftShift = 0x2A,
	Pipe = 0x2B,
	Z = 0x2C,
	X = 0x2D,
	C = 0x2E,
	V = 0x2F,
	B = 0x30,
	N = 0x31,
	M = 0x32,
	Comma = 0x33,
	Period = 0x34,
	QuestionMark = 0x35,
	RightShift = 0x36,
	NumpadMultiply = 0x37,
	LeftAlt = 0x38,
	Space = 0x39,
	CapsLock = 0x3A,
	F1 = 0x3B,
	F2 = 0x3C,
	F3 = 0x3D,
	F4 = 0x3E,
	F5 = 0x3F,
	F6 = 0x40,
	F7 = 0x41,
	F8 = 0x42,
	F9 = 0x43,
	F10 = 0x44,
	Pause = 0x45,
	ScrollLock = 0x46,
	Numpad7 = 0x47,
	Numpad8 = 0x48,
	Numpad9 = 0x49,
	NumpadMinus = 0x4A,
	Numpad4 = 0x4B,
	Numpad5 = 0x4C,
	Numpad6 = 0x4D,
	NumpadPlus = 0x4E,
	Numpad1 = 0x4F,
	Numpad2 = 0x50,
	Numpad3 = 0x51,
	Numpad0 = 0x52,
	NumpadPeriod = 0x53,
	AltPrintScreen = 0x54,
	_Unused = 0x55,
	OEM10 = 0x56,
	F11 = 0x57,
	F12 = 0x58,
	LeftWindows = 0xE05B,
	RightAlt = 0xE038,
	RightWindows = 0xE05C,
	Menu = 0xE05D,
	RightControl = 0xE01D,
	Insert = 0xE052,
	Home = 0xE047,
	PageUp = 0xE049,
	Delete = 0xE053,
	End = 0xE04F,
	PageDown = 0xE051,
	ArrowUp = 0xE048,
	ArrowLeft = 0xE04B,
	ArrowDown = 0xE050,
	ArrowRight = 0xE04D,
	NumLock = 0x45,
	NumpadForwardSlash = 0xE035,
	NumpadEnter = 0xE01C,
};

static void SetKeyDown(key Key) {
    bool HighBits = (u32)Key >= 64;
    u64 BitToSet = (u64)Key;
    if (HighBits) BitToSet -= 64;
    KeyboardState[HighBits] |= 1 << BitToSet;
}
static void SetKeyUp(key Key) {
    bool HighBits = (u32)Key >= 64;
    u64 BitToSet = (u64)Key;
    if (HighBits) BitToSet -= 64;
    KeyboardState[HighBits] &= ~(1 << BitToSet);
}

bool IsDown(key Key) {
    bool HighBits = (u32)Key >= 64;
    u64 BitToSet = (u64)Key;
    if (HighBits) BitToSet -= 64;
    bool Result = (KeyboardState[HighBits] & (1 << BitToSet)) != 0;
    return Result;
}

bool IsUp(key Key) {
    bool HighBits = (u32)Key >= 64;
    u64 BitToSet = (u64)Key;
    if (HighBits) BitToSet -= 64;
    bool Result = (KeyboardState[HighBits] & (1 << BitToSet)) == 0;
    return Result;
}

static inline key KeyFromScanCode(scan_code ScanCode) {
	u32 Value = 0;

	if ((u32)ScanCode <= 0x58) {
		Value = (u32)ScanCode;
	}

	if ((u32)ScanCode & 0xE000) {
		switch (ScanCode) {
			case scan_code::LeftWindows: {
				Value = (u32)key::LeftWindows;
			} break;
			case scan_code::RightAlt: {
				Value = (u32)key::RightAlt;
			} break;
			case scan_code::RightWindows: {
				Value = (u32)key::RightWindows;
			} break;
			case scan_code::Menu: {
				Value = (u32)key::Menu;
			} break;
			case scan_code::RightControl: {
				Value = (u32)key::RightControl;
			} break;
			case scan_code::Insert: {
				Value = (u32)key::Insert;
			} break;
			case scan_code::Home: {
				Value = (u32)key::Home;
			} break;
			case scan_code::PageUp: {
				Value = (u32)key::PageUp;
			} break;
			case scan_code::Delete: {
				Value = (u32)key::Delete;
			} break;
			case scan_code::End: {
				Value = (u32)key::End;
			} break;
			case scan_code::PageDown: {
				Value = (u32)key::PageDown;
			} break;
			case scan_code::ArrowUp: {
				Value = (u32)key::ArrowUp;
			} break;
			case scan_code::ArrowLeft: {
				Value = (u32)key::ArrowLeft;
			} break;
			case scan_code::ArrowDown: {
				Value = (u32)key::ArrowDown;
			} break;
			case scan_code::ArrowRight: {
				Value = (u32)key::ArrowRight;
			} break;
			case scan_code::NumLock: {
				Value = (u32)key::NumLock;
			} break;
			case scan_code::NumpadForwardSlash: {
				Value = (u32)key::NumpadForwardSlash;
			} break;
			case scan_code::NumpadEnter: {
				Value = (u32)key::NumpadEnter;
			} break;
		}
	}

	return (key)Value;
}

static void InitializeInput() {
    RAWINPUTDEVICE RID[1] = {};
    RID[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
    RID[0].usUsage = 0x06; // KEYBOARD
    RID[0].dwFlags = 0;
    RID[0].hwndTarget = WindowHandle;

    // RID[1].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
    // RID[1].usUsage = 0x02; // MOUSE
    // RID[1].dwFlags = 0;
    // RID[1].hwndTarget = WindowHandle;

    UINT Result = RegisterRawInputDevices(RID, array_len(RID), sizeof(RAWINPUTDEVICE));
    Assert(Result == TRUE);
}


[[noreturn]]
void AppMain() {

    InitOSProperties();

    init_params InitParams = {0};
    OnInit(&InitParams);
    CreateWindow(InitParams.WindowTitle, InitParams.WindowWidth, InitParams.WindowHeight);
    InitializeInput();
    InitializeOpenGL();

    while (!WindowShouldClose()) {
        memory_arena Scratch = Temp.CreateScratch();
        image Image = CreateImage(&Scratch, WindowWidth, WindowHeight, format::R8G8B8A8_U32);
        OnRender(Image);

        {
            glBindTexture(GL_TEXTURE_2D, GLTextureHandle);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Image.Width, Image.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image.Data);

            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glBegin(GL_TRIANGLES);

            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, -1.0f);

            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(1.0f, -1.0f);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);


            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, -1.0f);

            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 1.0f);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);
            glEnd();

            SwapBuffers(DeviceContext);
        }
    }

    ExitProcess(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_QUIT: {
            ShouldWindowClose = true;
            return 0;
        }
        case WM_SIZE: {
            UINT NewWidth = LOWORD(lParam);
            UINT NewHeight = HIWORD(lParam);
            WindowWidth = NewWidth;
            WindowHeight = NewHeight;
            glViewport(0, 0, NewWidth, NewHeight);
            return 0;
        }
        case WM_INPUT: {
            RAWINPUT Input = {};
            u32 Size = sizeof(RAWINPUT);
            UINT Result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &Input, &Size, sizeof(RAWINPUTHEADER));
            if (!Result) return 0;

            switch (Input.header.dwType) {
                case RIM_TYPEKEYBOARD: {
                    RAWKEYBOARD Keyboard = Input.data.keyboard;
                    u32 MakeCode = Keyboard.MakeCode;
                    if (Keyboard.Flags & RI_KEY_E0) {
                        MakeCode |= 0xE000;
                    }
                    scan_code ScanCode = (scan_code)MakeCode;
                    bool KeyDown = !(Keyboard.Flags & RI_KEY_BREAK);
                    key Key = KeyFromScanCode(ScanCode);
                    if (KeyDown) {
                        SetKeyDown(Key);
                    } else {
                        SetKeyUp(Key);
                    }
                    return 0;
                } break;
                case RIM_TYPEMOUSE: {

                    return 0;
                } break;
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

f64 QueryTimestampInMilliseconds() {
    LARGE_INTEGER PerformanceCounter = {};
    QueryPerformanceCounter(&PerformanceCounter);
    f64 Result = (f64)PerformanceCounter.QuadPart / (PerformanceFrequency.QuadPart / 1000.0);
    return Result;
}