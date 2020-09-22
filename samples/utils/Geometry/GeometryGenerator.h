// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/GLM.h"

namespace FGC
{

	//
	// Geometry Generator
	//

	struct GeometryGenerator
	{
		static bool  CreateGrid (OUT Array<float2> &vertices, OUT Array<uint> &indices, uint numVertInSide, uint patchSize = 3, float scale = 1.0f);

		struct CubeVertex {
			float3		position;
			float3		normal;
			float3		texcoord;
		};
		static bool  CreateCube (OUT Array<CubeVertex> &vertices, OUT Array<uint> &indices);
	};

}	// FGC
