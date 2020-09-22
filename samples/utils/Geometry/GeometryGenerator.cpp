// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GeometryGenerator.h"

namespace FGC
{
	
/*
=================================================
	CreateGrid
=================================================
*/
	bool  GeometryGenerator::CreateGrid (OUT Array<float2> &vertices, OUT Array<uint> &indices, uint numVertInSide, uint patchSize, float scale)
	{
		CHECK_ERR( numVertInSide > 1 );
		CHECK_ERR( scale > 1.0e-5f );
		CHECK_ERR( patchSize == 3 or patchSize == 4 or patchSize == 16 );

		vertices.clear();
		indices.clear();
		vertices.reserve( numVertInSide * numVertInSide * 3 );
		indices.reserve( numVertInSide * numVertInSide * 4 );

		const float	vert_scale = scale / float(numVertInSide - 1);

		for (uint y = 0; y < numVertInSide; ++y)
		{
			for (uint x = 0; x < numVertInSide; ++x)
			{
				vertices.push_back( float2{uint2{ x, y }} * vert_scale );

				if ( x != 0 and y != 0 )
				{
					if ( patchSize == 4 )
					{
						indices.push_back( (y + 0) * numVertInSide  + (x + 0) );
						indices.push_back( (y + 0) * numVertInSide  + (x - 1) );
						indices.push_back( (y - 1) * numVertInSide  + (x - 1) );
						indices.push_back( (y - 1) * numVertInSide  + (x + 0) );
					}
					else
					if ( patchSize == 3 )
					{
						indices.push_back( (y + 0) * numVertInSide  + (x + 0) );
						indices.push_back( (y + 0) * numVertInSide  + (x - 1) );
						indices.push_back( (y - 1) * numVertInSide  + (x - 1) );

						indices.push_back( (y - 1) * numVertInSide  + (x - 1) );
						indices.push_back( (y - 1) * numVertInSide  + (x + 0) );
						indices.push_back( (y + 0) * numVertInSide  + (x + 0) );
					}
					else
					if ( patchSize == 16 and (x % 3) == 0 and (y % 3) == 0 )
					{
						for (uint i = 0; i < 4; ++i)
						for (uint j = 0; j < 4; ++j)
							indices.push_back( (y + i - 3) * numVertInSide + (x + j - 3) );
					}
				}
			}
		}

		return true;
	}
	
/*
=================================================
	CreateCube
=================================================
*/
	bool  GeometryGenerator::CreateCube (OUT Array<CubeVertex> &vertices, OUT Array<uint> &indices)
	{
		static const float	a_vertices[] = {
			-0.5f, -0.5f, -0.5f,	-0.5f,  0.5f, -0.5f,	 0.5f,  0.5f, -0.5f,	
			 0.5f, -0.5f, -0.5f,	-0.5f, -0.5f,  0.5f,	 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,	-0.5f,  0.5f,  0.5f,	-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,	 0.5f, -0.5f,  0.5f,	-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,	 0.5f,  0.5f, -0.5f,	 0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,	 0.5f,  0.5f, -0.5f,	-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,	 0.5f,  0.5f,  0.5f,	-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,	-0.5f, -0.5f,  0.5f,	-0.5f,  0.5f,  0.5f
		};

		static const float	a_normals[] = {
			0.0f, 0.0f, -1.0f,		0.0f, 0.0f, -1.0f,	 0.0f,  0.0f, -1.0f,	 0.0f,  0.0f, -1.0f,	 0.0f,  0.0f, 1.0f,		 0.0f,  0.0f, 1.0f,
			0.0f, 0.0f,  1.0f,		0.0f, 0.0f,  1.0f,	 0.0f, -1.0f,  0.0f,	 0.0f, -1.0f,  0.0f,	 0.0f, -1.0f, 0.0f,		 0.0f, -1.0f, 0.0f,
			1.0f, 0.0f,  0.0f,		1.0f, 0.0f,  0.0f,	 1.0f,  0.0f,  0.0f,	 1.0f,  0.0f,  0.0f,	 0.0f,  1.0f, 0.0f,		 0.0f,  1.0f, 0.0f,
			0.0f, 1.0f,  0.0f,		0.0f, 1.0f,  0.0f,	-1.0f,  0.0f,  0.0f,	-1.0f,  0.0f,  0.0f,	-1.0f,  0.0f, 0.0f,		-1.0f,  0.0f, 0.0f
		};

		static const float a_texcoords[] = {
			1.0f, 1.0f, 0.0f,	1.0f, 0.0f, 0.0f,	0.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 1.0f,	0.0f, 1.0f, 0.0f,	1.0f, 1.0f, 1.0f,
			1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 1.0f,	1.0f, 0.0f, 1.0f,	0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 1.0f,	1.0f, 1.0f, 1.0f,	1.0f, 0.0f, 1.0f,
			0.0f, 0.0f, 1.0f,	0.0f, 1.0f, 1.0f,	1.0f, 1.0f, 0.0f,
			1.0f, 0.0f, 0.0f,	0.0f, 0.0f, 1.0f,	0.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 0.0f,	1.0f, 0.0f, 0.0f,	0.0f, 0.0f, 0.0f
		};

		static const uint	a_indices[] = {
			0, 1, 2,      2, 3, 0,      4, 5, 6,      6, 7, 4,
			8, 9, 10,     10, 11, 8,    12, 13, 14,   14, 15, 12,
			16, 17, 18,   18, 19, 16,   20, 21, 22,   22, 23, 20
		};

		STATIC_ASSERT( CountOf(a_vertices) == CountOf(a_normals) );
		STATIC_ASSERT( CountOf(a_vertices) == CountOf(a_texcoords) );

		vertices.resize( CountOf(a_vertices) );

		for (size_t i = 0; i < vertices.size(); ++i)
		{
			vertices[i] = CubeVertex{
				float3{ a_vertices[i*3+0],	a_vertices[i*3+1],	a_vertices[i*3+2]	},
				float3{ a_normals[i*3+0],	a_normals[i*3+1],	a_normals[i*3+2]	},
				float3{ a_texcoords[i*3+0],	a_texcoords[i*3+1],	a_texcoords[i*3+2]	}};
		}
		indices.assign( a_indices, a_indices + CountOf(a_indices) );
		return true;
	}

}	// FGC
