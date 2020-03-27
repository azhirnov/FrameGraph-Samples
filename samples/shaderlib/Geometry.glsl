/*
	Geometry functions
*/

#include "Math.glsl"


void  GetRayPerpendicular (const float3 dir, out float3 outLeft, out float3 outUp)
{
	const float3	a	 = Abs( dir );
	const float2	c	 = float2( 1.0f, 0.0f );
	const float3	axis = a.x < a.y ? (a.x < a.z ? c.xyy : c.yyx) :
									   (a.y < a.z ? c.xyx : c.yyx);
		
	outLeft = Normalize( Cross( dir, axis ));
	outUp   = Normalize( Cross( dir, outLeft ));
}


float  ToLinearDepth (float nonLinearDepth, const float2 clipPlanes)
{
	//float  d = 2.0 * nonLinearDepth - 1.0;	// for OpenGL
	float d = nonLinearDepth;
	d = 2.0 * clipPlanes.x * clipPlanes.y / (clipPlanes.y + clipPlanes.x - d * (clipPlanes.y - clipPlanes.x));
	return d;
}

float  ToNonlinearDepth (float linearDepth, const float2 clipPlanes)
{
	float d = (clipPlanes.y + clipPlanes.x - 2.0 * clipPlanes.x * clipPlanes.y / linearDepth) / (clipPlanes.y - clipPlanes.x);
	//d = (d + 1.0) * 0.5;	// for OpenGL
	return d;
}
