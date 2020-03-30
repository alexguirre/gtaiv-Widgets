#pragma once
#include <cstdint>

namespace rage
{
	enum class grcDrawMode : uint32_t
	{
		PointList = 0,
		LineList = 1,
		LineStrip = 2,
		TriangleList = 3,
		TriangleStrip = 4,
		TriangleFan = 5,
		LineList2 = 6,
	};

	void grcBegin(grcDrawMode mode, uint32_t vertexCount);
	void grcEnd();
	void grcVertex(float x,
				   float y,
				   float z,
				   float nX,
				   float nY,
				   float nZ,
				   uint32_t color,
				   float s,
				   float t);
}
