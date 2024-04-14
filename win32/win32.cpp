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
	MSG Message;
	while (PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE)) {
		DispatchMessage(&Message);
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

[[noreturn]]
void AppMain() {

    InitOSProperties();

    init_params InitParams = {0};
    OnInit(&InitParams);
    CreateWindow(InitParams.WindowTitle, InitParams.WindowWidth, InitParams.WindowHeight);
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
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}