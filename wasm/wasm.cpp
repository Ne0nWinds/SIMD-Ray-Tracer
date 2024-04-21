#include "..\base.h"

#define WASM_EXPORT(name) __attribute__((export_name(#name))) name
#define WASM_IMPORT(name) __attribute__((import_name(#name))) name

static memory_arena Temp;

static u32 WasmPagesAllocated = 0;
#define PAGE_SIZE 65536


memory_arena AllocateArenaFromOS(u32 Size, u64 StartingAddress) {
    memory_arena Result = {0};

    u32 CommitSize = RoundUpPowerOf2(Size, (u32)PAGE_SIZE);

    Result.Start = (void *)(WasmPagesAllocated * PAGE_SIZE);
    Result.Size = CommitSize;
    Result.Offset = Result.Start;

    u32 PagesNeeded = CommitSize / PAGE_SIZE;
    s32 Success = __builtin_wasm_memory_grow(0, PagesNeeded);
    Assert(Success != -1);
    (void)Success;

    (void)StartingAddress; // WASM already has a fixed address space

    return Result;
}

memory_arena memory_arena::CreateScratch() {
    memory_arena Result = {0};
    Result.Start = this->Offset;
    Result.Offset = Result.Start;
    Result.Size = this->Size - ((u8 *)this->Offset - (u8 *)this->Start);
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

static void InitWASMEnvironmentProperties() {
    WasmPagesAllocated = __builtin_wasm_memory_size(0);
    Temp = AllocateArenaFromOS(MB(256), 0);
}

extern "C" void _start() {
}

void WASM_EXPORT(init)() {
    InitWASMEnvironmentProperties();
    init_params Params;
    OnInit(&Params);
}

void *WASM_EXPORT(draw)(u32 Width, u32 Height) {
    memory_arena Scratch = Temp.CreateScratch();
    image Image = CreateImage(&Scratch, Width, Height, format::R8G8B8A8_U32);
    OnRender(Image);
    return Image.Data;
}