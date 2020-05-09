// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Math.glsl"

const int	MAX_ANGLES = MAX_FRAMES * 4;


float RayMarch (const float2 origin, const int channel)
{
	const float		cone_angle	= Pi2() / MAX_ANGLES;
	const float		tan_a		= Tan( cone_angle );
	const int		max_iter	= 512;
	const float		angle		= cone_angle * float(iFrame * 4 + channel);
	const float2	dir			= SinCos( angle ).yx;
	const float2	uv_scale	= 1.0 / float2(textureSize( iChannel1, 0 ).xy - 1);
	float			t			= texture( iChannel1, float2(origin.x * uv_scale.x, 1.0 - origin.y * uv_scale.y) ).r;

	if ( t < 0.0 )
		return 0.0;

	// cone tracing
#if 0
	for (int i = 0; i < max_iter; ++i)
	{
		float	rad		= t * tan_a;
		float2	pos		= (origin + dir * t) * uv_scale;
		float	dist	= texture( iChannel1, float2(pos.x, 1.0 - pos.y) ).r;

		if ( dist < rad or AnyLess(pos, float2(0.0)) or AnyGreater(pos, float2(1.0)) )
			break;

		t += rad;
	}
#else

	// sphere tracing
	const float	min_dist = 0.00625;

	for (int i = 0; i < max_iter; ++i)
	{
		float2	pos		= (origin + dir * t) * uv_scale;
		float	dist	= texture( iChannel1, float2(pos.x, 1.0 - pos.y) ).r;

		t += dist;

		if ( Abs(dist) < min_dist or AnyLess(pos, float2(0.0)) or AnyGreater(pos, float2(1.0)) )
			break;
	}
#endif

	return t;
}


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	if ( iFrame > MAX_FRAMES+1 )
		discard;
	
	if ( (iFrame == MAX_FRAMES   and (int(fragCoord.x) & 1) == 1) or
		 (iFrame == MAX_FRAMES+1 and (int(fragCoord.x) & 1) == 0) )
	{
		// copy to the second texture
		fragCoord.y = iResolution.y - fragCoord.y;
		fragColor = texelFetch( iChannel0, int2(fragCoord), 0 );
		return;
	}

	if ( (int(fragCoord.x) & (MAX_FRAMES-1)) != iFrame )
		discard;
	
	float2	coord = float2(int(fragCoord.x) / MAX_FRAMES, int(fragCoord.y));
	
	fragColor[0] = RayMarch( coord, 0 );
	fragColor[1] = RayMarch( coord, 1 );
	fragColor[2] = RayMarch( coord, 2 );
	fragColor[3] = RayMarch( coord, 3 );
}
