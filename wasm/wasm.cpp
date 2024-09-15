#include "..\base.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <webgl/webgl1.h>
#include <stdio.h>

static memory_arena Temp;

static u32 CanvasWidth, CanvasHeight;
static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE WebGLContext;
static constexpr u32 AllocationGranularity = KB(64); // Size of WASM virtual page
static u64 NumberOfProcessors;

static u32 GLTextureHandle;
static u32 GLVertexBufferHandle;
static u32 GLVertexShader;
static u32 GLFragmentShader;
static u32 GLProgram;

static u64 KeyState[2] = {0};
static u64 PrevKeyState[2] = {0};

memory_arena AllocateArenaFromOS(u32 Size, u64 StartingAddress) {
	(void)StartingAddress;

    memory_arena Result = {0};

    u32 CommitSize = RoundUpPowerOf2(Size, (u32)AllocationGranularity);

    Result.Start = malloc(CommitSize);
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

void memory_arena::Reset() {
    this->Offset = this->Start;
}

u32 GetProcessorThreadCount() {
    // return NumberOfProcessors;
	return 1;
}

static void InitializeWebGL() {
	CanvasWidth = EM_ASM_INT({ return window.innerWidth; });
	CanvasHeight = EM_ASM_INT({ return window.innerHeight; });
	constexpr char CanvasName[] = "#canvas";
	emscripten_set_canvas_element_size(CanvasName, CanvasWidth, CanvasHeight);

	EmscriptenWebGLContextAttributes Attributes = {0};
	emscripten_webgl_init_context_attributes(&Attributes);
	Attributes.majorVersion = 1;
	Attributes.antialias = EM_FALSE;
	WebGLContext = emscripten_webgl_create_context(CanvasName, &Attributes);
	emscripten_webgl_make_context_current(WebGLContext);

	emscripten_glClearColor(0,0,0,1);
	emscripten_glClear(GL_COLOR_BUFFER_BIT);

    emscripten_glGenTextures(1, &GLTextureHandle);
    emscripten_glBindTexture(GL_TEXTURE_2D, GLTextureHandle);
	emscripten_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	emscripten_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	emscripten_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	emscripten_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	static constexpr f32 Quad[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,

		-1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
	};

	emscripten_glGenBuffers(1, &GLVertexBufferHandle);
	emscripten_glBindBuffer(GL_ARRAY_BUFFER, GLVertexBufferHandle);
	emscripten_glBufferData(GL_ARRAY_BUFFER, sizeof(Quad), (void *)Quad, GL_STATIC_DRAW);

	emscripten_glEnableVertexAttribArray(0);
	emscripten_glEnableVertexAttribArray(1);
	emscripten_glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(f32) * 4, 0);
	emscripten_glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(f32) * 4, (void *)(sizeof(f32) * 2));

	const char *VertexShader = R"(
		attribute vec2 a_pos;
		attribute vec2 a_texCoord;
		varying vec2 v_texCoord;
		void main() {
			gl_Position = vec4(a_pos.xy, 0.0, 1.0);
			v_texCoord = a_texCoord;
		}
	)";
	const char* FragmentShader = R"(
		precision mediump float;
		varying vec2 v_texCoord;
		uniform sampler2D u_texture;
		void main() {
			vec4 outColor = texture2D(u_texture, v_texCoord);
			outColor.w = 1.0;
			gl_FragColor = outColor;
		}
	)";

	{
		GLVertexShader = emscripten_glCreateShader(GL_VERTEX_SHADER);
		GLint Length = sizeof(VertexShader) - 1;
		emscripten_glShaderSource(GLVertexShader, 1, (const GLchar *const*)&VertexShader, NULL);
		emscripten_glCompileShader(GLVertexShader);
	}
	{
		GLFragmentShader = emscripten_glCreateShader(GL_FRAGMENT_SHADER);
		emscripten_glShaderSource(GLFragmentShader, 1, (const GLchar *const*)&FragmentShader, NULL);
		emscripten_glCompileShader(GLFragmentShader);
	}

	GLProgram = emscripten_glCreateProgram();
	emscripten_glAttachShader(GLProgram, GLVertexShader);
	emscripten_glAttachShader(GLProgram, GLFragmentShader);
	emscripten_glLinkProgram(GLProgram);

	emscripten_glUseProgram(GLProgram);
}
static void InitOSProperties() {
	NumberOfProcessors = EM_ASM_INT({
		return navigator.hardwareConcurrency;
	});


	Temp = AllocateArenaFromOS(MB(256));
}

static EM_BOOL RequestAnimationFrameCallback(double time, void *) {

	memory_arena Scratch = Temp.CreateScratch();
	image Image = CreateImage(&Scratch, CanvasWidth, CanvasHeight, format::R8G8B8A8_U32);
	OnRender(Image);

	// emscripten_glBindTexture(GL_TEXTURE_2D, GLTextureHandle);
	emscripten_glClear(GL_COLOR_BUFFER_BIT);
	emscripten_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Image.Width, Image.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image.Data);
	emscripten_glActiveTexture(GL_TEXTURE0);
	emscripten_glDrawArrays(GL_TRIANGLES, 0, 6);

	return EM_TRUE;
}

static u64 KeyboardState[2];
static u64 PreviousKeyboardState[2];

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

static constexpr u32 KeyCodeHash(const char *Code) {
	u64 OutputHash = 0xFC738A83F87CC9C8;
	u32 Index = 0;
	char C = Code[Index];
	do {
		OutputHash += C;
		OutputHash *= 0x2321842E1AE88BC3;
		C = Code[++Index];
	} while (C);
	return OutputHash ^ (OutputHash >> 32);
}

static EM_BOOL KeyCallbackFunction(int EventType, const EmscriptenKeyboardEvent *Event, void *) {

	// if (Event->code[0] == 'F' && Event->code[1] == '1') {
	// 	emscripten_console_log("F1");
	// }
	u32 EventCodeHash = KeyCodeHash(Event->code);

	key Key = key::None;
    switch (EventCodeHash) {
        case KeyCodeHash("Escape"): {
            Key = key::Escape;
        } break;
        case KeyCodeHash("Digit1"): {
            Key = key::One;
        } break;
        case KeyCodeHash("Digit2"): {
            Key = key::Two;
        } break;
        case KeyCodeHash("Digit3"): {
            Key = key::Three;
        } break;
        case KeyCodeHash("Digit4"): {
            Key = key::Four;
        } break;
        case KeyCodeHash("Digit5"): {
            Key = key::Five;
        } break;
        case KeyCodeHash("Digit6"): {
            Key = key::Six;
        } break;
        case KeyCodeHash("Digit7"): {
            Key = key::Seven;
        } break;
        case KeyCodeHash("Digit8"): {
            Key = key::Eight;
        } break;
        case KeyCodeHash("Digit9"): {
            Key = key::Nine;
        } break;
        case KeyCodeHash("Digit0"): {
            Key = key::Zero;
        } break;
        case KeyCodeHash("Minus"): {
            Key = key::Minus;
        } break;
        case KeyCodeHash("Equal"): {
            Key = key::Plus;
        } break;
        case KeyCodeHash("Backspace"): {
            Key = key::Backspace;
        } break;
        case KeyCodeHash("Tab"): {
            Key = key::Tab;
        } break;
        case KeyCodeHash("KeyQ"): {
            Key = key::Q;
        } break;
        case KeyCodeHash("KeyW"): {
            Key = key::W;
        } break;
        case KeyCodeHash("KeyE"): {
            Key = key::E;
        } break;
        case KeyCodeHash("KeyR"): {
            Key = key::R;
        } break;
        case KeyCodeHash("KeyT"): {
            Key = key::T;
        } break;
        case KeyCodeHash("KeyY"): {
            Key = key::Y;
        } break;
        case KeyCodeHash("KeyU"): {
            Key = key::U;
        } break;
        case KeyCodeHash("KeyI"): {
            Key = key::I;
        } break;
        case KeyCodeHash("KeyO"): {
            Key = key::O;
        } break;
        case KeyCodeHash("KeyP"): {
            Key = key::P;
        } break;
        case KeyCodeHash("BracketLeft"): {
            Key = key::LeftBracket;
        } break;
        case KeyCodeHash("BracketRight"): {
            Key = key::RightBracket;
        } break;
        case KeyCodeHash("Enter"): {
            Key = key::Enter;
        } break;
        case KeyCodeHash("ControlLeft"): {
            Key = key::LeftControl;
        } break;
        case KeyCodeHash("KeyA"): {
            Key = key::A;
        } break;
        case KeyCodeHash("KeyS"): {
            Key = key::S;
        } break;
        case KeyCodeHash("KeyD"): {
            Key = key::D;
        } break;
        case KeyCodeHash("KeyF"): {
            Key = key::F;
        } break;
        case KeyCodeHash("KeyG"): {
            Key = key::G;
        } break;
        case KeyCodeHash("KeyH"): {
            Key = key::H;
        } break;
        case KeyCodeHash("KeyJ"): {
            Key = key::J;
        } break;
        case KeyCodeHash("KeyK"): {
            Key = key::K;
        } break;
        case KeyCodeHash("KeyL"): {
            Key = key::L;
        } break;
        case KeyCodeHash("Semicolon"): {
            Key = key::Semicolon;
        } break;
        case KeyCodeHash("Quote"): {
            Key = key::Quote;
        } break;
        case KeyCodeHash("Backquote"): {
            Key = key::GraveAccent;
        } break;
        case KeyCodeHash("ShiftLeft"): {
            Key = key::LeftShift;
        } break;
        case KeyCodeHash("Backslash"): {
            Key = key::Pipe;
        } break;
        case KeyCodeHash("Z"): {
            Key = key::Z;
        } break;
        case KeyCodeHash("KeyX"): {
            Key = key::X;
        } break;
        case KeyCodeHash("KeyC"): {
            Key = key::C;
        } break;
        case KeyCodeHash("KeyV"): {
            Key = key::V;
        } break;
        case KeyCodeHash("KeyB"): {
            Key = key::B;
        } break;
        case KeyCodeHash("KeyN"): {
            Key = key::N;
        } break;
        case KeyCodeHash("KeyM"): {
            Key = key::M;
        } break;
        case KeyCodeHash("Comma"): {
            Key = key::Comma;
        } break;
        case KeyCodeHash("Period"): {
            Key = key::Period;
        } break;
        case KeyCodeHash("Slash"): {
            Key = key::QuestionMark;
        } break;
        case KeyCodeHash("ShiftRight"): {
            Key = key::RightShift;
        } break;
        case KeyCodeHash("NumpadMultiply"): {
            Key = key::NumpadMultiply;
        } break;
        case KeyCodeHash("AltLeft"): {
            Key = key::LeftAlt;
        } break;
        case KeyCodeHash("Space"): {
            Key = key::Space;
        } break;
        case KeyCodeHash("CapsLock"): {
            Key = key::CapsLock;
        } break;
        case KeyCodeHash("F1"): {
            Key = key::F1;
        } break;
        case KeyCodeHash("F2"): {
            Key = key::F2;
        } break;
        case KeyCodeHash("F3"): {
            Key = key::F3;
        } break;
        case KeyCodeHash("F4"): {
            Key = key::F4;
        } break;
        case KeyCodeHash("F5"): {
            Key = key::F5;
        } break;
        case KeyCodeHash("F6"): {
            Key = key::F6;
        } break;
        case KeyCodeHash("F7"): {
            Key = key::F7;
        } break;
        case KeyCodeHash("F8"): {
            Key = key::F8;
        } break;
        case KeyCodeHash("F9"): {
            Key = key::F9;
        } break;
        case KeyCodeHash("F10"): {
            Key = key::F10;
        } break;
        case KeyCodeHash("Pause--"): {
            Key = key::Pause;
        } break;
        case KeyCodeHash("ScrollLock"): {
            Key = key::ScrollLock;
        } break;
        case KeyCodeHash("Numpad7"): {
            Key = key::Numpad7;
        } break;
        case KeyCodeHash("Numpad8"): {
            Key = key::Numpad8;
        } break;
        case KeyCodeHash("Numpad9"): {
            Key = key::Numpad9;
        } break;
        case KeyCodeHash("NumpadMinus"): {
            Key = key::NumpadMinus;
        } break;
        case KeyCodeHash("Numpad4"): {
            Key = key::Numpad4;
        } break;
        case KeyCodeHash("Numpad5"): {
            Key = key::Numpad5;
        } break;
        case KeyCodeHash("Numpad6"): {
            Key = key::Numpad6;
        } break;
        case KeyCodeHash("NumpadPlus"): {
            Key = key::NumpadPlus;
        } break;
        case KeyCodeHash("Numpad1"): {
            Key = key::Numpad1;
        } break;
        case KeyCodeHash("Numpad2"): {
            Key = key::Numpad2;
        } break;
        case KeyCodeHash("Numpad3"): {
            Key = key::Numpad3;
        } break;
        case KeyCodeHash("Numpad0"): {
            Key = key::Numpad0;
        } break;
        case KeyCodeHash("NumpadPeriod"): {
            Key = key::NumpadPeriod;
        } break;
        case KeyCodeHash("AltPrintScreen"): {
            Key = key::AltPrintScreen;
        } break;
        case KeyCodeHash("_Unused"): {
            Key = key::_Unused;
        } break;
        case KeyCodeHash("OEM10"): {
            Key = key::OEM10;
        } break;
        case KeyCodeHash("F11"): {
            Key = key::F11;
        } break;
        case KeyCodeHash("F12"): {
            Key = key::F12;
        } break;
        case KeyCodeHash("MetaLeft"): {
            Key = key::LeftWindows;
        } break;
        case KeyCodeHash("AltRight"): {
            Key = key::RightAlt;
        } break;
        case KeyCodeHash("MetaRight"): {
            Key = key::RightWindows;
        } break;
        case KeyCodeHash("Menu"): {
            Key = key::Menu;
        } break;
        case KeyCodeHash("ControlRight"): {
            Key = key::RightControl;
        } break;
        case KeyCodeHash("Insert"): {
            Key = key::Insert;
        } break;
        case KeyCodeHash("Home"): {
            Key = key::Home;
        } break;
        case KeyCodeHash("PageUp"): {
            Key = key::PageUp;
        } break;
        case KeyCodeHash("Delete"): {
            Key = key::Delete;
        } break;
        case KeyCodeHash("End"): {
            Key = key::End;
        } break;
        case KeyCodeHash("PageDown"): {
            Key = key::PageDown;
        } break;
        case KeyCodeHash("ArrowUp"): {
            Key = key::ArrowUp;
        } break;
        case KeyCodeHash("ArrowLeft"): {
            Key = key::ArrowLeft;
        } break;
        case KeyCodeHash("ArrowDown"): {
            Key = key::ArrowDown;
        } break;
        case KeyCodeHash("ArrowRight"): {
            Key = key::ArrowRight;
        } break;
        case KeyCodeHash("NumLock"): {
            Key = key::NumLock;
        } break;
        case KeyCodeHash("NumpadForwardSlash"): {
            Key = key::NumpadForwardSlash;
        } break;
        case KeyCodeHash("NumpadEnter"): {
            Key = key::NumpadEnter;
        } break;
    }

	if (EventType == EMSCRIPTEN_EVENT_KEYDOWN) {
		SetKeyDown(Key);
	} else {
		SetKeyUp(Key);
	}
	return EM_FALSE;
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

struct thread_function_data;

struct wasm_work_queue_data {
	u32 WorkIndex;
	u32 WorkCompleted;
	u32 WorkItemCount;

	u32 ThreadCount;
	thread_callback ThreadCallback;
};

void work_queue::Create(memory_arena *Arena, thread_callback ThreadCallback, u32 Count) {
	wasm_work_queue_data *WorkQueueData = 0;
	WorkQueueData = (wasm_work_queue_data *)Arena->Push(sizeof(wasm_work_queue_data));

	u32 ThreadCount = 1;

	WorkQueueData->ThreadCallback = ThreadCallback;
	WorkQueueData->WorkIndex = 0;
	WorkQueueData->WorkCompleted = 0;
	WorkQueueData->ThreadCount = ThreadCount;
	this->OSData = WorkQueueData;
}
void work_queue::Start(u32 WorkItemCount) {
    wasm_work_queue_data *WorkQueueData = (wasm_work_queue_data *)this->OSData;
    WorkQueueData->WorkItemCount = WorkItemCount;
    WorkQueueData->WorkIndex = 0;
    WorkQueueData->WorkCompleted = 0;
}
void work_queue::Wait() {
    wasm_work_queue_data *WorkQueueData = (wasm_work_queue_data *)this->OSData;

	u32 ItemCount = WorkQueueData->WorkItemCount;
	u32 WorkEntry = WorkQueueData->WorkIndex;
	while (WorkEntry < ItemCount) {
		work_queue_context Context = {
			.WorkEntry = WorkEntry,
			.ThreadIndex = 0
		};
		WorkQueueData->ThreadCallback(&Context);
		WorkEntry += 1;
		WorkQueueData->WorkCompleted += 1;
	}
}

f64 QueryTimestampInMilliseconds() {
    return EM_ASM_DOUBLE({
		return window.performance.now();
	});
}

static EM_BOOL WindowResizeCallback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData) {
	CanvasWidth = EM_ASM_INT({ return window.innerWidth; });
	CanvasHeight = EM_ASM_INT({ return window.innerHeight; });
	return EM_TRUE;
}

int main() {
	InitializeWebGL();
	InitOSProperties();

	init_params InitParams = {0};
	OnInit(&InitParams);

	emscripten_request_animation_frame_loop(RequestAnimationFrameCallback, 0);

	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, KeyCallbackFunction);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, KeyCallbackFunction);

	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, WindowResizeCallback);
}
