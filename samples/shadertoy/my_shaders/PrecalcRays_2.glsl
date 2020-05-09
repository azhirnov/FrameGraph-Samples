// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Math.glsl"


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	if ( iFrame > 1 )
		discard;

	int2	coord		= int2(fragCoord.x, iResolution.y - fragCoord.y);
	float	dist		= float(RADIUS);
	bool	is_outer	= texelFetch( iChannel0, coord, 0 ).r < 0.25;

	for (int y = -RADIUS; y <= RADIUS; ++y)
	for (int x = -RADIUS; x <= RADIUS; ++x)
	{
		int2	pos	= coord + int2(x,y);
		
		if ( is_outer != (texelFetch( iChannel0, pos, 0 ).r < 0.25) )
		{
			dist = Min( dist, Distance( float2(coord), float2(pos) ));
		}
	}

	dist -= 0.5;

	if ( !is_outer )
		dist = -dist;

	fragColor = float4(dist);
}
