#include "GtaThread.h"
#include <Hooking.Patterns.h>

GtaThread*& GtaThread::ms_pRunningThread =
	**hook::get_pattern<GtaThread**>("8B 35 ? ? ? ? 8B 47 10 FF 77 08 89 74 24 30", 2);