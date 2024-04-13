#include "..\base.h"

memory_arena Temp;

void CreateWindow(const string8 &title, u32 Width, u32 Height) {

}
bool WindowShouldClose() {

    return true;
}

memory_arena memory_arena::CreateScratch() {
    memory_arena Result = {0};

    return Result;
}

__attribute__((export_name("main")))
s32 main() {
    s32 Result = AppMain();
    return Result;
}