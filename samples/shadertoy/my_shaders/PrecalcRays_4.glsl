
#include "Math.glsl"

void mainImage (out float4 fragColor, in float2 fragCoord)
{
	const float2	uv			= fragCoord / iResolution.xy;
	const float2	tex_size	= float2(textureSize( iChannel0, 0 ).xy - 1);
	const float2	origin		= tex_size * uv;
	const int2		texc		= int2(origin.x, tex_size.y - origin.y + 0.5);
	const float2	light_pos	= tex_size * float2(0.5); //*/ * (SinCos( iTime * 0.25 ) * 0.35 + 0.5);
	const float		tmax		= Max( Length( light_pos - origin ) - 0.01, 0.01 );
	const float2	dir			= Normalize( light_pos - origin );
	
	fragColor = float4(0.0);	
	fragColor.b = texelFetch( iChannel0, texc, 0 ).r;

	float	angle	= ATan( dir.y, dir.x );
	angle = angle < 0.0 ? Pi2() + angle : angle;

	const int	max_angles	= ANGLE_COUNT * 4 - 1;
	const int	index		= int(max_angles * angle / Pi2() + 0.5) & max_angles;
	const int2	texc2		= int2( texc.x * ANGLE_COUNT + (index >> 2), texc.y );
	float		t			= texelFetch( iChannel1, texc2, 0 )[index & 3];
	
	if ( t > tmax ) {
		fragColor.r = 10.1 / tmax;
		fragColor.g = 0.1 / tmax;
	}
	//fragColor.r = t * 0.1;
}
