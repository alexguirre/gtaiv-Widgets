#include "WidgetManager.h"
#include <imgui.h>
#include <mutex>
#include <stack>
#include <utility>
#include <vector>

static std::vector<Widget> gTopLevelWidgets{};
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

WidgetId WidgetManager::CreateGroup(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	Widget& w =
		gCurrentCreationStack.top()->emplace_back(NextId(),
												  WidgetGroup{ std::move(label), {}, false });
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

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetSlider{ std::move(label), v, min, max, step })
		.Id();
}

WidgetId
WidgetManager::AddFloatSlider(std::string label, float* v, float min, float max, float step)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetFloatSlider{ std::move(label), v, min, max, step })
		.Id();
}

WidgetId WidgetManager::AddReadOnly(std::string label, const int* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetReadOnly{ std::move(label), v })
		.Id();
}

WidgetId WidgetManager::AddFloatReadOnly(std::string label, const float* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetFloatReadOnly{ std::move(label), v })
		.Id();
}

WidgetId WidgetManager::AddToggle(std::string label, bool* v)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetToggle{ std::move(label), v })
		.Id();
}

WidgetId WidgetManager::AddString(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetString{ std::move(label) })
		.Id();
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

	return gCurrentCreationStack.top()
		->emplace_back(NextId(),
					   WidgetCombo{ std::move(label), std::move(gCurrentComboOptions), selected })
		.Id();
}

WidgetId WidgetManager::AddText(std::string label)
{
	std::scoped_lock lock{ gWidgetsMutex };

	return gCurrentCreationStack.top()
		->emplace_back(NextId(), WidgetText{ std::move(label), {} })
		.Id();
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

bool WidgetManager::DeleteGroup(WidgetId id) // TODO: WidgetManager::DeleteGroup
{
	return false;
}

bool WidgetManager::Delete(WidgetId id) // TODO: WidgetManager::Delete
{
	return false;
}

bool WidgetManager::DoesGroupExist(WidgetId id)
{
	if (Widget* w = FindWidget(id); w && w->Type() == WidgetType::Group)
	{
		return true;
	}

	return false;
}

void WidgetManager::Draw()
{
	std::scoped_lock lock{ gWidgetsMutex };

	if (ImGui::Begin("Widget manager"))
	{
		for (Widget& w : gTopLevelWidgets)
		{
			if (w.Type() == WidgetType::Group)
			{
				WidgetGroup& g = std::get<WidgetGroup>(w.Info());

				ImGui::Checkbox(g.Label.c_str(), &g.Open);
			}
		}
	}
	ImGui::End();

	for (Widget& w : gTopLevelWidgets)
	{
		w.Draw();
	}
}