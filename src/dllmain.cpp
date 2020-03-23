#include "D3D9Hook.h"
#include "D3D9ImGui.h"
#include "WidgetManager.h"
#include <Hooking.Patterns.h>
#include <MinHook.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <imgui.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

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

static size_t Indent{ 0 };
static constexpr uint32_t hashINIT_DEBUG_WIDGETS{ 0x73E911E8 };
static void cmdINIT_DEBUG_WIDGETS(scrNativeCallContext& ctx)
{
	spdlog::debug("INIT_DEBUG_WIDGETS() [args:{}]", ctx.GetArgumentCount());
}

static constexpr uint32_t hashCREATE_WIDGET_GROUP{ 0x558C4259 };
static void cmdCREATE_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}CREATE_WIDGET_GROUP(\"{}\") [args:{}]",
				  "",
				  (Indent++) * 4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::CreateGroup(ctx.GetArgument<const char*>(0)));
}

static constexpr uint32_t hashEND_WIDGET_GROUP{ 0x6F760759 };
static void cmdEND_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}END_WIDGET_GROUP() [args:{}]", "", (--Indent) * 4, ctx.GetArgumentCount());

	WidgetManager::EndGroup();
}

static constexpr uint32_t hashADD_WIDGET_SLIDER{ 0x4A904476 };
static void cmdADD_WIDGET_SLIDER(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_SLIDER(\"{}\", ref {}, {}, {}, {}) [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgument<int>(2),
				  ctx.GetArgument<int>(3),
				  ctx.GetArgument<int>(4),
				  ctx.GetArgumentCount());

	ctx.SetResult(0,
				  WidgetManager::AddSlider(ctx.GetArgument<const char*>(0),
										   ctx.GetArgument<int*>(1),
										   ctx.GetArgument<int>(2),
										   ctx.GetArgument<int>(3),
										   ctx.GetArgument<int>(4)));
}

static constexpr uint32_t hashADD_WIDGET_FLOAT_SLIDER{ 0x6F9256DF };
static void cmdADD_WIDGET_FLOAT_SLIDER(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_FLOAT_SLIDER(\"{}\", ref {}, {}, {}, {}) [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgument<float>(2),
				  ctx.GetArgument<float>(3),
				  ctx.GetArgument<float>(4),
				  ctx.GetArgumentCount());

	ctx.SetResult(0,
				  WidgetManager::AddFloatSlider(ctx.GetArgument<const char*>(0),
												ctx.GetArgument<float*>(1),
												ctx.GetArgument<float>(2),
												ctx.GetArgument<float>(3),
												ctx.GetArgument<float>(4)));
}

static constexpr uint32_t hashADD_WIDGET_READ_ONLY{ 0x4A2E3BCA };
static void cmdADD_WIDGET_READ_ONLY(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_READ_ONLY(\"{}\", ref {}) [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgumentCount());

	ctx.SetResult(
		0,
		WidgetManager::AddReadOnly(ctx.GetArgument<const char*>(0), ctx.GetArgument<int*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_FLOAT_READ_ONLY{ 0x4C8A7614 };
static void cmdADD_WIDGET_FLOAT_READ_ONLY(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_FLOAT_READ_ONLY(\"{}\", ref {}) [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgumentCount());

	ctx.SetResult(0,
				  WidgetManager::AddFloatReadOnly(ctx.GetArgument<const char*>(0),
												  ctx.GetArgument<float*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_TOGGLE{ 0x66F47727 };
static void cmdADD_WIDGET_TOGGLE(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_TOGGLE(\"{}\", ref {}) [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgumentCount());

	ctx.SetResult(
		0,
		WidgetManager::AddToggle(ctx.GetArgument<const char*>(0), ctx.GetArgument<bool*>(1)));
}

static constexpr uint32_t hashADD_WIDGET_STRING{ 0x27D20F21 };
static void cmdADD_WIDGET_STRING(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_WIDGET_STRING(\"{}\") [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::AddString(ctx.GetArgument<const char*>(0)));
}

static constexpr uint32_t hashDELETE_WIDGET_GROUP{ 0x17D72833 };
static void cmdDELETE_WIDGET_GROUP(scrNativeCallContext& ctx)
{
	spdlog::debug("DELETE_WIDGET_GROUP({}) [args:{}]",
				  ctx.GetArgument<int>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::DeleteGroup(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashDELETE_WIDGET{ 0x267D5146 };
static void cmdDELETE_WIDGET(scrNativeCallContext& ctx)
{
	spdlog::debug("DELETE_WIDGET({}) [args:{}]", ctx.GetArgument<int>(0), ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::Delete(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashDOES_WIDGET_GROUP_EXIST{ 0x3AAF5BE5 };
static void cmdDOES_WIDGET_GROUP_EXIST(scrNativeCallContext& ctx)
{
	spdlog::debug("DOES_WIDGET_GROUP_EXIST({}) [args:{}]",
				  ctx.GetArgument<int>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::DoesGroupExist(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashSTART_NEW_WIDGET_COMBO{ 0x3893A3A };
static void cmdSTART_NEW_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}START_NEW_WIDGET_COMBO() [args:{}]",
				  "",
				  (Indent++) * 4,
				  ctx.GetArgumentCount());

	WidgetManager::StartNewCombo();
}

static constexpr uint32_t hashADD_TO_WIDGET_COMBO{ 0x4F0D4AC7 };
static void cmdADD_TO_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_TO_WIDGET_COMBO(\"{}\") [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgumentCount());

	WidgetManager::AddToCombo(ctx.GetArgument<const char*>(0));
}

static constexpr uint32_t hashFINISH_WIDGET_COMBO{ 0x2CCA0D6A };
static void cmdFINISH_WIDGET_COMBO(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}FINISH_WIDGET_COMBO(\"{}\", ref {}) [args:{}]",
				  "",
				  (--Indent) * 4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgument<void*>(1),
				  ctx.GetArgumentCount());

	ctx.SetResult(
		0,
		WidgetManager::FinishCombo(ctx.GetArgument<const char*>(0), ctx.GetArgument<int*>(1)));
}

static constexpr uint32_t hashADD_TEXT_WIDGET{ 0x7537050D };
static void cmdADD_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	spdlog::debug("{:{}}ADD_TEXT_WIDGET(\"{}\") [args:{}]",
				  "",
				  (Indent)*4,
				  ctx.GetArgument<const char*>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::AddText(ctx.GetArgument<const char*>(0)));
}

static constexpr uint32_t hashGET_CONTENTS_OF_TEXT_WIDGET{ 0x742E3376 };
static void cmdGET_CONTENTS_OF_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	spdlog::debug("GET_CONTENTS_OF_TEXT_WIDGET({}) [args:{}]",
				  ctx.GetArgument<int>(0),
				  ctx.GetArgumentCount());

	ctx.SetResult(0, WidgetManager::GetTextContents(ctx.GetArgument<WidgetId>(0)));
}

static constexpr uint32_t hashSET_CONTENTS_OF_TEXT_WIDGET{ 0x6B9C6127 };
static void cmdSET_CONTENTS_OF_TEXT_WIDGET(scrNativeCallContext& ctx)
{
	spdlog::debug("SET_CONTENTS_OF_TEXT_WIDGET({}, \"{}\") [args:{}]",
				  ctx.GetArgument<int>(0),
				  ctx.GetArgument<const char*>(1),
				  ctx.GetArgumentCount());

	WidgetManager::SetTextContents(ctx.GetArgument<WidgetId>(0), ctx.GetArgument<const char*>(1));
}

static DWORD WINAPI Main(PVOID)
{
	MH_Initialize();
	d3d9_hook::init();
	d3d9_imgui::init();
	MH_EnableHook(MH_ALL_HOOKS);

	while (!gNativesTableSize)
	{
		Sleep(5);
	}
	Sleep(5);

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
		N{ hashSET_CONTENTS_OF_TEXT_WIDGET, &cmdSET_CONTENTS_OF_TEXT_WIDGET }
	};
	std::for_each(nativesToReplace.begin(), nativesToReplace.end(), replaceNative);

	d3d9_imgui::set_callback([]() { WidgetManager::Draw(); });

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
		spdlog::set_level(spdlog::level::debug);
		if (HANDLE h = CreateThread(nullptr, 0, &Main, nullptr, 0, nullptr))
		{
			CloseHandle(h);
		}
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		spdlog::shutdown();
	}

	return TRUE;
}
