#include "Widget.h"
#include <imgui.h>
#include <imgui_internal.h>
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
	std::visit(
		overloaded{
			[](const auto&) {},
			[](WidgetGroup& w) {
				const bool isChild = ImGui::GetCurrentContext()->CurrentWindowStack.Size > 1;
				if (isChild)
				{
					if (ImGui::CollapsingHeader(w.Label.c_str(), &w.Open))
					{
						for (Widget& c : w.Children)
						{
							c.Draw();
						}
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
			[](const WidgetSlider& w) { ImGui::SliderInt(w.Label.c_str(), w.Value, w.Min, w.Max); },
			[](const WidgetFloatSlider& w) {
				ImGui::SliderFloat(w.Label.c_str(), w.Value, w.Min, w.Max);
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
