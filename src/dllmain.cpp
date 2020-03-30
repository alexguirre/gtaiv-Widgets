#include "D3D9Hook.h"
#include "D3D9ImGui.h"
#include "Graphics.h"
#include "GtaThread.h"
#include "LogWindow.h"
#include "WidgetManager.h"
#include "ioInput.h"
#include <Hooking.Patterns.h>
#include <MinHook.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <imgui.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

//#define IMMEDIATE_FLUSH

#ifdef IMMEDIATE_FLUSH
#define FLUSH_LOG() spdlog::default_logger()->flush()
#else
#define FLUSH_LOG()
#endif

// from
// https://github.com/citizenfx/fivem/blob/3be9e7423e01c5960a04e5ebd3bc1cfa89aa958d/code/components/rage-scripting-five/include/scrThread.h#L54
class scrNativeCallContext
{
	void* m_pReturn;
	uint32_t m_nArgCount;
	void* m_pArgs;

	uint32_t m_nDataCount;

	// ...
public:
	template<typename T>
	inline T GetArgument(size_t idx)
	{
		intptr_t* arguments = (intptr_t*)m_pArgs;

		return *(T*)&arguments[idx];
	}

	template<typename T>
	inline void SetResult(size_t idx, T value)
	{
		intptr_t* returnValues = (intptr_t*)m_pReturn;

		*(T*)&returnValues[idx] = value;
	}

	inline size_t GetArgumentCount() { return m_nArgCount; }
};

using scrNativeCommand = void (*)(scrNativeCallContext&);

struct NativeEntry
{
	uint32_t Hash;
	scrNativeCommand Cmd;
};

static NativeEntry*& gNatives = **hook::pattern("8B 1D ? ? ? ? 8B CF 8B 04 D3 83 F8 01 76 16")
									  .count(2)
									  .get(1)
									  .get<NativeEntry**>(2);
static uint32_t& gNativesTableSize =
	**hook::pattern("8B 35 ? ? ? ? 85 F6 75 11 FF 35 ? ? ? ? E8 ? ? ? ?")
		  .count(2)
		  .get(1)
		  .get<uint32_t*>(2);

static NativeEntry* FindNative(uint32_t hash)
{
	if (!gNativesTableSize)
		return nullptr;
	uint32_t v2 = hash % gNativesTableSize;
	uint32_t v3 = hash;
	uint32_t v4 = gNatives[hash % gNativesTableSize].Hash;
	if (v4 != hash)
	{
		while (v4)
		{
			v3 = (v3 >> 1) + 1;
			v2 = (v3 + v2) % gNativesTableSize;
			v4 = gNatives[v2].Hash;
			if (v4 == hash)
				goto LABEL_6;
		}
		return nullptr;
	}
LABEL_6:
	if (!v4)
		return nullptr;
	return &gNatives[v2];
}

static const char* GetStringArg(scrNativeCallContext& ctx, size_t idx)
{
	const char* name = ctx.GetArgument<const char*>(0);
	if (!name)
	{
		name = "<NULL>";
	}
	return name;
}

static size_t Indent{ 0 };
static constexpr uint32_t hashINIT_DEBUG_WIDGETS{ 0x73E911E8 };
static void cmdINIT_DEBUG_WIDGETS(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("INIT_DEBUG_WIDGETS() [args:{}]", ctx.GetArgumentCount());
	FLUSH_LOG();
}

static constexpr uint32_t hashCREATE_WIDGET_GROUP{ 0x558C4259 };
static void cmdCREATE_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}CREATE_WIDGET_GROUP(\"{}\") [args:{}, thread:('{}', {})]",
				 "",
				 (Indent++) * 4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgumentCount(),
				 reinterpret_cast<const char*>(GtaThread::ms_pRunningThread->programName),
				 GtaThread::ms_pRunningThread->threadId);
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::CreateGroup(GetStringArg(ctx, 0)));
}

static constexpr uint32_t hashEND_WIDGET_GROUP{ 0x6F760759 };
static void cmdEND_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}END_WIDGET_GROUP() [args:{}]", "", (--Indent) * 4, ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::EndGroup();
}

static constexpr uint32_t hashADD_WIDGET_SLIDER{ 0x4A904476 };
static void cmdADD_WIDGET_SLIDER(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_SLIDER(\"{}\", ref {}, {}, {}, {}) [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgument<int>(2),
				 ctx.GetArgument<int>(3),
				 ctx.GetArgument<int>(4),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0,
				  WidgetManager::AddSlider(GetStringArg(ctx, 0),
										   ctx.GetArgument<int*>(1),
										   ctx.GetArgument<int>(2),
										   ctx.GetArgument<int>(3),
										   ctx.GetArgument<int>(4)));
}

static constexpr uint32_t hashADD_WIDGET_FLOAT_SLIDER{ 0x6F9256DF };
static void cmdADD_WIDGET_FLOAT_SLIDER(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_FLOAT_SLIDER(\"{}\", ref {}, {}, {}, {}) [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgument<float>(2),
				 ctx.GetArgument<float>(3),
				 ctx.GetArgument<float>(4),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0,
				  WidgetManager::AddFloatSlider(GetStringArg(ctx, 0),
												ctx.GetArgument<float*>(1),
												ctx.GetArgument<float>(2),
												ctx.GetArgument<float>(3),
												ctx.GetArgument<float>(4)));
}

static constexpr uint32_t hashADD_WIDGET_READ_ONLY{ 0x4A2E3BCA };
static void cmdADD_WIDGET_READ_ONLY(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_READ_ONLY(\"{}\", ref {}) [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::AddReadOnly(GetStringArg(ctx, 0), ctx.GetArgument<int*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_FLOAT_READ_ONLY{ 0x4C8A7614 };
static void cmdADD_WIDGET_FLOAT_READ_ONLY(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_FLOAT_READ_ONLY(\"{}\", ref {}) [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(
		0,
		WidgetManager::AddFloatReadOnly(GetStringArg(ctx, 0), ctx.GetArgument<float*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_TOGGLE{ 0x66F47727 };
static void cmdADD_WIDGET_TOGGLE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_TOGGLE(\"{}\", ref {}) [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::AddToggle(GetStringArg(ctx, 0), ctx.GetArgument<bool*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_STRING{ 0x27D20F21 };
static void cmdADD_WIDGET_STRING(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_WIDGET_STRING(\"{}\") [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::AddString(GetStringArg(ctx, 0)));
}

static constexpr uint32_t hashDELETE_WIDGET_GROUP{ 0x17D72833 };
static void cmdDELETE_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("DELETE_WIDGET_GROUP({}) [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::DeleteGroup(ctx.GetArgument<WidgetId>(0));
}

static constexpr uint32_t hashDELETE_WIDGET{ 0x267D5146 };
static void cmdDELETE_WIDGET(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("DELETE_WIDGET({}) [args:{}]", ctx.GetArgument<int>(0), ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::Delete(ctx.GetArgument<WidgetId>(0));
}

static constexpr uint32_t hashDOES_WIDGET_GROUP_EXIST{ 0x3AAF5BE5 };
static void cmdDOES_WIDGET_GROUP_EXIST(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("DOES_WIDGET_GROUP_EXIST({}) [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::DoesGroupExist(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashSTART_NEW_WIDGET_COMBO{ 0x3893A3A };
static void cmdSTART_NEW_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}START_NEW_WIDGET_COMBO() [args:{}]",
				 "",
				 (Indent++) * 4,
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::StartNewCombo();
}

static constexpr uint32_t hashADD_TO_WIDGET_COMBO{ 0x4F0D4AC7 };
static void cmdADD_TO_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_TO_WIDGET_COMBO(\"{}\") [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::AddToCombo(GetStringArg(ctx, 0));
}

static constexpr uint32_t hashFINISH_WIDGET_COMBO{ 0x2CCA0D6A };
static void cmdFINISH_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}FINISH_WIDGET_COMBO(\"{}\", ref {}) [args:{}]",
				 "",
				 (--Indent) * 4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgument<void*>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::FinishCombo(GetStringArg(ctx, 0), ctx.GetArgument<int*>(1)));
}

static constexpr uint32_t hashADD_TEXT_WIDGET{ 0x7537050D };
static void cmdADD_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("{:{}}ADD_TEXT_WIDGET(\"{}\") [args:{}]",
				 "",
				 (Indent)*4,
				 GetStringArg(ctx, 0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::AddText(GetStringArg(ctx, 0)));
}

static constexpr uint32_t hashGET_CONTENTS_OF_TEXT_WIDGET{ 0x742E3376 };
static void cmdGET_CONTENTS_OF_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("GET_CONTENTS_OF_TEXT_WIDGET({}) [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	ctx.SetResult(0, WidgetManager::GetTextContents(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashSET_CONTENTS_OF_TEXT_WIDGET{ 0x6B9C6127 };
static void cmdSET_CONTENTS_OF_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SET_CONTENTS_OF_TEXT_WIDGET({}, \"{}\") [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgument<const char*>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	WidgetManager::SetTextContents(ctx.GetArgument<WidgetId>(0), ctx.GetArgument<const char*>(1));
}

static constexpr uint32_t hashSAVE_INT_TO_DEBUG_FILE{ 0x65EF0CB8 };
static void cmdSAVE_INT_TO_DEBUG_FILE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SAVE_INT_TO_DEBUG_FILE({}) [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddDebugFileLog("%d", ctx.GetArgument<int>(0));
}

static constexpr uint32_t hashSAVE_FLOAT_TO_DEBUG_FILE{ 0x66317064 };
static void cmdSAVE_FLOAT_TO_DEBUG_FILE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SAVE_FLOAT_TO_DEBUG_FILE({}) [args:{}]",
				 ctx.GetArgument<float>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddDebugFileLog("%f", ctx.GetArgument<float>(0));
}

static constexpr uint32_t hashSAVE_NEWLINE_TO_DEBUG_FILE{ 0x69D90F11 };
static void cmdSAVE_NEWLINE_TO_DEBUG_FILE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SAVE_NEWLINE_TO_DEBUG_FILE() [args:{}]", ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddDebugFileLog("\n");
}

static constexpr uint32_t hashSAVE_STRING_TO_DEBUG_FILE{ 0x27FA32D4 };
static void cmdSAVE_STRING_TO_DEBUG_FILE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SAVE_STRING_TO_DEBUG_FILE(\"{}\") [args:{}]",
				 ctx.GetArgument<const char*>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddDebugFileLog("%s", ctx.GetArgument<const char*>(0));
}

static constexpr uint32_t hashPRINTSTRING{ 0x616F492C };
static void cmdPRINTSTRING(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTSTRING(\"{}\") [args:{}]",
				 ctx.GetArgument<const char*>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("%s", ctx.GetArgument<const char*>(0));
}

static constexpr uint32_t hashPRINTFLOAT{ 0x2F206763 };
static void cmdPRINTFLOAT(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTFLOAT({}) [args:{}]", ctx.GetArgument<float>(0), ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("%f", ctx.GetArgument<float>(0));
}

static constexpr uint32_t hashPRINTFLOAT2{ 0x108A527F };
static void cmdPRINTFLOAT2(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTFLOAT2({}, {}) [args:{}]",
				 ctx.GetArgument<float>(0),
				 ctx.GetArgument<float>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("%f, %f", ctx.GetArgument<float>(0), ctx.GetArgument<float>(1));
}

static constexpr uint32_t hashPRINTINT{ 0x20421014 };
static void cmdPRINTINT(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTINT({}) [args:{}]", ctx.GetArgument<int>(0), ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("%d", ctx.GetArgument<int>(0));
}

static constexpr uint32_t hashPRINTINT2{ 0x49B35C2D };
static void cmdPRINTINT2(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTINT2({}, {}) [args:{}]",
				 ctx.GetArgument<int>(0),
				 ctx.GetArgument<int>(1),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("%d, %d", ctx.GetArgument<int>(0), ctx.GetArgument<int>(1));
}

static constexpr uint32_t hashPRINTNL{ 0x4013147B };
static void cmdPRINTNL(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTNL() [args:{}]", ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("\n");
}

static constexpr uint32_t hashPRINTVECTOR{ 0x61965EB3 };
static void cmdPRINTVECTOR(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("PRINTVECTOR({{ {}, {}, {} }}) [args:{}]",
				 ctx.GetArgument<float>(0),
				 ctx.GetArgument<float>(1),
				 ctx.GetArgument<float>(2),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("{ %f, %f, %f }",
						   ctx.GetArgument<float>(0),
						   ctx.GetArgument<float>(1),
						   ctx.GetArgument<float>(2));
}

static constexpr uint32_t hashSCRIPT_ASSERT{ 0x10C75BDA };
static void cmdSCRIPT_ASSERT(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("SCRIPT_ASSERT(\"{}\") [args:{}]",
				 ctx.GetArgument<const char*>(0),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	LogWindow::AddPrintLog("[ASSERT] %s", ctx.GetArgument<const char*>(0));
}

static void queueDrawLine(float x1, float y1, float z1, float x2, float y2, float z2);

static constexpr uint32_t hashLINE{ 0x6C6F6052 };
static void cmdLINE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("LINE({}, {}, {}, {}, {}, {}) [args:{}]",
				 ctx.GetArgument<float>(0),
				 ctx.GetArgument<float>(1),
				 ctx.GetArgument<float>(2),
				 ctx.GetArgument<float>(3),
				 ctx.GetArgument<float>(4),
				 ctx.GetArgument<float>(5),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	queueDrawLine(ctx.GetArgument<float>(0),
				  ctx.GetArgument<float>(1),
				  ctx.GetArgument<float>(2),
				  ctx.GetArgument<float>(3),
				  ctx.GetArgument<float>(4),
				  ctx.GetArgument<float>(5));
}

static void queueDrawSphere(float x, float y, float z, float radius);

static constexpr uint32_t hashDRAW_DEBUG_SPHERE{ 0x539572F3 };
static void cmdDRAW_DEBUG_SPHERE(scrNativeCallContext& ctx)
{
	SPDLOG_DEBUG("DRAW_DEBUG_SPHERE({}, {}, {}, {}, {}, {}) [args:{}]",
				 ctx.GetArgument<float>(0),
				 ctx.GetArgument<float>(1),
				 ctx.GetArgument<float>(2),
				 ctx.GetArgument<float>(3),
				 ctx.GetArgumentCount());
	FLUSH_LOG();

	queueDrawSphere(ctx.GetArgument<float>(0),
					ctx.GetArgument<float>(1),
					ctx.GetArgument<float>(2),
					ctx.GetArgument<float>(3));
}

static void ToggleDebugKeyboard(bool enable)
{
	static std::array<uint8_t*, 2> addresses = []() {
		// pattern for IS_KEYBOARD_KEY_PRESSED and IS_KEYBOARD_KEY_JUST_PRESSED
		hook::pattern p("6A 00 FF 74 24 10 B9 ? ? ? ? 32 DB E8 ? ? ? ? 0F B6 CB");
		p.count(2);
		return std::array{ p.get(0).get<uint8_t>(1), p.get(1).get<uint8_t>(1) };
	}();

	// TODO: modifying the code each time may not be too good
	for (uint8_t* v : addresses)
	{
		DWORD oldProtect;
		VirtualProtect(v, sizeof(uint8_t), PAGE_EXECUTE_READWRITE, &oldProtect);
		*v = enable ? 1 : 0;
		VirtualProtect(v, sizeof(uint8_t), oldProtect, &oldProtect);
	}
}

struct LineDrawCall
{
	float from[3];
	float to[3];
};
static std::array<LineDrawCall, 512> gLineDrawCalls{};
static size_t gLineDrawCallsCount;
static std::array<LineDrawCall, 512> gLineDrawCallsToRender{};
static size_t gLineDrawCallsToRenderCount;

struct SphereDrawCall
{
	float pos[3];
	float radius;
};
static std::array<SphereDrawCall, 512> gSphereDrawCalls{};
static size_t gSphereDrawCallsCount;
static std::array<SphereDrawCall, 512> gSphereDrawCallsToRender{};
static size_t gSphereDrawCallsToRenderCount;

static bool gDrawCommandSubmitted;

static void queueDrawLine(float x1, float y1, float z1, float x2, float y2, float z2)
{
	if (gDrawCommandSubmitted)
	{
		gLineDrawCallsCount = 0;
		gSphereDrawCallsCount = 0;
		gDrawCommandSubmitted = false;
	}

	if (gLineDrawCallsCount < gLineDrawCalls.size())
	{
		LineDrawCall& l = gLineDrawCalls[gLineDrawCallsCount];
		l.from[0] = x1;
		l.from[1] = y1;
		l.from[2] = z1;

		l.to[0] = x2;
		l.to[1] = y2;
		l.to[2] = z2;

		gLineDrawCallsCount++;
	}
}

static void queueDrawSphere(float x, float y, float z, float radius)
{
	if (gDrawCommandSubmitted)
	{
		gLineDrawCallsCount = 0;
		gSphereDrawCallsCount = 0;
		gDrawCommandSubmitted = false;
	}

	if (gSphereDrawCallsCount < gSphereDrawCalls.size())
	{
		SphereDrawCall& s = gSphereDrawCalls[gSphereDrawCallsCount];
		s.pos[0] = x;
		s.pos[1] = y;
		s.pos[2] = z;
		s.radius = radius;

		gSphereDrawCallsCount++;
	}
}

constexpr int32_t constexpr_ceil(float num)
{
	return (static_cast<float>(static_cast<int32_t>(num)) == num) ?
			   static_cast<int32_t>(num) :
			   static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
}

static void drawSphere(const SphereDrawCall& s)
{
	constexpr float Pi{ 3.141592f };
	constexpr size_t Color{ 0xA0FF0000 };

	// based on https://code.i-harness.com/en/q/754bec

	constexpr size_t Gradation{ 20 };

	for (float alpha = 0.0; alpha < Pi; alpha += Pi / Gradation)
	{
		constexpr size_t NumVertices{ constexpr_ceil((2.01f * Pi) / (Pi / Gradation)) * 2 };
		size_t n = 0;
		rage::grcBegin(rage::grcDrawMode::TriangleStrip, NumVertices);
		for (float beta = 0.0f; beta < 2.01f * Pi; beta += Pi / Gradation)
		{
			float x, y, z;
			x = s.pos[0] + s.radius * cos(beta) * sin(alpha);
			y = s.pos[1] + s.radius * sin(beta) * sin(alpha);
			z = s.pos[2] + s.radius * cos(alpha);
			rage::grcVertex(x, y, z, 0.0f, 0.0f, 0.0f, Color, 0.0f, 0.0f);
			x = s.pos[0] + s.radius * cos(beta) * sin(alpha + Pi / Gradation);
			y = s.pos[1] + s.radius * sin(beta) * sin(alpha + Pi / Gradation);
			z = s.pos[2] + s.radius * cos(alpha + Pi / Gradation);
			rage::grcVertex(x, y, z, 0.0f, 0.0f, 0.0f, Color, 0.0f, 0.0f);

			n += 2;
		}
		rage::grcEnd();
	}
}

static void (*genericCallbackDC)(void (*cb)(void));

static void (*submitMarkersDrawCommands_orig)();
static void submitMarkersDrawCommands_detour()
{
	submitMarkersDrawCommands_orig();

	memcpy(gLineDrawCallsToRender.data(),
		   gLineDrawCalls.data(),
		   sizeof(LineDrawCall) * gLineDrawCallsCount);
	gLineDrawCallsToRenderCount = gLineDrawCallsCount;

	memcpy(gSphereDrawCallsToRender.data(),
		   gSphereDrawCalls.data(),
		   sizeof(SphereDrawCall) * gSphereDrawCallsCount);
	gSphereDrawCallsToRenderCount = gSphereDrawCallsCount;

	genericCallbackDC([]() {
		constexpr uint32_t LineColor{ 0xFF0000FF };

		rage::grcBegin(rage::grcDrawMode::LineList, gLineDrawCallsToRenderCount * 2);
		for (size_t i = 0; i < gLineDrawCallsToRenderCount; i++)
		{
			const LineDrawCall& l = gLineDrawCallsToRender[i];
			// clang-format off
			rage::grcVertex(l.from[0], l.from[1], l.from[2], 0.0f, 0.0f, 1.0, LineColor, 0.0f, 0.0f);
			rage::grcVertex(  l.to[0],   l.to[1],   l.to[2], 0.0f, 0.0f, 1.0, LineColor, 0.0f, 0.0f);
			// clang-format on
		}
		rage::grcEnd();

		for (size_t i = 0; i < gSphereDrawCallsToRenderCount; i++)
		{
			drawSphere(gSphereDrawCallsToRender[i]);
		}
	});

	if (gDrawCommandSubmitted)
	{
		gLineDrawCallsCount = 0;
		gSphereDrawCallsCount = 0;
		gDrawCommandSubmitted = false;
	}
	gDrawCommandSubmitted = true;
}

static DWORD WINAPI Main(PVOID)
{
	// clang-format off
	SPDLOG_INFO("Initializing...");
	MH_Initialize();
	d3d9_hook::init(); SPDLOG_INFO(" > D3D9 Hook initialized");
	d3d9_imgui::init(); SPDLOG_INFO(" > D3D9 ImGui initialized");
	WidgetManager::Init(); SPDLOG_INFO(" > Widget Manager initialized");
	{
		genericCallbackDC = reinterpret_cast<decltype(genericCallbackDC)>(hook::get_pattern("56 6A 00 6A 0C E8 ? ? ? ? 8B F0"));

		MH_CreateHook(hook::get_pattern("81 EC ? ? ? ? 83 3D ? ? ? ? ? 0F 28 05 ? ? ? ? 53 56", -0x6),
			&submitMarkersDrawCommands_detour,
			reinterpret_cast<void**>(&submitMarkersDrawCommands_orig));
	}
	SPDLOG_INFO(" > Draw Line/Sphere Hook initialized");
	MH_EnableHook(MH_ALL_HOOKS);
	// clang-format on

	SPDLOG_INFO(" > Waiting for natives to be registered");
	while (!gNativesTableSize)
	{
		Sleep(5);
	}
	Sleep(5);

	SPDLOG_INFO(" > Replacing natives...");
	using N = std::pair<uint32_t, scrNativeCommand>;
	const auto replaceNative = [](const N& n) {
		if (NativeEntry* e = FindNative(n.first))
		{
			e->Cmd = n.second;
		}
	};

	const std::array nativesToReplace{
		N{ hashINIT_DEBUG_WIDGETS, &cmdINIT_DEBUG_WIDGETS },
		N{ hashCREATE_WIDGET_GROUP, &cmdCREATE_WIDGET_GROUP },
		N{ hashEND_WIDGET_GROUP, &cmdEND_WIDGET_GROUP },
		N{ hashADD_WIDGET_SLIDER, &cmdADD_WIDGET_SLIDER },
		N{ hashADD_WIDGET_FLOAT_SLIDER, &cmdADD_WIDGET_FLOAT_SLIDER },
		N{ hashADD_WIDGET_READ_ONLY, &cmdADD_WIDGET_READ_ONLY },
		N{ hashADD_WIDGET_FLOAT_READ_ONLY, &cmdADD_WIDGET_FLOAT_READ_ONLY },
		N{ hashADD_WIDGET_TOGGLE, &cmdADD_WIDGET_TOGGLE },
		N{ hashADD_WIDGET_STRING, &cmdADD_WIDGET_STRING },
		N{ hashDELETE_WIDGET_GROUP, &cmdDELETE_WIDGET_GROUP },
		N{ hashDELETE_WIDGET, &cmdDELETE_WIDGET },
		N{ hashDOES_WIDGET_GROUP_EXIST, &cmdDOES_WIDGET_GROUP_EXIST },
		N{ hashSTART_NEW_WIDGET_COMBO, &cmdSTART_NEW_WIDGET_COMBO },
		N{ hashADD_TO_WIDGET_COMBO, &cmdADD_TO_WIDGET_COMBO },
		N{ hashFINISH_WIDGET_COMBO, &cmdFINISH_WIDGET_COMBO },
		N{ hashADD_TEXT_WIDGET, &cmdADD_TEXT_WIDGET },
		N{ hashGET_CONTENTS_OF_TEXT_WIDGET, &cmdGET_CONTENTS_OF_TEXT_WIDGET },
		N{ hashSET_CONTENTS_OF_TEXT_WIDGET, &cmdSET_CONTENTS_OF_TEXT_WIDGET },
		N{ hashSAVE_INT_TO_DEBUG_FILE, &cmdSAVE_INT_TO_DEBUG_FILE },
		N{ hashSAVE_FLOAT_TO_DEBUG_FILE, &cmdSAVE_FLOAT_TO_DEBUG_FILE },
		N{ hashSAVE_NEWLINE_TO_DEBUG_FILE, &cmdSAVE_NEWLINE_TO_DEBUG_FILE },
		N{ hashSAVE_STRING_TO_DEBUG_FILE, &cmdSAVE_STRING_TO_DEBUG_FILE },
		N{ hashPRINTSTRING, &cmdPRINTSTRING },
		N{ hashPRINTFLOAT, &cmdPRINTFLOAT },
		N{ hashPRINTFLOAT2, &cmdPRINTFLOAT2 },
		N{ hashPRINTINT, &cmdPRINTINT },
		N{ hashPRINTINT2, &cmdPRINTINT2 },
		N{ hashPRINTNL, &cmdPRINTNL },
		N{ hashPRINTVECTOR, &cmdPRINTVECTOR },
		N{ hashSCRIPT_ASSERT, &cmdSCRIPT_ASSERT },
		N{ hashLINE, &cmdLINE },
		N{ hashDRAW_DEBUG_SPHERE, &cmdDRAW_DEBUG_SPHERE },
	};
	std::for_each(nativesToReplace.begin(), nativesToReplace.end(), replaceNative);

	SPDLOG_INFO(" > Natives replaced");

	static bool blockInput = false;
	rage::ioInput::sm_Updaters[0] = []() {
		if (ImGui::GetCurrentContext())
		{
			ImGui::GetIO().MouseWheel = rage::ioMouse::m_dZ;
		}

		if (blockInput)
		{
			memset(rage::ioKeyboard::sm_Keys, 0, sizeof(rage::ioKeyboard::sm_Keys));
			rage::ioMouse::m_LastButtons = 0;
			rage::ioMouse::m_Buttons = 0;
			rage::ioMouse::m_dX = 0;
			rage::ioMouse::m_dY = 0;
			rage::ioMouse::m_dZ = 0;
		}
	};

	d3d9_imgui::set_callback([]() {
		if (ImGui::IsKeyDown(VK_SHIFT) && ImGui::IsKeyPressed(VK_F5))
		{
			blockInput = !blockInput;
		}

		if (ImGui::Begin("Widget manager"))
		{
			ImGui::Checkbox("Logs", &LogWindow::Open);

			static bool debugKeyboard = false;
			if (ImGui::Checkbox("Debug keyboard", &debugKeyboard))
			{
				ToggleDebugKeyboard(debugKeyboard);
			}

			ImGui::Checkbox("Block keyboard&mouse game input (Shift+F5)", &blockInput);

			ImGui::Separator();

			ImGui::TextUnformatted("Widgets");
			if (ImGui::BeginChild("WidgetsFrame", ImVec2{ 0.0f, 0.0f }, true))
			{
				WidgetManager::DrawList();
			}
			ImGui::EndChild();
		}
		ImGui::End();

		LogWindow::Draw();

		WidgetManager::DrawWidgets();
	});

	SPDLOG_INFO(" > Done");

	return 0;
}

BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
					  [[maybe_unused]] DWORD ul_reason_for_call,
					  [[maybe_unused]] LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		spdlog::set_default_logger(spdlog::basic_logger_mt("file_logger", "Widgets.log"));
		spdlog::flush_every(std::chrono::seconds(30));
		spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
		if (HANDLE h = CreateThread(nullptr, 0, &Main, nullptr, 0, nullptr))
		{
			CloseHandle(h);
		}
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		d3d9_imgui::shutdown();
		spdlog::shutdown();
	}

	return TRUE;
}
