#include "base.h"

#define WIN32_LEAN_AND_MEAN 1
#define _UNICODE
#include <Windows.h>

memory_arena Temp;

static u64 AllocationGranularity = 0;
static u64 NumberOfCPUs = 0;
static LARGE_INTEGER PerformanceFrequency;

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

#undef CreateWindow
void CreateWindow(const string8 &Title, u32 WindowWidth, u32 WindowHeight) {
    memory_arena ScratchArena = Temp.CreateScratch();

    HMODULE HInstance = GetModuleHandle(0);
    // wchar_t *WindowTitle = WideStringFromString8(Title);
    wchar_t *WindowTitle = L"Test";

    {
        WNDCLASSW WindowClass = {0};
        WindowClass.lpfnWndProc = WindowProc;
        WindowClass.hInstance = HInstance;
        WindowClass.lpszClassName = WindowTitle;
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassW(&WindowClass);
    }

    DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
    {
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

    {
        BOOL Success = QueryPerformanceFrequency(&PerformanceFrequency);
        Assert(Success);
        (void)Success;
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

[[noreturn]]
void _AppMain() {
    {
        SYSTEM_INFO SystemInfo = {};
        GetSystemInfo(&SystemInfo);
        AllocationGranularity = SystemInfo.dwAllocationGranularity;
        NumberOfCPUs = SystemInfo.dwNumberOfProcessors;
        Assert(PopCount(AllocationGranularity) == 1);
    }

    u64 StartingAddress = 0;
#ifdef _DEBUG
    StartingAddress = TB(0x4);
#endif
    Temp = AllocateArenaFromOS(MB(64), StartingAddress);

    s32 Result = AppMain();
    ExitProcess(Result);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_QUIT: {
            ShouldWindowClose = true;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}