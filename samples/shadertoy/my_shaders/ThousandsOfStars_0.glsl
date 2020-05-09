// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
// from https://www.shadertoy.com/view/4ldSz4

float Hash11 (float a)
{
    return fract( sin(a) * 10403.9 );
}

vec2 Hash22 (vec2 p)
{
    vec2 q = vec2( dot( p, vec2(127.1,311.7) ), 
				   dot( p, vec2(269.5,183.3) ) );
    
	return fract( sin(q) * 43758.5453 );
}

bool IsEven (int x)
{
    return x - (x / 2 * 2) == 0;
}

float Hash21 (vec2 p)
{
    return fract( sin( p.x + p.y * 64.0 ) * 104003.9 );
}

bool Hash1b (float x)
{
    return Hash11(x) > 0.5;
}


vec2 CenterOfVoronoiCell (vec2 local, vec2 global, float time)
{
    vec2 point = local + global;
    return local +
        	vec2( IsEven( int(point.y) ) ? 0.5 : 0.0, 0.0 ) +		// hex
        	(sin( time * Hash22( point ) * 0.628 ) * 0.5 + 0.5);	// animation
}

float ValueOfVoronoiCell (vec2 coord)
{
    return Hash21( coord );
}


vec2 VoronoiCircles (in vec2 coord, float freq, float time, float radiusScale)
{
    const int radius = 1;
    
    vec2 point = coord * freq;
    vec2 ipoint = floor( point );
    vec2 fpoint = fract( point );
    
    vec2 center = fpoint;
    vec2 icenter = vec2(0);
    
    float md = 2147483647.0;
	float mr = 2147483647.0;
    
	// find nearest circle
	for (int y = -radius; y <= radius; ++y)
	for (int x = -radius; x <= radius; ++x)
	{
        vec2 cur = vec2(x, y);
		vec2 c = CenterOfVoronoiCell( vec2(cur), ipoint, time );
		float d = dot( c - fpoint, c - fpoint );

		if ( d < md )
		{
			md = d;
			center = c;
			icenter = cur;
		}
	}
    
	// calc circle radius
	for (int y = -radius; y <= radius; ++y)
	for (int x = -radius; x <= radius; ++x)
	{
        if ( x == 0 && y == 0 )
            continue;
        
        vec2 cur = icenter + vec2(x, y);
		vec2 c = CenterOfVoronoiCell( vec2(cur), ipoint, time );
		float d = dot( c - fpoint, c - fpoint );
		
		if ( d < mr )
			mr = d;
	}
    
    md = sqrt( md );
	mr = sqrt( mr ) * 0.5 * radiusScale;
    
	if ( md < mr )
		return vec2( md / mr, ValueOfVoronoiCell( icenter + ipoint ) );

	return vec2( 0.0, -2.0 );
}


vec3 Mix3 (vec3 color1, vec3 color2, vec3 color3, float a)
{
    if ( a < 0.5 )
        return mix( color1, color2, a * 2.0 );
    else
        return mix( color2, color3, a * 2.0 - 1.0 );
}

vec3 StarColor (float value)
{
    return mix( vec3(1.0, 0.5, 0.0), vec3(0.0, 0.7, 1.0), value );
}

vec4 Star (vec2 value)
{
    if ( value.y < -1.0 )
        return vec4( 0.0 );
    
    float alpha;
    
    if ( Hash1b( value.y ) )
        alpha = 1.0 / (value.x * value.x * 16.0) - 0.07;
    else
        alpha = 1.0 / (value.x * 8.0) - 0.15;
    
    return clamp( vec4( Mix3( vec3(0.0), StarColor( value.y ), vec3(1.0), alpha ), alpha ), 0.0, 1.0 );
}


vec4 StarLayers (vec2 coord, float time)
{
    vec4 color = Star( VoronoiCircles( coord, 1.25, time * 0.07274, 0.5 ) );
    color += Star( VoronoiCircles( coord + vec2(2.372), 10.0, time * 0.229 + 2.496, 0.75 ) );
    color += Star( VoronoiCircles( coord + vec2(6.518), 30.0, time * 0.57 + 8.513, 0.9 ) ) * 0.85;
    color += Star( VoronoiCircles( coord + vec2(3.584), 60.0, time + 2.649, 1.0 ) ) * 0.75;
    color += Star( VoronoiCircles( coord + vec2(0.493), 60.0, time + 8.624, 1.0 ) ) * 0.75;
    
    return color;
}


void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / min( iResolution.x, iResolution.y );
    
    uv += vec2(1.0) * iTime * 0.01;
    
    fragColor = StarLayers( uv, iTime + 412.54 );
}
