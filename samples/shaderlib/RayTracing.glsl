// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Ray
*/

#include "Math.glsl"
#include "Quaternion.glsl"


struct Ray
{
	float3	origin;		// camera (eye, light, ...) position
	float3	dir;		// normalized direction
	float3	pos;		// current position
	float	t;
};


/*
=================================================
	Ray_Create
=================================================
*/
Ray  Ray_Create (const float3 origin, const float3 direction, const float tmin)
{
	Ray	ray;
	ray.origin	= origin;
	ray.t		= tmin;
	ray.dir		= direction;
	ray.pos		= FusedMulAdd( ray.dir, float3(ray.t), ray.origin );
	return ray;
}

/*
=================================================
	Ray_FromScreen
----
	create ray for raytracing, raymarching, ...
=================================================
*/
Ray  Ray_FromScreen (const float3 origin, const quat rotation, const float fovX, const float nearPlane,
					 const int2 screenSize, const int2 screenCoord)
{
	float2	scr_size	= float2(screenSize);
	float2	coord		= float2(screenCoord);

	float	ratio		= scr_size.y / scr_size.x;
	float 	fovY 		= fovX * ratio;
	float2 	scale		= nearPlane / Cos( float2(fovX, fovY) * 0.5 );
	float2 	uv 			= (coord - scr_size * 0.5) / (scr_size.x * 0.5) * scale;

	Ray	ray;
	ray.origin	= origin;
	ray.dir		= Normalize( QMul( rotation, Normalize( float3(uv.x, -uv.y, -0.5) )));
	ray.t		= nearPlane;
	ray.pos		= FusedMulAdd( ray.dir, float3(ray.t), ray.origin );

	return ray;
}

/*
=================================================
	Ray_From
----
	create ray from frustum rays and origin
=================================================
*/
Ray  Ray_From (const float3 leftBottom, const float3 rightBottom, const float3 leftTop, const float3 rightTop,
			   const float3 origin, const float nearPlane, const float2 unormCoord)
{
	const float2 coord	= unormCoord;
	const float3 vec	= Lerp( Lerp( leftBottom, rightBottom, coord.x ),
								Lerp( leftTop, rightTop, coord.x ),
								coord.y );

	Ray	ray;
	ray.origin	= origin;
	ray.dir		= Normalize( vec );
	ray.t		= nearPlane;
	ray.pos		= FusedMulAdd( ray.dir, float3(ray.t), ray.origin );

	return ray;
}

/*
=================================================
	Ray_CalcX
=================================================
*/
float3  Ray_CalcX (const Ray ray, const float2 pointYZ)
{
	const float	x = ray.pos.x + ray.dir.x * (pointYZ[1] - ray.pos.z) / ray.dir.z;

	return float3( x, pointYZ[0], pointYZ[1] );
}

/*
=================================================
	Ray_CalcY
=================================================
*/
float3  Ray_CalcY (const Ray ray, const float2 pointXZ)
{
	const float	y = ray.pos.y + ray.dir.y * (pointXZ[1] - ray.pos.z) / ray.dir.z;

	return float3( pointXZ[0], y, pointXZ[1] );
}

/*
=================================================
	Ray_CalcZ
=================================================
*/
float3  Ray_CalcZ (const Ray ray, const float2 pointXY)
{
	const float	z = ray.pos.z + ray.dir.z * (pointXY[0] - ray.pos.x) / ray.dir.x;

	return float3( pointXY[0], pointXY[1], z );
}

/*
=================================================
	Ray_Contains
=================================================
*/
bool  Ray_Contains (const Ray ray, const float3 point)
{
	// z(x), z(y)
	const float2	z = ray.pos.zz + ray.dir.zz * (point.xy - ray.pos.xy) / ray.dir.xy;

	// z(x) == z(y) and z(x) == point.z
	return Equals( z.x, z.y ) and Equals( z.x, point.z );
}

/*
=================================================
	Ray_Rotate
=================================================
*/
void  Ray_Rotate (inout Ray ray, const quat rotation)
{
	// ray.origin - const
	ray.dir = Normalize( QMul( rotation, ray.dir ));
	ray.t	= Distance( ray.origin, ray.pos );
	ray.pos	= ray.t * ray.dir;
}

/*
=================================================
	Ray_Move
=================================================
*/
void  Ray_Move (inout Ray ray, const float length)
{
	ray.t   += length;
	ray.pos  = FusedMulAdd( ray.dir, float3(ray.t), ray.origin );
}

/*
=================================================
	Ray_SetLength
=================================================
*/
void  Ray_SetLength (inout Ray ray, const float length)
{
	ray.t   = length;
	ray.pos = FusedMulAdd( ray.dir, float3(length), ray.origin );
}

/*
=================================================
	Ray_SetOrigin
=================================================
*/
void  Ray_SetOrigin (inout Ray ray, const float3 origin)
{
	ray.origin	= origin;
	ray.pos		= FusedMulAdd( ray.dir, float3(ray.t), origin );
}
