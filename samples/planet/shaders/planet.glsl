// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "Math.glsl"

#define SH_VERTEX			(1 << 0)
#define SH_TESS_CONTROL		(1 << 1)
#define SH_TESS_EVALUATION	(1 << 2)
#define SH_FRAGMENT			(1 << 3)

#if SHADER & (SH_TESS_CONTROL | SH_TESS_EVALUATION | SH_FRAGMENT)
struct Material
{
	float		specular;
	float		roughtness;
	float		metallic;
};

layout(set=0, binding=0, std140) uniform un_PlanetData
{
	float4x4	viewProj;
	float4		position;
	float2		clipPlanes;
	float		tessLevel;
	float		radius;

	float3		lightDirection;	// temp

	//Material	materials [256];
} ub;
#endif
//-----------------------------------------------------------------------------



#if SHADER & SH_VERTEX
layout(location=0) in float3  at_Position;
layout(location=1) in float3  at_TextureUV;

layout(location=0) out float3  out_Texcoord;

void main ()
{
	gl_Position  = float4(at_Position.xyz, 1.0f);
	out_Texcoord = at_TextureUV.xyz;
}
#endif	// SH_VERTEX
//-----------------------------------------------------------------------------



#if USE_QUADS && (SHADER & SH_TESS_CONTROL)
layout(vertices = 4) out;

layout(location=0) in  float3  in_Texcoord[];
layout(location=0) out float3  out_Texcoord[];

void main ()
{
#	define I	gl_InvocationID
	
	if ( I == 0 ) {
		gl_TessLevelInner[0] = ub.tessLevel;
		gl_TessLevelInner[1] = ub.tessLevel;
		gl_TessLevelOuter[0] = ub.tessLevel;
		gl_TessLevelOuter[1] = ub.tessLevel;
		gl_TessLevelOuter[2] = ub.tessLevel;
		gl_TessLevelOuter[3] = ub.tessLevel;
	}
	gl_out[I].gl_Position = gl_in[I].gl_Position;
	out_Texcoord[I] = in_Texcoord[I];
}
#endif	// SH_TESS_CONTROL
//-----------------------------------------------------------------------------



#if !USE_QUADS && (SHADER & SH_TESS_CONTROL)
layout(vertices = 3) out;

layout(location=0) in  float3  in_Texcoord[];
layout(location=0) out float3  out_Texcoord[];

void main ()
{
#	define I	gl_InvocationID
	
	if ( I == 0 ) {
		gl_TessLevelInner[0] = ub.tessLevel;
		gl_TessLevelOuter[0] = ub.tessLevel;
		gl_TessLevelOuter[1] = ub.tessLevel;
		gl_TessLevelOuter[2] = ub.tessLevel;
	}
	gl_out[I].gl_Position = gl_in[I].gl_Position;
	out_Texcoord[I] = in_Texcoord[I];
}
#endif	// SH_TESS_CONTROL
//-----------------------------------------------------------------------------



#if SHADER & SH_TESS_EVALUATION
layout(set=0, binding=1) uniform samplerCube  un_HeightMap;

layout(location=0) in  float3  in_Texcoord[];
layout(location=0) out float3  out_Texcoord;

# if USE_QUADS
	layout(quads, equal_spacing, ccw) in;

	#define Interpolate( _arr_, _field_ ) \
		(mix( mix( _arr_[0] _field_, _arr_[1] _field_, gl_TessCoord.x ), \
				mix( _arr_[3] _field_, _arr_[2] _field_, gl_TessCoord.x ), \
				gl_TessCoord.y ))

# else
	layout(triangles, equal_spacing, ccw) in;

	#define Interpolate( _arr_, _field_ ) \
		( gl_TessCoord.x * _arr_[0] _field_ + \
			gl_TessCoord.y * _arr_[1] _field_ + \
			gl_TessCoord.z * _arr_[2] _field_ )

# endif	// USE_QUADS


void main ()
{
	float3	texc	= Interpolate( in_Texcoord, );
	float	height	= texture( un_HeightMap, texc ).r;
	float4	pos		= Interpolate( gl_in, .gl_Position );
	float3	surf_n	= normalize( pos.xyz );
	
	out_Texcoord = texc;
	pos.xyz		 = surf_n * ub.radius * (1.0 + height);
	gl_Position	 = ub.viewProj * (ub.position + pos);
}
#endif	// SH_TESS_EVALUATION
//-----------------------------------------------------------------------------



#if SHADER & SH_FRAGMENT
layout(location=0) out float4  out_Color;

layout(set=0, binding=2) uniform samplerCube  un_NormalMap;
layout(set=0, binding=3) uniform samplerCube  un_AlbedoMap;
layout(set=0, binding=4) uniform samplerCube  un_EmissionMap;

layout(location=0) in float3  in_Texcoord;

void main ()
{
	float3	norm	 = texture( un_NormalMap, in_Texcoord ).xyz;
	float	lighting = clamp( dot( norm, ub.lightDirection ), 0.2, 1.0 );

	//out_Color = float4( norm * 0.5 + 0.5, 1.0 );
	float3	albedo	= texture( un_AlbedoMap, in_Texcoord ).rgb;

	out_Color = float4( albedo * lighting, 1.0 );
}
#endif	// SH_FRAGMENT
//-----------------------------------------------------------------------------
