// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Math.glsl"
#include "SDF.glsl"
#include "RayTracing.glsl"


struct DistAndMat
{
	float	dist;
	float3	pos;
	int		mtrIndex;
};

DistAndMat  DM_Create ()
{
	DistAndMat	dm;
	dm.dist		= 1.0e+10;
	dm.pos		= float3(0.0);
	dm.mtrIndex	= -1;
	return dm;
}

DistAndMat  DM_Create (const float d, const float3 pos, const int mtr)
{
	DistAndMat	dm;
	dm.dist		= d;
	dm.pos		= pos;
	dm.mtrIndex	= mtr;
	return dm;
}

DistAndMat  DM_Min (const DistAndMat x, const DistAndMat y)
{
	return x.dist < y.dist ? x : y;
}

const int	MTR_GROUND		= 0;
const int	MTR_BUILLDING_1	= 1;


//-----------------------------------------------------------------------------
// scene

DistAndMat SDFBuilding1 (const float3 position)
{
	const float3 center		= float3(0.0, 0.0, -4.0);
	const float  height		= 4.0;
	const float  rotation	= Pi() * 0.4 / height;

	float3	pos = SDF_Move( position, center );
	pos.xz = SDF_Rotate2D( pos.xz, pos.y * rotation );

	const float  scale	= Abs(Sin( ((-pos.y*0.5 + 0.13) / height) * 2.14 + 1.0 ));
	const float	 d1		= Max( Abs(pos.x) - 1.0 * scale, Abs(pos.z) - 1.0 * scale );
	const float  d2		= Min( Abs(pos.z) - 0.2 * scale, Abs(pos.x) - 0.2 * scale );
	const float  d3		= Max( Abs(pos.x) - 0.4 * scale, Abs(pos.z) - 0.4 * scale );
	const float  d4		= Min( d3, Max(d1, d2) );
	const float  d		= SDF_OpExtrusion( pos.y, d4, height );
	return DM_Create( d, center, MTR_BUILLDING_1 );
}

DistAndMat SDFScene (const float3 pos)
{
	return SDFBuilding1( pos );
}

GEN_SDF_NORMAL_FN( SDFNormal, SDFScene, .dist )


//-----------------------------------------------------------------------------
// material

float3 MtrBuilding1 (const Ray ray, const DistAndMat dm, const float3 norm)
{
	float3	light_dir	= -ray.dir; // Normalize(float3( 0.0, 1.0, 0.2 ));

	float	shading = Dot( norm, light_dir );
	return float3(shading);
}

float4 RayTrace (in Ray ray)
{
	const int	max_iter	= 256;
	const float	min_dist	= 0.00625;
	const float	max_dist	= 100.0;
	DistAndMat	dm			= DM_Create();
	
	for (int i = 0; i < max_iter; ++i)
	{
		dm = SDFScene( ray.pos );

		Ray_Move( INOUT ray, dm.dist * 0.5 );

		if ( Abs(dm.dist) < min_dist or ray.t > max_dist )
			break;
	}

	if ( dm.dist < 0.1 )
	{
		float3	norm = SDFNormal( ray.pos );

		switch ( dm.mtrIndex )
		{
			case MTR_BUILLDING_1 :	return float4(MtrBuilding1( ray, dm, norm ), 0.0);
		}
	}
	else
	{
		// sky
		return float4(0.412, 0.796, 1.0, 0.0);
	}
}
//-----------------------------------------------------------------------------


void mainVR (out float4 fragColor, in float2 fragCoord, in float3 fragRayOri, in float3 fragRayDir)
{
	Ray	ray = Ray_Create( fragRayOri, fragRayDir, 0.1 );

	fragColor = RayTrace( ray );
}

void mainImage (out float4 fragColor, in float2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 0.1, fragCoord / iResolution.xy );

	fragColor = RayTrace( ray );
}
