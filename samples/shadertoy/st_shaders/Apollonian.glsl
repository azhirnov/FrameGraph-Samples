// from https://www.shadertoy.com/view/4ds3zn

// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// I can't recall where I learnt about this fractal.
//
// Coloring and fake occlusions are done by orbit trapping, as usual.

#include "RayTracing.glsl"

// Antialiasing level
#if HW_PERFORMANCE==0
#define AA 1
#else
#define AA 2 // Make it 3 if you have a fast machine
#endif

vec4 orb; 

float map( vec3 p, float s )
{
    float scale = 1.0;

    orb = vec4(1000.0); 
    
    for( int i=0; i<8;i++ )
    {
        p = -1.0 + 2.0*fract(0.5*p+0.5);

        float r2 = dot(p,p);
        
        orb = min( orb, vec4(abs(p),r2) );
        
        float k = s/r2;
        p     *= k;
        scale *= k;
    }
    
    return 0.25*abs(p.y)/scale;
}

float trace( in vec3 ro, in vec3 rd, float s )
{
    float maxd = 30.0;
    float t = 0.01;
    for( int i=0; i<200; i++ )
    {
        float precis = 0.001 * t;
        
        float h = map( ro+rd*t, s );
        if( h<precis||t>maxd ) break;
        t += h;
    }

    if( t>maxd ) t=-1.0;
    return t;
}

vec3 calcNormal( in vec3 pos, in float t, in float s )
{
    float precis = 0.001 * t;

    vec2 e = vec2(1.0,-1.0)*precis;
    return normalize( e.xyy*map( pos + e.xyy, s ) + 
                      e.yyx*map( pos + e.yyx, s ) + 
                      e.yxy*map( pos + e.yxy, s ) + 
                      e.xxx*map( pos + e.xxx, s ) );
}

vec3 render( in vec3 ro, in vec3 rd, in float anim )
{
    // trace	
    vec3 col = vec3(0.0);
    float t = trace( ro, rd, anim );
    if( t>0.0 )
    {
        vec4 tra = orb;
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal( pos, t, anim );

        // lighting
        vec3  light1 = vec3(  0.577, 0.577, -0.577 );
        vec3  light2 = vec3( -0.707, 0.000,  0.707 );
        float key = clamp( dot( light1, nor ), 0.0, 1.0 );
        float bac = clamp( 0.2 + 0.8*dot( light2, nor ), 0.0, 1.0 );
        float amb = (0.7+0.3*nor.y);
        float ao = pow( clamp(tra.w*2.0,0.0,1.0), 1.2 );

        vec3 brdf  = 1.0*vec3(0.40,0.40,0.40)*amb*ao;
        brdf += 1.0*vec3(1.00,1.00,1.00)*key*ao;
        brdf += 1.0*vec3(0.40,0.40,0.40)*bac*ao;

        // material		
        vec3 rgb = vec3(1.0);
        rgb = mix( rgb, vec3(1.0,0.80,0.2), clamp(6.0*tra.y,0.0,1.0) );
        rgb = mix( rgb, vec3(1.0,0.55,0.0), pow(clamp(1.0-2.0*tra.z,0.0,1.0),8.0) );

        // color
        col = rgb*brdf*exp(-0.2*t);
    }

    return sqrt(col);
}

vec4 RayTrace (const Ray ray)
{
    float time = iTime*0.25;
    float anim = 1.1 + 0.5*smoothstep( -0.3, 0.3, cos(0.1*iTime) );
    vec3 ro = vec3( 2.8*cos(0.1+.33*time), 0.4 + 0.30*cos(0.37*time), 2.8*cos(0.5+0.35*time) );

    vec3 col = render( ray.origin + ro, ray.dir, anim );
    return vec4( col, 1.0 );
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
