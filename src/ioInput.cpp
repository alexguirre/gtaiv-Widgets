#include "ioInput.h"
#include <Hooking.Patterns.h>

namespace rage
{
	void (*(&ioInput::sm_Updaters)[4])() =
		**hook::get_pattern<decltype(&ioInput::sm_Updaters)>("BE ? ? ? ? 8B 06 85 C0 74 02", 1);

	uint8_t (&ioKeyboard::sm_Keys)[512] = **hook::get_pattern<decltype(&ioKeyboard::sm_Keys)>(
		"81 C7 ? ? ? ? B9 ? ? ? ? BE ? ? ? ? F3 A5",
		2);

	static const hook::pattern_match ioMousePattern =
		hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 89 35 ? ? ? ? 89 3D ? ? ? ? A3 ? ? ? ? 74 11")
			.get_one();

	uint32_t& ioMouse::m_LastButtons = **ioMousePattern.get<decltype(&ioMouse::m_LastButtons)>(23);
	uint32_t& ioMouse::m_Buttons = **ioMousePattern.get<decltype(&ioMouse::m_Buttons)>(6);
	int32_t& ioMouse::m_dX = **ioMousePattern.get<decltype(&ioMouse::m_dX)>(12);
	int32_t& ioMouse::m_dY = **ioMousePattern.get<decltype(&ioMouse::m_dY)>(18);
	int32_t& ioMouse::m_dZ = **ioMousePattern.get<decltype(&ioMouse::m_dZ)>(1);
}
