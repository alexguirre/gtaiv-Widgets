#include "Graphics.h"
#include <Hooking.Patterns.h>

namespace rage
{
	static const decltype(&grcBegin) grcBeginFn = hook::get_pattern<decltype(grcBegin)>(
		"83 3D ? ? ? ? ? 75 18 83 3D ? ? ? ? ? 75 0F 83 3D ? ? ? ? ? 0F 95 C1 E8 ? ? ? ? 56");
	static const decltype(&grcEnd) grcEndFn = hook::get_pattern<decltype(grcEnd)>(
		"83 3D ? ? ? ? ? 74 0F E8 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C3");
	static const decltype(&grcVertex) grcVertexFn =
		hook::get_pattern<decltype(grcVertex)>("83 3D ? ? ? ? ? 74 78 A1 ? ? ? ? 85 C0 74 6F");

	void grcBegin(grcDrawMode mode, uint32_t vertexCount) { grcBeginFn(mode, vertexCount); }
	void grcEnd() { grcEndFn(); }
	void grcVertex(float x,
				   float y,
				   float z,
				   float nX,
				   float nY,
				   float nZ,
				   uint32_t color,
				   float s,
				   float t)
	{
		grcVertexFn(x, y, z, nX, nY, nZ, color, s, t);
	}
}
