#include "WidgetManager.h"
#include "GtaThread.h"
#include <Hooking.Patterns.h>
#include <MinHook.h>
#include <imgui.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

static std::vector<Widget> gTopLevelWidgets{};
static std::unordered_map<uint32_t, std::vector<WidgetId>> gTopLevelGroupsByThreadId{};
static std::unordered_map<uint32_t, std::vector<WidgetId>> gTopLevelWidgetsByThreadId{};
static std::stack<std::vector<Widget>*> gCurrentCreationStack{ { &gTopLevelWidgets } };
static std::vector<std::string> gCurrentComboOptions{};
static std::mutex gWidgetsMutex;

static WidgetId NextId()
{
	static WidgetId last{ 0 };
	return ++last;
}

static Widget* FindWidget(std::vector<Widget>& widgets, WidgetId id)
{
	// TODO: this lookup may be too slow
	for (Widget& w : widgets)
	{
		if (w.Id() == id)
		{
			return &w;
		}
		else if (w.Type() == WidgetType::Group)
		{
			if (Widget* c = FindWidget(std::get<WidgetGroup>(w.Info()).Children, id))
			{
				return c;
			}
		}
	}

	return nullptr;
}

static Widget* FindWidget(WidgetId id)
{
	return FindWidget(gTopLevelWidgets, id);
}

static bool DeleteWidget(std::vector<Widget>& widgets, WidgetId id, bool onlyIfGroup)
{
	size_t toDeleteIndex = -1;
	for (size_t i = 0; i < widgets.size(); i++)
	{
		Widget& w = widgets[i];
		if (w.Id() == id && (onlyIfGroup == (w.Type() == WidgetType::Group)))
		{
			toDeleteIndex = i;
			break;
		}
		else if (w.Type() == WidgetType::Group)
		{
			if (DeleteWidget(std::get<WidgetGroup>(w.Info()).Children, id, onlyIfGroup))
			{
				return true;
			}
		}
	}

	if (toDeleteIndex != -1)
	{
		widgets.erase(widgets.begin() + toDeleteIndex);
		return true;
	}
	else
	{
		return false;
	}
}

static bool DeleteWidget(WidgetId id, bool onlyIfGroup)
{
	return DeleteWidget(gTopLevelWidgets, id, onlyIfGroup);
}

static void DeleteWidgetsFromThread(uint32_t threadId)
{
	const auto deleteWidgets = [threadId](auto& d, bool groups) {
		if (auto it = d.find(threadId); it != d.end())
		{
			for (WidgetId w : it->second)
			{
				DeleteWidget(w, groups);
			}

			d.erase(threadId);
		}
	};

	deleteWidgets(gTopLevelGroupsByThreadId, true);
	deleteWidgets(gTopLevelWidgetsByThreadId, false);
}

static WidgetId RegisterWidgetToThread(Widget& w)
{
	if (gCurrentCreationStack.size() == 1 && GtaThread::ms_pRunningThread)
	{
		auto& d =
			w.Type() == WidgetType::Group ? gTopLevelGroupsByThreadId : gTopLevelWidgetsByThreadId;

		const uint32_t threadId = GtaThread::ms_pRunningThread->threadId;
		if (auto it = d.find(threadId); it != d.end())
		{
			it->second.push_back(w.Id());
		}
		else
		{
			d.emplace(threadId, std::vector{ w.Id() });
		}
	}

	return w.Id(); // just return the Id for convenience when using the function
}

struct HookDummy
{
	static inline void (__thiscall HookDummy::*GtaThread_Kill_orig)() = nullptr;
	void __thiscall GtaThread_Kill_detour()
	{
		GtaThread* This = reinterpret_cast<GtaThread*>(this);
		spdlog::debug("thread:('{}', {}) killed",
					  reinterpret_cast<const char*>(This->programName),
					  This->threadId);

		{
			std::scoped_lock lock{ gWidgetsMutex };

			DeleteWidgetsFromThread(This->threadId);
		}

		(this->*GtaThread_Kill_orig)();
	}
};

void WidgetManager::Init()
{
	union
	{
		void (HookDummy::*fn)();
		void* ptr;
	} detour{ &HookDummy::GtaThread_Kill_detour };

	MH_CreateHook(hook::get_pattern("57 8B F9 8B 0D ? ? ? ? 57 8B 01 FF 50 04"),
				  detour.ptr,
				  reinterpret_cast<void**>(&HookDummy::GtaThread_Kill_orig));
}

WidgetId WidgetManager::CreateGroup(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	Widget& w =
		gCurrentCreationStack.top()->emplace_back(NextId(),
												  WidgetGroup{ std::move(label), {}, false });

	RegisterWidgetToThread(w);

	gCurrentCreationStack.push(&std::get<WidgetGroup>(w.Info()).Children);
	return w.Id();
}

void WidgetManager::EndGroup()
{
	if (gCurrentCreationStack.size() > 1)
	{
		gCurrentCreationStack.pop();
	}
}

WidgetId WidgetManager::AddSlider(std::string label, int* v, int min, int max, int step)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(gCurrentCreationStack.top()->emplace_back(
		NextId(),
		WidgetSlider{ std::move(label), v, min, max, step }));
}

WidgetId
WidgetManager::AddFloatSlider(std::string label, float* v, float min, float max, float step)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(gCurrentCreationStack.top()->emplace_back(
		NextId(),
		WidgetFloatSlider{ std::move(label), v, min, max, step }));
}

WidgetId WidgetManager::AddReadOnly(std::string label, const int* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(
		gCurrentCreationStack.top()->emplace_back(NextId(), WidgetReadOnly{ std::move(label), v }));
}

WidgetId WidgetManager::AddFloatReadOnly(std::string label, const float* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(
		gCurrentCreationStack.top()->emplace_back(NextId(),
												  WidgetFloatReadOnly{ std::move(label), v }));
}

WidgetId WidgetManager::AddToggle(std::string label, bool* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(
		gCurrentCreationStack.top()->emplace_back(NextId(), WidgetToggle{ std::move(label), v }));
}

WidgetId WidgetManager::AddString(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(
		gCurrentCreationStack.top()->emplace_back(NextId(), WidgetString{ std::move(label) }));
}

void WidgetManager::StartNewCombo()
{
	gCurrentComboOptions.clear();
}

void WidgetManager::AddToCombo(std::string option)
{
	gCurrentComboOptions.emplace_back(std::move(option));
}

WidgetId WidgetManager::FinishCombo(std::string label, int* selected)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(gCurrentCreationStack.top()->emplace_back(
		NextId(),
		WidgetCombo{ std::move(label), std::move(gCurrentComboOptions), selected }));
}

WidgetId WidgetManager::AddText(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return RegisterWidgetToThread(
		gCurrentCreationStack.top()->emplace_back(NextId(), WidgetText{ std::move(label), {} }));
}

const char* WidgetManager::GetTextContents(WidgetId id)
{
	if (Widget* w = FindWidget(id); w && w->Type() == WidgetType::Text)
	{
		return std::get<WidgetText>(w->Info()).Value.c_str();
	}

	return "";
}

void WidgetManager::SetTextContents(WidgetId id, std::string value)
{
	if (Widget* w = FindWidget(id); w && w->Type() == WidgetType::Text)
	{
		std::get<WidgetText>(w->Info()).Value = std::move(value);
	}
}

bool WidgetManager::DeleteGroup(WidgetId id)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return DeleteWidget(id, true);
}

bool WidgetManager::Delete(WidgetId id)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return DeleteWidget(id, false);
}

bool WidgetManager::DoesGroupExist(WidgetId id)
{
	if (Widget* w = FindWidget(id); w && w->Type() == WidgetType::Group)
	{
		return true;
	}

	return false;
}

void WidgetManager::DrawList()
{
	std::scoped_lock lock{ gWidgetsMutex };

	for (Widget& w : gTopLevelWidgets)
	{
		if (w.Type() == WidgetType::Group)
		{
			WidgetGroup& g = std::get<WidgetGroup>(w.Info());

			ImGui::Checkbox(g.Label.c_str(), &g.Open);
		}
	}
}

void WidgetManager::DrawWidgets()
{
	std::scoped_lock lock{ gWidgetsMutex };

	for (Widget& w : gTopLevelWidgets)
	{
		w.Draw();
	}
}