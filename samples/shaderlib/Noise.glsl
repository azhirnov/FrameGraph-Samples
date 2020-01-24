/*
	Noise functions
*/

#include "Hash.glsl"


// range [-1..1]
float GradientNoise (sampler2D rgbaNoise, const float3 pos)
{
	// from https://www.shadertoy.com/view/4dffRH
	// The MIT License
	// Copyright © 2017 Inigo Quilez
#	define hash( _p_ )	ToSNorm( textureLod( rgbaNoise, (_p_).xy * 0.01 + (_p_).z * float2(0.01723059, 0.053092949), 0.0 ).rgb )
	
	// grid
	float3 i = Floor(pos);
	float3 w = Fract(pos);
	
	#if 1
	// quintic interpolant
	float3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
	float3 du = 30.0*w*w*(w*(w-2.0)+1.0);
	#else
	// cubic interpolant
	float3 u = w*w*(3.0-2.0*w);
	float3 du = 6.0*w*(1.0-w);
	#endif    
	
	// gradients
	float3 ga = hash( i+float3(0.0,0.0,0.0) );
	float3 gb = hash( i+float3(1.0,0.0,0.0) );
	float3 gc = hash( i+float3(0.0,1.0,0.0) );
	float3 gd = hash( i+float3(1.0,1.0,0.0) );
	float3 ge = hash( i+float3(0.0,0.0,1.0) );
	float3 gf = hash( i+float3(1.0,0.0,1.0) );
	float3 gg = hash( i+float3(0.0,1.0,1.0) );
	float3 gh = hash( i+float3(1.0,1.0,1.0) );
	
	// projections
	float va = Dot( ga, w-float3(0.0,0.0,0.0) );
	float vb = Dot( gb, w-float3(1.0,0.0,0.0) );
	float vc = Dot( gc, w-float3(0.0,1.0,0.0) );
	float vd = Dot( gd, w-float3(1.0,1.0,0.0) );
	float ve = Dot( ge, w-float3(0.0,0.0,1.0) );
	float vf = Dot( gf, w-float3(1.0,0.0,1.0) );
	float vg = Dot( gg, w-float3(0.0,1.0,1.0) );
	float vh = Dot( gh, w-float3(1.0,1.0,1.0) );
	
	// interpolations
	return 	va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) + u.x*u.y*(va-vb-vc+vd) +
			u.y*u.z*(va-vc-ve+vg) + u.z*u.x*(va-vb-ve+vf) + (-va+vb+vc-vd+ve-vf-vg+vh)*u.x*u.y*u.z;
#	undef hash
}

float GradientNoise (const float3 pos)
{
	// from https://www.shadertoy.com/view/4dffRH
	// The MIT License
	// Copyright © 2017 Inigo Quilez
#	define hash( _p_ )	ToSNorm( DHash33( _p_ ))
	
	// grid
	float3 i = Floor(pos);
	float3 w = Fract(pos);
	
	#if 1
	// quintic interpolant
	float3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
	float3 du = 30.0*w*w*(w*(w-2.0)+1.0);
	#else
	// cubic interpolant
	float3 u = w*w*(3.0-2.0*w);
	float3 du = 6.0*w*(1.0-w);
	#endif    
	
	// gradients
	float3 ga = hash( i+float3(0.0,0.0,0.0) );
	float3 gb = hash( i+float3(1.0,0.0,0.0) );
	float3 gc = hash( i+float3(0.0,1.0,0.0) );
	float3 gd = hash( i+float3(1.0,1.0,0.0) );
	float3 ge = hash( i+float3(0.0,0.0,1.0) );
	float3 gf = hash( i+float3(1.0,0.0,1.0) );
	float3 gg = hash( i+float3(0.0,1.0,1.0) );
	float3 gh = hash( i+float3(1.0,1.0,1.0) );
	
	// projections
	float va = Dot( ga, w-float3(0.0,0.0,0.0) );
	float vb = Dot( gb, w-float3(1.0,0.0,0.0) );
	float vc = Dot( gc, w-float3(0.0,1.0,0.0) );
	float vd = Dot( gd, w-float3(1.0,1.0,0.0) );
	float ve = Dot( ge, w-float3(0.0,0.0,1.0) );
	float vf = Dot( gf, w-float3(1.0,0.0,1.0) );
	float vg = Dot( gg, w-float3(0.0,1.0,1.0) );
	float vh = Dot( gh, w-float3(1.0,1.0,1.0) );
	
	// interpolations
	return 	va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) + u.x*u.y*(va-vb-vc+vd) +
			u.y*u.z*(va-vc-ve+vg) + u.z*u.x*(va-vb-ve+vf) + (-va+vb+vc-vd+ve-vf-vg+vh)*u.x*u.y*u.z;
#	undef hash
}
//-----------------------------------------------------------------------------


// range [-1..1]
float IQNoise (sampler2D rgbaNoise, const float3 pos, float u, float v)
{
	// from https://www.shadertoy.com/view/Xd23Dh
	// The MIT License
	// Copyright © 2014 Inigo Quilez
#	define hash( _p_ )	textureLod( rgbaNoise, (_p_).xy * 0.01 + (_p_).z * float2(0.01723059, 0.053092949), 0.0 ).rgb

	float3 p = Floor(pos);
	float3 f = Fract(pos);
	
	float k = 1.0 + 63.0 * Pow( 1.0 - v, 6.0 );
	
	float va = 0.0;
	float wt = 0.0;
	for (int z = -2; z <= 2; ++z)
	for (int y = -2; y <= 2; ++y)
	for (int x = -2; x <= 2; ++x)
	{
		float3 g = float3( float(x),float(y), float(z) );
		float3 o = hash( p + g ) * float3(u,u,1.0);
		float3 r = g - f + o;
		float d = Dot( r, r );
		float ww = Pow( 1.0 - SmoothStep(0.0,1.414, Sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
	}
	
	return va/wt;
#	undef hash
}

float IQNoise (const float3 pos, float u, float v)
{
	// from https://www.shadertoy.com/view/Xd23Dh
	// The MIT License
	// Copyright © 2014 Inigo Quilez
#	define hash( _p_ )	DHash33( _p_ )

	float3 p = Floor(pos);
	float3 f = Fract(pos);
	
	float k = 1.0 + 63.0 * Pow( 1.0 - v, 6.0 );
	
	float va = 0.0;
	float wt = 0.0;
	for (int z = -2; z <= 2; ++z)
	for (int y = -2; y <= 2; ++y)
	for (int x = -2; x <= 2; ++x)
	{
		float3 g = float3( float(x),float(y), float(z) );
		float3 o = hash( p + g ) * float3(u,u,1.0);
		float3 r = g - f + o;
		float d = Dot( r, r );
		float ww = Pow( 1.0 - SmoothStep(0.0,1.414, Sqrt(d)), k );
		va += o.z*ww;
		wt += ww;
	}
	
	return va/wt;
#	undef hash
}
//-----------------------------------------------------------------------------


float ValueNoise (sampler2D greyNoise, const float3 pos)
{
#	define hash( _p_ )	ToSNorm( textureLod( greyNoise, (_p_).xy * 0.01 + (_p_).z * float2(0.01723059, 0.053092949), 0.0 ).r )

    float3 pi = Floor(pos);
    float3 pf = pos - pi;
    
    float3 w = pf * pf * (3.0 - 2.0 * pf);
    
    return 	Lerp(
        		Lerp(
        			Lerp(hash(pi + float3(0, 0, 0)), hash(pi + float3(1, 0, 0)), w.x),
        			Lerp(hash(pi + float3(0, 0, 1)), hash(pi + float3(1, 0, 1)), w.x), 
                    w.z),
        		Lerp(
                    Lerp(hash(pi + float3(0, 1, 0)), hash(pi + float3(1, 1, 0)), w.x),
        			Lerp(hash(pi + float3(0, 1, 1)), hash(pi + float3(1, 1, 1)), w.x), 
                    w.z),
        		w.y);
#	undef hash
}

float ValueNoise (const float3 pos)
{
#	define hash( _p_ )	ToSNorm( DHash13( _p_ ))

    float3 pi = Floor(pos);
    float3 pf = pos - pi;
    
    float3 w = pf * pf * (3.0 - 2.0 * pf);
    
    return 	Lerp(
        		Lerp(
        			Lerp(hash(pi + float3(0, 0, 0)), hash(pi + float3(1, 0, 0)), w.x),
        			Lerp(hash(pi + float3(0, 0, 1)), hash(pi + float3(1, 0, 1)), w.x), 
                    w.z),
        		Lerp(
                    Lerp(hash(pi + float3(0, 1, 0)), hash(pi + float3(1, 1, 0)), w.x),
        			Lerp(hash(pi + float3(0, 1, 1)), hash(pi + float3(1, 1, 1)), w.x), 
                    w.z),
        		w.y);
#	undef hash
}
//-----------------------------------------------------------------------------


// range [-1..1]
float PerlinNoise (sampler2D rgbaNoise, const float3 pos)
{
	// from https://www.shadertoy.com/view/4sc3z2
	// license CC BY-NC-SA 3.0
#	define hash( _p_ )	ToSNorm( textureLod( rgbaNoise, (_p_).xy * 0.01 + (_p_).z * float2(0.08723059, 0.053092949), 0.0 ).rgb )

    float3 pi = Floor(pos);
    float3 pf = pos - pi;
    
    float3 w = pf * pf * (3.0 - 2.0 * pf);
    
    return 	Lerp(
        		Lerp(
                	Lerp(Dot(pf - float3(0, 0, 0), hash(pi + float3(0, 0, 0))), 
                         Dot(pf - float3(1, 0, 0), hash(pi + float3(1, 0, 0))),
                         w.x),
                	Lerp(Dot(pf - float3(0, 0, 1), hash(pi + float3(0, 0, 1))), 
                         Dot(pf - float3(1, 0, 1), hash(pi + float3(1, 0, 1))),
                       	 w.x),
                	w.z),
        		Lerp(
                    Lerp(Dot(pf - float3(0, 1, 0), hash(pi + float3(0, 1, 0))), 
                         Dot(pf - float3(1, 1, 0), hash(pi + float3(1, 1, 0))),
                       	 w.x),
                   	Lerp(Dot(pf - float3(0, 1, 1), hash(pi + float3(0, 1, 1))), 
                         Dot(pf - float3(1, 1, 1), hash(pi + float3(1, 1, 1))),
                         w.x),
                	w.z),
    			w.y);
#	undef hash
}

float PerlinNoise (const float3 pos)
{
	// from https://www.shadertoy.com/view/4sc3z2
	// license CC BY-NC-SA 3.0
#	define hash( _p_ )	ToSNorm( DHash33( _p_ ))

    float3 pi = Floor(pos);
    float3 pf = pos - pi;
    
    float3 w = pf * pf * (3.0 - 2.0 * pf);
    
    return 	Lerp(
        		Lerp(
                	Lerp(Dot(pf - float3(0, 0, 0), hash(pi + float3(0, 0, 0))), 
                         Dot(pf - float3(1, 0, 0), hash(pi + float3(1, 0, 0))),
                         w.x),
                	Lerp(Dot(pf - float3(0, 0, 1), hash(pi + float3(0, 0, 1))), 
                         Dot(pf - float3(1, 0, 1), hash(pi + float3(1, 0, 1))),
                       	 w.x),
                	w.z),
        		Lerp(
                    Lerp(Dot(pf - float3(0, 1, 0), hash(pi + float3(0, 1, 0))), 
                         Dot(pf - float3(1, 1, 0), hash(pi + float3(1, 1, 0))),
                       	 w.x),
                   	Lerp(Dot(pf - float3(0, 1, 1), hash(pi + float3(0, 1, 1))), 
                         Dot(pf - float3(1, 1, 1), hash(pi + float3(1, 1, 1))),
                         w.x),
                	w.z),
    			w.y);
#	undef hash
}
//-----------------------------------------------------------------------------


float PerlinFBM (in float3 pos, const float lacunarity, const float persistence, const int octaveCount)
{
	float	value	= 0.0;
	float	pers	= 1.0;
	
	for (int octave = 0; octave < octaveCount; ++octave)
	{
		value += PerlinNoise( pos ) * pers;
		pos   *= lacunarity;
		pers  *= persistence;
	}
	return value;
}

float3 Turbulence (const float3 pos, const float power, const float lacunarity, const float persistence, const int octaveCount)
{
	const float3 p0 = pos + float3( 12414.0, 65124.0, 31337.0 ) / 65536.0;
	const float3 p1 = pos + float3( 26519.0, 18128.0, 60493.0 ) / 65536.0;
	const float3 p2 = pos + float3( 53820.0, 11213.0, 44845.0 ) / 65536.0;

	const float3 distort = float3(PerlinFBM( p0, lacunarity, persistence, octaveCount ),
								  PerlinFBM( p1, lacunarity, persistence, octaveCount ),
								  PerlinFBM( p2, lacunarity, persistence, octaveCount )) * power + pos;
	return distort;
}
//-----------------------------------------------------------------------------


// range [-1..1]
float SimplexNoise (sampler2D rgbaNoise, const float3 pos)
{
	// from https://www.shadertoy.com/view/4sc3z2
	// license CC BY-NC-SA 3.0
#	define hash( _p_ )	ToSNorm( textureLod( rgbaNoise, (_p_).xy * 0.02 + (_p_).z * float2(0.01723059, 0.053092949), 0.0 ).rgb )

    const float K1 = 0.333333333;
    const float K2 = 0.166666667;
    
    float3 i = Floor(pos + (pos.x + pos.y + pos.z) * K1);
    float3 d0 = pos - (i - (i.x + i.y + i.z) * K2);
    
    float3 e = Step(float3(0.0), d0 - d0.yzx);
	float3 i1 = e * (1.0 - e.zxy);
	float3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    float3 d1 = d0 - (i1 - 1.0 * K2);
    float3 d2 = d0 - (i2 - 2.0 * K2);
    float3 d3 = d0 - (1.0 - 3.0 * K2);
    
    float4 h = Max(0.6 - float4(Dot(d0, d0), Dot(d1, d1), Dot(d2, d2), Dot(d3, d3)), 0.0);
    float4 n = h * h * h * h * float4(Dot(d0, hash(i)), Dot(d1, hash(i + i1)), Dot(d2, hash(i + i2)), Dot(d3, hash(i + 1.0)));
    
    return Dot(float4(31.316), n);
#	undef hash
}

float SimplexNoise (const float3 pos)
{
	// from https://www.shadertoy.com/view/4sc3z2
	// license CC BY-NC-SA 3.0
#	define hash( _p_ )	ToSNorm( DHash33( _p_ ))

    const float K1 = 0.333333333;
    const float K2 = 0.166666667;
    
    float3 i = Floor(pos + (pos.x + pos.y + pos.z) * K1);
    float3 d0 = pos - (i - (i.x + i.y + i.z) * K2);
    
    float3 e = Step(float3(0.0), d0 - d0.yzx);
	float3 i1 = e * (1.0 - e.zxy);
	float3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    float3 d1 = d0 - (i1 - 1.0 * K2);
    float3 d2 = d0 - (i2 - 2.0 * K2);
    float3 d3 = d0 - (1.0 - 3.0 * K2);
    
    float4 h = Max(0.6 - float4(Dot(d0, d0), Dot(d1, d1), Dot(d2, d2), Dot(d3, d3)), 0.0);
    float4 n = h * h * h * h * float4(Dot(d0, hash(i)), Dot(d1, hash(i + i1)), Dot(d2, hash(i + i2)), Dot(d3, hash(i + 1.0)));
    
    return Dot(float4(31.316), n);
#	undef hash
}
//-----------------------------------------------------------------------------


// range [0..inf]
float  VoronoiContour (const float2 coord, const float seed)
{
	// from https://www.shadertoy.com/view/ldl3W8
	// The MIT License
	// Copyright © 2013 Inigo Quilez

	float2	ipoint	= Floor( coord );
	float2	fpoint	= Fract( coord );
	
	float2	icenter	= float2(0.0);
	float	md		= 2147483647.0;
	float2	mr;
	
	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
		float2	cur	= float2(x, y);
		float2	off	= DHash22( cur + ipoint + seed ) + cur - fpoint;
		float	d	= Dot( off, off );

		if ( d < md )
		{
			md = d;
			mr = off;
			icenter = cur;
		}
	}
	
	md = 2147483647.0;
	[[unroll]] for (int y = -2; y <= 2; ++y)
	[[unroll]] for (int x = -2; x <= 2; ++x)
	{
		float2	cur = icenter + float2(x, y);
		float2	off	= DHash22( cur + ipoint + seed ) + cur - fpoint;
		float	d   = Dot( mr - off, mr - off );
		
		if ( d > 0.00001 )
			md = Min( md, Dot( 0.5*(mr + off), Normalize(off - mr) ));
	}
	
	return md;
}


// range [0..inf]
float  VoronoiCircles (const float3 coord, const float radiusScale, const float seed)
{
	// based on shader from https://www.shadertoy.com/view/ldl3W8
	// The MIT License
	// Copyright © 2013 Inigo Quilez

	const int radius = 1;
	
	float3	ipoint	= Floor( coord );
	float3	fpoint	= Fract( coord );
	
	float3	icenter	= float3(0.0);
	float	md		= 2147483647.0;
	float	mr		= 2147483647.0;
	
	// find nearest circle
	[[unroll]] for (int z = -1; z <= 1; ++z)
	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
		float3	cur	= float3(x, y, z);
		float3	off	= DHash33( cur + ipoint + seed ) + cur - fpoint;
		float	d	= Dot( off, off );

		if ( d < md )
		{
			md = d;
			icenter = cur;
		}
	}
	
	// calc circle radius
	[[unroll]] for (int z = -2; z <= 2; ++z)
	[[unroll]] for (int y = -2; y <= 2; ++y)
	[[unroll]] for (int x = -2; x <= 2; ++x)
	{
		if ( AllEqual( int3(x,y,z), int3(0) ))
			continue;
		
		float3	cur = icenter + float3(x, y, z);
		float3	off	= DHash33( cur + ipoint + seed ) + cur - fpoint;
		float	d	= Dot( off, off );
		
		if ( d < mr )
			mr = d;
	}
	
	md = Sqrt( md );
	mr = Sqrt( mr ) * 0.5 * radiusScale;
	
	if ( md < mr )
		return 1.0 / (Square( md / mr ) * 16.0) - 0.07;

	return 0.0;
}
