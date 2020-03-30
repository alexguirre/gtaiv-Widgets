#include "Widget.h"
#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <type_traits>
#include <utility>

namespace ImGui
{
	struct InputTextCallback_UserData
	{
		std::string* Str;
		ImGuiInputTextCallback ChainCallback;
		void* ChainCallbackUserData;
	};

	static int InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we
			// need to set them back to what we want.
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		}
		else if (user_data->ChainCallback)
		{
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	}

	static bool InputText(const char* label,
						  std::string* str,
						  ImGuiInputTextFlags flags = 0,
						  ImGuiInputTextCallback callback = nullptr,
						  void* user_data = nullptr)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str = str;
		cb_user_data.ChainCallback = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return InputText(label,
						 (char*)str->c_str(),
						 str->capacity() + 1,
						 flags,
						 InputTextCallback,
						 &cb_user_data);
	}

	/// Combines a Slider with +/- buttons, based on ImGui::InputScalar
	template<class T>
	static bool InputSlider(const char* label, T* v, T min, T max, T step)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		if constexpr (std::is_same_v<T, int>)
		{
			// see assert in ImGui::SliderBehavior
			if (min < (INT_MIN / 2) || max > (INT_MAX / 2))
			{
				// fallback to just InputInt without slider
				int tmp = *v;
				if (InputInt(label, &tmp, step, step))
				{
					*v = std::clamp(tmp, min, max);
					return true;
				}
				return false;
			}
		}

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;

		const float buttonSide = GetFrameHeight();
		const ImVec2 buttonSize{ buttonSide, buttonSide };

		BeginGroup();
		PushID(v);
		SetNextItemWidth(
			ImMax(1.0f, CalcItemWidth() - (buttonSide + style.ItemInnerSpacing.x) * 2));
		bool valueChanged = false;
		if constexpr (std::is_same_v<T, float>)
		{
			float tmp = *v;
			if (SliderFloat("", &tmp, min, max))
			{
				*v = std::clamp(tmp, min, max);
				valueChanged = true;
			}
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			int tmp = *v;
			if (SliderInt("", &tmp, min, max))
			{
				*v = std::clamp(tmp, min, max);
				valueChanged = true;
			}
		}

		// Step buttons
		const ImVec2 backupFramePadding = style.FramePadding;
		style.FramePadding.x = style.FramePadding.y;
		constexpr ImGuiButtonFlags buttonFlags =
			ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
		SameLine(0, style.ItemInnerSpacing.x);
		if (ButtonEx("-", buttonSize, buttonFlags | (*v == min ? ImGuiButtonFlags_Disabled : 0)))
		{
			*v = std::clamp(*v - step, min, max);
			valueChanged = true;
		}
		SameLine(0, style.ItemInnerSpacing.x);
		if (ButtonEx("+", buttonSize, buttonFlags | (*v == max ? ImGuiButtonFlags_Disabled : 0)))
		{
			*v = std::clamp(*v + step, min, max);
			valueChanged = true;
		}

		const char* labelEnd = FindRenderedTextEnd(label);
		if (label != labelEnd)
		{
			SameLine(0, style.ItemInnerSpacing.x);
			TextEx(label, labelEnd);
		}
		style.FramePadding = backupFramePadding;

		PopID();
		EndGroup();

		return valueChanged;
	}
}

// helper for std::visit
template<class... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

Widget::Widget(WidgetId id, WidgetInfo info)
	: mId{ id }, mType{ GetInfoType(info) }, mInfo{ std::move(info) }
{
}

void Widget::Draw()
{
	ImGui::PushID(this);
	std::visit(overloaded{
				   [](const auto&) {},
				   [](WidgetGroup& w) {
					   const bool isChild = ImGui::GetCurrentContext()->CurrentWindowStack.Size > 1;
					   if (isChild)
					   {
						   if (ImGui::CollapsingHeader(w.Label.c_str()))
						   {
							   ImGui::Indent();
							   for (Widget& c : w.Children)
							   {
								   c.Draw();
							   }
							   ImGui::Unindent();
						   }
					   }
					   else if (w.Open)
					   {
						   if (ImGui::Begin(w.Label.c_str(), &w.Open))
						   {
							   for (Widget& c : w.Children)
							   {
								   c.Draw();
							   }
						   }
						   ImGui::End();
					   }
				   },
				   [](const WidgetSlider& w) {
					   ImGui::InputSlider<int>(w.Label.c_str(), w.Value, w.Min, w.Max, w.Step);
				   },
				   [](const WidgetFloatSlider& w) {
					   ImGui::InputSlider<float>(w.Label.c_str(), w.Value, w.Min, w.Max, w.Step);
				   },
				   [](const WidgetReadOnly& w) {
					   ImGui::InputInt(w.Label.c_str(),
									   const_cast<int*>(w.Value),
									   0,
									   0,
									   ImGuiInputTextFlags_ReadOnly);
				   },
				   [](const WidgetFloatReadOnly& w) {
					   ImGui::InputFloat(w.Label.c_str(),
										 const_cast<float*>(w.Value),
										 0.0f,
										 0.0f,
										 0,
										 ImGuiInputTextFlags_ReadOnly);
				   },
				   [](const WidgetToggle& w) { ImGui::Checkbox(w.Label.c_str(), w.Value); },
				   [](const WidgetString& w) { ImGui::Text(w.Label.c_str()); },
				   [](const WidgetCombo& w) {
					   if (ImGui::BeginCombo(w.Label.c_str(),
											 (*w.Selected >= 0 && *w.Selected < w.Options.size()) ?
												 w.Options[*w.Selected].c_str() :
												 ""))
					   {
						   for (size_t i = 0; i < w.Options.size(); i++)
						   {
							   ImGui::PushID(i);
							   const bool isSelected = (i == *w.Selected);
							   if (ImGui::Selectable(w.Options[i].c_str(), isSelected))
							   {
								   *w.Selected = static_cast<int>(i);
							   }
							   if (isSelected)
								   ImGui::SetItemDefaultFocus();
							   ImGui::PopID();
						   }
						   ImGui::EndCombo();
					   }
				   },
				   [](WidgetText& w) { ImGui::InputText(w.Label.c_str(), &w.Value); },
			   },
			   mInfo);
	ImGui::PopID();
}

WidgetType Widget::GetInfoType(const WidgetInfo& info)
{
	return std::visit(overloaded{
						  [](const auto&) { return WidgetType::Invalid; },
						  [](const WidgetGroup&) { return WidgetType::Group; },
						  [](const WidgetSlider&) { return WidgetType::Slider; },
						  [](const WidgetFloatSlider&) { return WidgetType::FloatSlider; },
						  [](const WidgetReadOnly&) { return WidgetType::ReadOnly; },
						  [](const WidgetFloatReadOnly&) { return WidgetType::FloatReadOnly; },
						  [](const WidgetToggle&) { return WidgetType::Toggle; },
						  [](const WidgetString&) { return WidgetType::String; },
						  [](const WidgetCombo&) { return WidgetType::Combo; },
						  [](const WidgetText&) { return WidgetType::Text; },
					  },
					  info);
}
