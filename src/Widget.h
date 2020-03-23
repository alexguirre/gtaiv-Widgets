#pragma once
#include <limits>
#include <string>
#include <variant>
#include <vector>

enum class WidgetType
{
	Invalid = 0,
	Group,
	Slider,
	FloatSlider,
	ReadOnly,
	FloatReadOnly,
	Toggle,
	String,
	Combo,
	Text,
};

struct Widget;

struct WidgetGroup
{
	std::string Label{};
	std::vector<Widget> Children{};
	bool Open{ false };
};

template<class T>
struct WidgetSliderBase
{
	std::string Label{};
	T* Value{ nullptr };
	T Min{ std::numeric_limits<T>::min() };
	T Max{ std::numeric_limits<T>::max() };
	T Step{ 1 };
};

using WidgetSlider = WidgetSliderBase<int>;
using WidgetFloatSlider = WidgetSliderBase<float>;

template<class T>
struct WidgetReadOnlyBase
{
	std::string Label{};
	const T* Value{ nullptr };
};

using WidgetReadOnly = WidgetReadOnlyBase<int>;
using WidgetFloatReadOnly = WidgetReadOnlyBase<float>;

struct WidgetToggle
{
	std::string Label{};
	bool* Value{ nullptr };
};

struct WidgetString
{
	std::string Label{};
};

struct WidgetCombo
{
	std::string Label{};
	std::vector<std::string> Options{};
	int* Selected{ nullptr };
};

struct WidgetText
{
	std::string Label{};
	std::string Value{};
};

using WidgetInfo = std::variant<std::monostate,
								WidgetGroup,
								WidgetSlider,
								WidgetFloatSlider,
								WidgetReadOnly,
								WidgetFloatReadOnly,
								WidgetToggle,
								WidgetString,
								WidgetCombo,
								WidgetText>;

using WidgetId = uint32_t;

class Widget
{
public:
	Widget(WidgetId id, WidgetInfo info);
	Widget(const Widget&) = delete;
	Widget(Widget&&) = default;

	Widget& operator=(const Widget&) = delete;
	Widget& operator=(Widget&&) = default;

	void Draw();

	inline WidgetId Id() const { return mId; }
	inline WidgetType Type() const { return mType; }
	inline const WidgetInfo& Info() const { return mInfo; }
	inline WidgetInfo& Info() { return mInfo; }

private:
	WidgetId mId{ 0 };
	WidgetType mType{ WidgetType::Invalid };
	WidgetInfo mInfo{};

	static WidgetType GetInfoType(const WidgetInfo&);
};
