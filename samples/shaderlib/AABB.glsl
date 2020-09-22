// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Axis aligned bounding box.
*/

#include "Math.glsl"


struct AABB
{
	float3	min;
	float3	max;
};

bool    AABB_IsInside (const AABB box, const float3 pos);
bool    AABB_RayIntersection (const AABB box, const float3 rayPos, const float3 rayDir, out float2 tBeginEnd);
float3  AABB_ToLocal (const AABB box, const float3 globalPos);
float3  AABB_ToLocalSNorm (const AABB box, const float3 globalPos);

float3  AABB_Center (const AABB box);
float3  AABB_Size (const AABB box);
float3  AABB_GetPointInBox (const AABB box, const float3 snormPos);

// for particles
float3  AABB_Wrap (const AABB box, const float3 pos);
float3  AABB_Clamp (const AABB box, const float3 pos);
bool    AABB_Rebound (const AABB box, inout float3 pos, inout float3 vel);


//-----------------------------------------------------------------------------



/*
=================================================
	AABB_IsInside
=================================================
*/
bool  AABB_IsInside (const AABB box, const float3 pos)
{
	return AllGreaterEqual( pos, box.min ) and AllLessEqual( pos, box.max );
}

/*
=================================================
	AABB_Center
=================================================
*/
float3  AABB_Center (const AABB box)
{
	return (box.min + box.max) * 0.5;
}

/*
=================================================
	AABB_Size
=================================================
*/
float3  AABB_Size (const AABB box)
{
	return (box.max - box.min);
}

/*
=================================================
	AABB_GetPointInBox
----
	converts AABB local snorm position to global position
=================================================
*/
float3  AABB_GetPointInBox (const AABB box, const float3 snormPos)
{
	return AABB_Center(box) + AABB_Size(box) * snormPos * 0.5;
}

/*
=================================================
	AABB_Wrap
=================================================
*/
float3  AABB_Wrap (const AABB box, const float3 pos)
{
	return Wrap( pos, box.min, box.max );
}

/*
=================================================
	AABB_Clamp
=================================================
*/
float3  AABB_Clamp (const AABB box, const float3 pos)
{
	return Clamp( pos, box.min, box.max );
}

/*
=================================================
	AABB_Rebound
=================================================
*/
bool  AABB_Rebound (const AABB box, inout float3 pos, inout float3 vel)
{
	if ( AABB_IsInside( box, pos ))
		return false;

	pos = AABB_Clamp( box, pos );
	vel = -vel;

	return true;
}

/*
=================================================
	AABB_RayIntersection
----
	returns 'true' if ray intersects with AABB,
	in 'tMinMax' returns nearest and farthest ray length to intersection points
=================================================
*/
bool  AABB_RayIntersection (const AABB box, const float3 rayPos, const float3 rayDir, out float2 tMinMax)
{
	// from https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms

	float3	dirfrac	= 1.0 / rayDir;
	float3	t135	= (box.min - rayPos) * dirfrac;
	float3	t246	= (box.max - rayPos) * dirfrac;
	float	tmin	= Max( Max( Min(t135[0], t246[0]), Min(t135[1], t246[1])), Min(t135[2], t246[2]) );
	float	tmax	= Min( Min( Max(t135[0], t246[0]), Max(t135[1], t246[1])), Max(t135[2], t246[2]) );

	if ( tmax < 0 )
		return false;

	if ( tmin > tmax )
		return false;

	tMinMax = float2( tmin, tmax );
	return true;
}

/*
=================================================
	AABB_ToLocal
----
	converts global position to AABB local position
=================================================
*/
float3  AABB_ToLocal (const AABB box, const float3 globalPos)
{
	return globalPos - AABB_Center( box );
}

float3  AABB_ToLocalSNorm (const AABB box, const float3 globalPos)
{
	return (globalPos - AABB_Center( box )) / (AABB_Size( box ) * 0.5);
}
