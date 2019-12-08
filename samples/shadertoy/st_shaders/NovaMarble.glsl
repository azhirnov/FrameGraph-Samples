// from https://www.shadertoy.com/view/MtdGD8

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Created by S. Guillitte 2015

#include "RayTracing.glsl"

float zoom=1.;

vec2 cmul( vec2 a, vec2 b )  { return vec2( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x ); }
vec2 csqr( vec2 a )  { return vec2( a.x*a.x - a.y*a.y, 2.*a.x*a.y  ); }


mat2 rot(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));	
}

vec2 iSphere( in vec3 ro, in vec3 rd, in vec4 sph )//from iq
{
	vec3 oc = ro - sph.xyz;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - sph.w*sph.w;
	float h = b*b - c;
	if( h<0.0 ) return vec2(-1.0);
	h = sqrt(h);
	return vec2(-b-h, -b+h );
}

float map(in vec3 p) {
	float res = 0.;
	
    vec3 c = p;
	for (int i = 0; i < 10; ++i) {
        p =.7*abs(p+cos(iTime*0.15+1.6)*0.15)/dot(p,p) -.7+cos(iTime*0.15)*0.15;
        p.yz= csqr(p.yz);
        p=p.zxy;
        res += exp(-19. * abs(dot(p,c)));
        
	}
	return res/2.;
}



vec3 raymarch (in Ray ray, float tmax)
{
    //float dt = .1;
    float dt = .1 - .075*cos(iTime*.025);//animated
    vec3 col= vec3(0.);
    float c = 0.;
    for( int i=0; i<64; i++ )
	{
		Ray_Move( INOUT ray, dt*exp(-2.*c) );

        if (ray.t > tmax) break;
        
        c = map( ray.pos );
        
        //col = .99*col+ .08*vec3(c*c, c, c*c*c);//gree
        col = .99*col+ .08*vec3(c*c*c, c*c, c);//blue
    }    
    return col;
}

vec4 RayTrace (in Ray ray)
{
	vec3 initial = vec3(0.0, 0.0, 3.5);
	Ray_SetOrigin( INOUT ray, ray.origin + initial );

    vec2 tmm = iSphere( ray.origin, ray.dir, vec4(0.,0.,0.,2.) );
	Ray_SetLength( INOUT ray, tmm.x );

    vec3 col = raymarch( ray, tmm.y );
	
    col =  .5 *(log(1.+col));
    col = clamp(col,0.,1.);
    return vec4( col, 1.0 );
}
//-----------------------------------------------------------------------------


void mainVR (out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir)
{
	Ray	ray = Ray_Create( fragRayOri, fragRayDir, 0.1 );
    fragColor = RayTrace( ray );
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 0.1, fragCoord / iResolution.xy );
	fragColor = RayTrace( ray );
}
