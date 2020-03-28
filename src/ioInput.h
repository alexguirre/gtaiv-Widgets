#pragma once
#include <cstdint>

namespace rage
{
	class ioInput
	{
	public:
		static void (*(&sm_Updaters)[4])();
	};

	class ioKeyboard
	{
	public:
		static uint8_t (&sm_Keys)[512];
	};

	class ioMouse
	{
	public:
		static uint32_t& m_LastButtons;
		static uint32_t& m_Buttons;
		static int32_t& m_dX;
		static int32_t& m_dY;
		static int32_t& m_dZ;
	};
}
