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


layout(push_constant, std140) uniform PushConst {
	int2	faceDim;
	int		face;
} pc;

// @discard
layout(set=0, binding=0) writeonly restrict uniform image2D  un_OutHeight;

// @discard
layout(set=0, binding=1) writeonly restrict uniform image2D  un_OutNormal;


float FBM (in float3 coord)
{
	float	total		= 0.0;
	float	amplitude	= 1.0;
	float	freq		= 1.0;

	for (int i = 0; i < 7; ++i)
	{
		total 		+= GradientNoise( coord*freq ) * amplitude;
		freq 		*= 2.5;
		amplitude 	*= 0.5;
	}
	return total;
}


float4 GetPosition (const int2 coord)
{
	float2	ncoord	= ToSNorm( float2(coord) / float2(pc.faceDim - 1) );
	float3	pos		= PROJECTION( ncoord, pc.face );
	float	height	= FBM( pos ) * 0.04;
	return float4( pos, height );
}


// positions with 1 pixel border for normals calculation
shared float3  s_Positions[ gl_WorkGroupSize.x * gl_WorkGroupSize.y ];

float3  ReadPosition (int2 local)
{
	local += 1;
	return s_Positions[ local.x + local.y * gl_WorkGroupSize.x ];
}


void main ()
{
	const int2		local		= GetLocalCoord().xy - 1;
	const int2		lsize		= GetLocalSize().xy - 2;
	const int2		group		= GetGroupCoord().xy;
	const int2		coord		= local + lsize * group;
	const float4	pos_h		= GetPosition( coord );
	const float3	pos			= pos_h.xyz * (1.0 + pos_h.w);
	const bool4		is_active	= bool4( greaterThanEqual( local, int2(0) ), lessThan( local, lsize ));

	s_Positions[ gl_LocalInvocationIndex ] = pos;
	memoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease );

	if ( All( is_active ))
	{
		barrier();
		memoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquire );

		const int3		offset	= int3(-1, 0, 1);
		const float3	v0		= ReadPosition( local + offset.xx );
		const float3	v1		= ReadPosition( local + offset.yx );
		const float3	v2		= ReadPosition( local + offset.zx );
		const float3	v3		= ReadPosition( local + offset.xy );
		const float3	v4		= pos;
		const float3	v5		= ReadPosition( local + offset.zy );
		const float3	v6		= ReadPosition( local + offset.xz );
		const float3	v7		= ReadPosition( local + offset.yz );
		const float3	v8		= ReadPosition( local + offset.zz );
		float3			normal	= float3(0.0);

		normal += Cross( v1 - v4, v2 - v4 );	// 1-4, 2-4
		normal += Cross( v5 - v4, v8 - v4 );	// 5-4, 8-4
		normal += Cross( v7 - v4, v6 - v4 );	// 7-4, 6-4
		normal += Cross( v3 - v4, v0 - v4 );	// 3-4, 0-4
		normal  = Normalize( normal );

		imageStore( un_OutHeight, coord, float4(pos_h.w) );
		imageStore( un_OutNormal, coord, float4(normal, 0.0) );
	}
}
