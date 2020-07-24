// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#extension GL_GOOGLE_include_directive : require
#extension GL_KHR_memory_scope_semantics : require
#extension GL_EXT_control_flow_attributes : require

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;

#include "GlobalIndex.glsl"
#include "SDF.glsl"
#include "CubeMap.glsl"
#include "Hash.glsl"
#include "Noise.glsl"
#include "Color.glsl"

layout(push_constant, std140) uniform PushConst {
	int2	faceDim;
	int		face;
} pc;

// @discard
layout(set=0, binding=0) writeonly restrict uniform image2D  un_OutAlbedo;

// @discard
layout(set=0, binding=1) writeonly restrict uniform image2D  un_OutEmission;

layout(set=0, binding=2, r16f)    readonly  restrict uniform image2D  un_HeightMap;
layout(set=0, binding=3, rgba16f) readonly  restrict uniform image2D  un_NormalMap;


shared float3  s_Positions[ gl_WorkGroupSize.x * gl_WorkGroupSize.y ];
shared float3  s_Normals  [ gl_WorkGroupSize.x * gl_WorkGroupSize.y ];


void main ()
{
	const int2	coord	= GetGlobalCoord().xy;
	
	// read height map
	float3	sphere_pos;
	{
		float	height	= imageLoad( un_HeightMap, coord ).r;
		float3	norm	= imageLoad( un_NormalMap, coord ).rgb;
		float2	ncoord	= ToSNorm( float2(coord) / float2(pc.faceDim - 1) );
		sphere_pos		= PROJECTION( ncoord, pc.face );
	
		s_Positions[ gl_LocalInvocationIndex ] = sphere_pos * (1.0 + height);
		s_Normals[ gl_LocalInvocationIndex ]   = norm;
		memoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease );
	}

	barrier();
	memoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquire );


	float3	albedo		= float3(1.0);
	float	emission	= 0.0;
	float	temperature	= 0.0;
	float3	pos			= s_Positions[ gl_LocalInvocationIndex ];

	float	biom		= DHash13( Voronoi( Turbulence( sphere_pos * 8.0, 1.0, 2.0, 0.6, 7 ), float2(3.9672) ).icenter );
	int		mtr_id		= int(biom * 255.0f) & 0xF;

	//albedo = (mtr_id == 0 ? float3(0.0, 0.0, 1.0) : float3(0.0));
	albedo = HSVtoRGB( float3( biom, 1.0, 1.0 ));

	imageStore( un_OutAlbedo, coord, float4(albedo, 0.0) );
	imageStore( un_OutEmission, coord, float4(emission, temperature, 0.0, 0.0) );
}
