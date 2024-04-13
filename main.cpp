
#include "base.h"

static window_handle WindowHandle;
static u32 Width = 1280;
static u32 Height = 720;

void OnWindowResize(u32 NewWidth, u32 NewHeight) {
    Width = NewWidth;
    Height = NewHeight;
}

s32 AppMain() {

    constexpr string8 WindowTitle = u8"Raytracing In One Weekend";
    CreateWindow(WindowTitle, Width, Height);

    while (!WindowShouldClose()) {
        memory_arena Scratch = Temp.CreateScratch();
        // image Image = CreateImage(&Scratch, Width, Height, format::R32B32G32A32_F32);

        // v4 *ImageData = (v4 *)Image.Data;
        // for (u32 y = 0; y < Height; ++y) {
        //     for (u32 x = 0; x < Height; ++x) {
        //         ImageData[y * Width + x] = v4(1.0f, 0.0f, 0.0f, 1.0f);
        //     }
        // }

        // BlitImage(WindowHandle, Image);
    }

    return 0;
}