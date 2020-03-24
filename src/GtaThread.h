#pragma once
#include <cstdint>

class GtaThread
{
public:
	void* vtable;
	uint32_t threadId;
	uint8_t padding[0x70 - 0x8];
	char programName[24];
	// ...

	static GtaThread*& ms_pRunningThread;
};
