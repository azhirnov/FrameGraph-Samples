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

	layout(location=0) out float4	out_StartPos;
	layout(location=1) out float4	out_EndPos;
	layout(location=2) out float4	out_Color;
	layout(location=3) out float	out_Size;

	void main ()
	{
		out_EndPos		= ub.modelView * float4(in_Position, 1.0);
		out_Color		= in_Color;
		out_Size		= in_Size * 2.0 / Max( ub.viewport.x, ub.viewport.y );

		float3	v		= Normalize(in_Velocity) * Min( Length(in_Velocity), out_Size * 25.0 );
		out_StartPos	= ub.modelView * float4(in_Position - v * 0.5, 1.0);
	}
#endif	// SH_VERTEX
//-----------------------------------------------------------------------------



#if SHADER & SH_GEOMETRY
	layout (points) in;
	layout (triangle_strip, max_vertices = 4) out;
	
	layout(location=0) in  float4	in_StartPos[];
	layout(location=1) in  float4	in_EndPos[];
	layout(location=2) in  float4	in_Color[];
	layout(location=3) in  float	in_Size[];
	
	layout(location=0) out float2	out_UV;
	layout(location=1) out float4	out_Color;


	// check is point inside oriented rectangle
	bool IsPointInside (in float2 a, in float2 b, in float2 c, in float2 m)
	{
		float2	ab		= b - a;
		float2	bc		= c - b;
		float2	am		= m - a;
		float2	bm		= m - b;

		float	ab_am	= Dot( ab, am );
		float	ab_ab	= Dot( ab, ab );
		float	bc_bm	= Dot( bc, bm );
		float	bc_bc	= Dot( bc, bc );

		return	ab_am >= 0.0f  and bc_bm >= 0.0f and
				ab_am <= ab_ab and bc_bm <= bc_bc;
	}

	void main ()
	{
		//        _ _
		//      /\ / \		    \ /
		//     /  s  /		     s
		//    /  / \/		    / \
		//   /\ /  /		 \ /
		//  \  e  /			  e
		//   \/_\/			 / \
		
		float4	start	= in_StartPos[0];
		float4	end		= in_EndPos[0];
		float2	dir		= Normalize( end.xy - start.xy );
		float2	norm	= float2( -dir.y, dir.x );

		float	size	= in_Size[0] * 0.5;	// rotated rectangle size
		float	side	= size * 0.95;			// size with error
		float4	color	= in_Color[0];
		
		float2	rect_a	= start.xy + norm * size;
		float2	rect_b	= start.xy - norm * size;
		float2	rect_c	= end.xy - norm * size;

		float4	points[]	= float4[](
			start + float4( norm + dir, 0.0, 0.0) * side,
			start + float4( norm - dir, 0.0, 0.0) * side,
			start + float4(-norm - dir, 0.0, 0.0) * side,
			start + float4(-norm + dir, 0.0, 0.0) * side,
			end   + float4( norm + dir, 0.0, 0.0) * side,
			end   + float4( norm - dir, 0.0, 0.0) * side,
			end   + float4(-norm - dir, 0.0, 0.0) * side,
			end   + float4(-norm + dir, 0.0, 0.0) * side
		);

		const float2	uv_coords[] = float2[](
			float2(0.0, 1.0),
			float2(0.0, 0.0),
			float2(1.0, 1.0),
			float2(1.0, 0.0)
		);

		// find external points (must be 4 points)
		for (int i = 0, j = 0; i < points.length() and j < 4; ++i)
		{
			if ( not IsPointInside( rect_a, rect_b, rect_c, points[i].xy ))
			{
				gl_Position	 = ub.proj * points[i];
				out_UV		 = uv_coords[j];
				out_Color	 = color;

				EmitVertex();
				++j;
			}
		}

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
