// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Spherical.h"
#include "SphericalCubeMath.h"

namespace FG
{

	//
	// Spherical Cube Projection
	//

	template <typename PosProj, typename TexProj>
	class SphericalCubeProjection
	{
	// types
	public:
		using Projection_t			= PosProj;
		using TextureProjection_t	= TexProj;


	// methods
	public:
		// position projection
		ND_ static double3  ForwardProjection (const double2 &ncoord, ECubeFace face);
		ND_ static float3   ForwardProjection (const float2 &ncoord, ECubeFace face);

		ND_ static Pair<double2, ECubeFace>  InverseProjection (const double3 &coord);
		ND_ static Pair<float2, ECubeFace>   InverseProjection (const float3 &coord);
		
		// texture coordinate projection
		ND_ static double3  ForwardTexProjection (const double2 &ncoord, ECubeFace face);
		ND_ static float3   ForwardTexProjection (const float2 &ncoord, ECubeFace face);

		ND_ static Pair<double2, ECubeFace>  InverseTexProjection (const double3 &coord);
		ND_ static Pair<float2, ECubeFace>   InverseTexProjection (const float3 &coord);
	};



	//
	// Spherical Cube
	//

	class SphericalCube final : public SphericalCubeProjection< TangentialSphericalCube, TextureProjection >
	{
	// types
	public:
		struct Vertex
		{
			float3		position;	// in normalized coords
			float		_padding1;
			float3		texcoord;
			float		_padding2;
			//float3	tangent;	// TODO: use it for distortion correction and for tessellation

			Vertex () {}
			Vertex (const float3 &pos, const float3 &texc) : position{pos}, texcoord{texc} {}
		};


	// variables
	private:
		BufferID		_vertexBuffer;
		BufferID		_indexBuffer;
		uint			_minLod			= 0;
		uint			_maxLod			= 0;
		bool			_quads			= false;


	// methods
	public:
		SphericalCube () {}
		~SphericalCube ();

		bool Create (const CommandBuffer &cmdbuf, uint minLod, uint maxLod, bool quads);
		void Destroy (const FrameGraph &fg);

		ND_ DrawIndexed  Draw (uint lod) const;

			bool GetVertexBuffer (uint lod, uint face, OUT RawBufferID &id, OUT BytesU &offset, OUT BytesU &size, OUT uint2 &vertCount) const;

		ND_ bool RayCast (const float3 &center, float radius, const float3 &begin, const float3 &end, OUT float3 &outIntersection) const;

		ND_ static VertexInputState	GetAttribs ();
	};

	

/*
=================================================
	ForwardProjection
=================================================
*/
	template <typename PP, typename TP>
	inline double3  SphericalCubeProjection<PP,TP>::ForwardProjection (const double2 &ncoord, ECubeFace face)
	{
		return Projection_t::Forward( ncoord, face );
	}
	
	template <typename PP, typename TP>
	inline float3  SphericalCubeProjection<PP,TP>::ForwardProjection (const float2 &ncoord, ECubeFace face)
	{
		return float3(Projection_t::Forward( double2(ncoord), face ));
	}
	
/*
=================================================
	InverseProjection
=================================================
*/
	template <typename PP, typename TP>
	inline Pair<double2, ECubeFace>  SphericalCubeProjection<PP,TP>::InverseProjection (const double3 &coord)
	{
		auto[c, face] = Projection_t::Inverse( coord );
		return { c, face };
	}
	
	template <typename PP, typename TP>
	inline Pair<float2, ECubeFace>  SphericalCubeProjection<PP,TP>::InverseProjection (const float3 &coord)
	{
		auto[c, face] = Projection_t::Inverse( double3(coord) );
		return { float2(c), face };
	}
	
/*
=================================================
	ForwardTexProjection
=================================================
*/
	template <typename PP, typename TP>
	inline double3  SphericalCubeProjection<PP,TP>::ForwardTexProjection (const double2 &ncoord, ECubeFace face)
	{
		return TextureProjection_t::Forward( ncoord, face );
	}
	
	template <typename PP, typename TP>
	inline float3  SphericalCubeProjection<PP,TP>::ForwardTexProjection (const float2 &ncoord, ECubeFace face)
	{
		return float3(TextureProjection_t::Forward( double2(ncoord), face ));
	}

/*
=================================================
	InverseTexProjection
=================================================
*/
	template <typename PP, typename TP>
	inline Pair<double2, ECubeFace>  SphericalCubeProjection<PP,TP>::InverseTexProjection (const double3 &coord)
	{
		auto[c, face] = TextureProjection_t::Inverse( coord );
		return { c, face };
	}
	
	template <typename PP, typename TP>
	inline Pair<float2, ECubeFace>  SphericalCubeProjection<PP,TP>::InverseTexProjection (const float3 &coord)
	{
		auto[c, face] = TextureProjection_t::Inverse( double3(coord) );
		return { float2(c), face };
	}


}	// FG
