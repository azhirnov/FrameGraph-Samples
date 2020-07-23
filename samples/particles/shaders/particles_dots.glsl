// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define SH_VERTEX			(1 << 0)
#define SH_TESS_CONTROL		(1 << 1)
#define SH_TESS_EVALUATION	(1 << 2)
#define SH_GEOMETRY			(1 << 3)
#define SH_FRAGMENT			(1 << 4)

#include "Math.glsl"

layout(set=0, binding=0, std140) uniform CameraUB {
	float4x4		proj;
	float4x4		modelView;
	float4x4		modelViewProj;
	float2			viewport;
	float2			clipPlanes;
} ub;
//-----------------------------------------------------------------------------


#if SHADER & SH_VERTEX
	layout(location=0) in  float3	in_Position;
	layout(location=1) in  float3	in_Velocity;
	layout(location=2) in  float4	in_Color;
	layout(location=3) in  float	in_Size;

	layout(location=0) out float4	out_Color;
	layout(location=1) out float	out_Size;

	void main ()
	{
		gl_Position		= ub.modelView * float4(in_Position, 1.0);
		out_Color		= in_Color;
		out_Size		= in_Size * 4.0 / Max( ub.viewport.x, ub.viewport.y );
	}
#endif	// SH_VERTEX
//-----------------------------------------------------------------------------



#if SHADER & SH_GEOMETRY
	layout (points) in;
	layout (triangle_strip, max_vertices = 4) out;
	
	layout(location=0) in  float4	in_Color[];
	layout(location=1) in  float	in_Size[];
	
	layout(location=0) out float2	out_UV;
	layout(location=1) out float4	out_Color;


	void main ()
	{
		float4	pos		= gl_in[0].gl_Position;
		float	size	= in_Size[0];


		// a: left-bottom
		float2	va	= pos.xy + float2(-0.5, -0.5) * size;
		gl_Position	= ub.proj * float4(va, pos.zw);
		out_UV		= float2(0.0, 0.0);
		out_Color	= in_Color[0];
		EmitVertex();

		// b: left-top
		float2	vb	= pos.xy + float2(-0.5, 0.5) * size;
		gl_Position	= ub.proj * float4(vb, pos.zw);
		out_UV		= float2(0.0, 1.0);
		out_Color	= in_Color[0];
		EmitVertex();

		// d: right-bottom
		float2	vd	= pos.xy + float2(0.5, -0.5) * size;
		gl_Position	= ub.proj * float4(vd, pos.zw);
		out_UV		= float2(1.0, 0.0);
		out_Color	= in_Color[0];
		EmitVertex();

		// c: right-top
		float2	vc	= pos.xy + float2(0.5, 0.5) * size;
		gl_Position	= ub.proj * float4(vc, pos.zw);
		out_UV		= float2(1.0, 1.0);
		out_Color	= in_Color[0];
		EmitVertex();

		EndPrimitive();
	}
#endif	// SH_GEOMETRY
//-----------------------------------------------------------------------------


#if SHADER & SH_FRAGMENT
	layout(location=0) out float4  out_Color;
	
	layout(location=0) in  float2	in_UV;
	layout(location=1) in  float4	in_Color;

	void main ()
	{
		out_Color = in_Color * (1.0 - Distance( ToSNorm(in_UV), float2(0.0) ));
	}
#endif	// SH_FRAGMENT
//-----------------------------------------------------------------------------
