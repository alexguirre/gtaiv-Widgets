#pragma once
#include "Widget.h"

class WidgetManager
{
public:
	static void Init();

	static WidgetId CreateGroup(std::string label);
	static void EndGroup();
	static WidgetId AddSlider(std::string label, int* v, int min, int max, int step);
	static WidgetId AddFloatSlider(std::string label, float* v, float min, float max, float step);
	static WidgetId AddReadOnly(std::string label, const int* v);
	static WidgetId AddFloatReadOnly(std::string label, const float* v);
	static WidgetId AddToggle(std::string label, bool* v);
	static WidgetId AddString(std::string label);
	static void StartNewCombo();
	static void AddToCombo(std::string option);
	static WidgetId FinishCombo(std::string label, int* selected);
	static WidgetId AddText(std::string label);
	static const char* GetTextContents(WidgetId id);
	static void SetTextContents(WidgetId id, std::string value);

	static bool DeleteGroup(WidgetId id);
	static bool Delete(WidgetId id);
	static bool DoesGroupExist(WidgetId id);

	static void DrawList();
	static void DrawWidgets();
};
