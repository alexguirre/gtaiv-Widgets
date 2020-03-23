#pragma once
#include "Widget.h"

class WidgetManager
{
public:
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

	static void Draw();
};

/*
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
*/
