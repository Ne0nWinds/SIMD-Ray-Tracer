
#include "base.h"

void OnInit(init_params *Params) {
    constexpr string8 WindowTitle = u8"Raytracing In One Weekend";
    Params->WindowTitle = WindowTitle;
    Params->WindowWidth = 1280;
    Params->WindowHeight = 720;
}

static u32 U32FromV4(v4 Value) {
    u8 r = Value.x * 255.0f;
    u8 g = Value.y * 255.0f;
    u8 b = Value.z * 255.0f;
    u8 a = 255;
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

void OnRender(const image &Image) {
    u32 *ImageData = (u32 *)Image.Data;
    for (u32 y = 0; y < Image.Height; ++y) {
        for (u32 x = 0; x < Image.Width; ++x) {
            v4 PixelValue = v4(x / (f32)Image.Width, y / (f32)Image.Height, 0.0f, 1.0f);
            ImageData[y * Image.Width + x] = U32FromV4(PixelValue);
        }
    }
}