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
	constexpr char CanvasName[] = "#canvas";
	f64 Width, Height = 0;
	emscripten_get_element_css_size(CanvasName, &Width, &Height);
	CanvasWidth = EM_ASM_INT({ return window.innerWidth; });
	CanvasHeight = EM_ASM_INT({ return window.innerHeight; });
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

static EM_BOOL KeyCallbackFunction(int EventType, const EmscriptenKeyboardEvent *Event, void *) {

	emscripten_log(EM_LOG_CONSOLE, "%d\n", EventType);

	return EM_TRUE;
}

bool IsDown(key Key) {
	return false;
}
bool IsUp(key Key) {
	return false;
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

int main() {
	InitializeWebGL();
	InitOSProperties();

	init_params InitParams = {0};
	OnInit(&InitParams);

	emscripten_request_animation_frame_loop(RequestAnimationFrameCallback, 0);

	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, KeyCallbackFunction);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, EM_FALSE, KeyCallbackFunction);
}
